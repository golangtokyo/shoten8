= GoとコンセンサスアルゴリズムRaftによる分散システム構築入門

こんにちは@<b>{@po3rin}@<fn>{po3rin}です。仕事ではメインでGoを使っています。今回は@<b>{コンセンサスアルゴリズム}の1つである@<b>{Raft}@<fn>{raft}の紹介と、実際にGoとRaftパッケージを使った分散システムの開発方法を紹介します。Kubernetesの内部でも使われているetcdや、全文検索エンジンのAlgoria、分散データベースのCockroachDBなど様々な技術の裏でこのRaftアルゴリズムが使われています。Raftの仕組みを知ることで分散システムについてより理解が深まることは間違いありません。本章はコンセンサスアルゴリズム初心者向けであり、Go+Raftによる分散システムの実装方法の紹介を中心に行う為、Raftの説明は仕組みがなんとなく分かる程度にとどめています。本章ではGoでオリジナルの分散システムを実装する入り口に立つことがゴールです。サンプルコードは@<code>{https://github.com/po3rin/kvraft}にあります。

//footnote[po3rin][@<href>{https://twitter.com/po3rin}]
//footnote[raft][@<href>{https://raft.github.io/raft.pdf}]

== なぜコンセンサスアルゴリズムが必要か

@<b>{コンセンサスアルゴリズム}はクラスターのノードの一部の障害に耐えることを可能にするアルゴリズムです。安全な分散システムを構築する上で重要な役割を果たします。Raftの前にそもそもなぜコンセンサスアルゴリズムが必要なのかを見ていきましょう。たとえばチームで１つのプロセスだけでデータストアを運用している場合、そのデータストアが障害で落ちたら重大な問題になります。その為、高可用性を担保するために冗長化を行うのが一般的です(@<img>{ha1})。

//image[ha1][サーバーを2つ用意することで冗長化を行う][scale=0.4]

しかし、データストアを複数台動かそうとすると別の問題が発生します。障害に耐えるためには１つのデータストアがもつ状態を別のデータストアで一致させなければいけません。たとえばAとBのデータストアに対してデータを更新する途中で障害が発生したとします。Aは更新できたけど、Bは更新できませんでした。そうするとAとBでデータが一致しなくなり、Bが復活したとしても間違ったデータを返してしまいます(@<img>{ha2})。実はダウンタイムなしで複数のデータストアの状態を一致させるのは一筋縄ではいきません。

//image[ha2][データストアのうちの１つが障害で更新処理できず][scale=0.52]

そこでデータストアなどを@<b>{State machine}としてとらえることが重要になります。State machineを超シンプルに説明すると、「同じ操作を同じ順序で実行すれば必ず同じ状態になる」というモデルです。今回の例でいうと50円入金、30円入金、20円入金と、入力によってデータストアの状態が決定します(@<img>{ha3})。つまりデータストアはState Machineとしてモデル化できるということです。分散システムにおいてはデータストアへの操作のログを複製して保存しておき、同じ操作を同じ順序で実行することを保証することで、同じ状態を維持できるようにします。このようなアーキテクチャを@<b>{Replicated state machines}と言います。

//image[ha3][ログを複製して同じ操作を同じ順序で実行することで同じ状態を維持できる][scale=0.8]

これで、いつデータストアがダウンしてもログを順番に適用すれば元の状態に戻せます。先ほど説明したように「同じ操作を同じ順序で実行すれば必ず同じ状態になる」ことが分かっているからです。ここで「一部のログが欠損したらどうするのか」「ログの順序が入れ替わったらどうするのか」という疑問が生まれているはずです。この複製されたログの一貫性を保つことがコンセンサスアルゴリズムの仕事です。次の節からはコンセンサスアルゴリズムの１つであるRaftアルゴリズムの概要を通じて、コンセンサスアルゴリズムがどのように複製されたログの一貫性を保っているのかを説明していきます。

== Raftとは
コンセンサスアルゴリズムにはいくつか種類があり、その中で@<b>{Paxos}というアルゴリズムが過去10年間メインで使われ続けていました。一方でPaxosのアルゴリズムはかなり複雑で理解が難しいものでした。そこで登場したのが@<b>{Raft}というアルゴリズムです。Raftの論文によれば、RaftはPaxosと同じくらい信頼性と効率性がある一方で、PaxosよりRaftの方がシンプルで理解しやすい構造になっているとのことです。Kubernetesの内部で使われているetcdや全文検索エンジンのAlgoria、分散データベースのCockroachDBなども様々な技術がこのRaftを使っています。

=== どのようにログ複製を管理するか

Raftの論文においてはリーダーの選出方法を先に説明していますが、本章では理解しやすさのためにログ複製の管理をどのように行っているかを先に説明します。Raftは最初に1人のリーダーを選出し、複製されたログの管理に関する完全な責任をリーダーに与えることによってコンセンサスを実現します。クライアントからのリクエストは必ずリーダーが受け取ります(@<img>{logrep1})。もしリクエストがフォロワーに届いてもリクエストをリーダーに渡します。

//image[logrep1][クライアントからのリクエストは必ずリーダーが処理する][scale=0.7]

そしてリクエストを受け取ったリーダーはまず自分のログに命令のログ（Raftの論文では@<b>{ログエントリ}や@<b>{エントリ}と呼ばれている。以降エントリ）を書き込みます(@<img>{logrep2})。ログにエントリを書き込むだけで実際のデータの操作は行われていません。

//image[logrep2][リーダーがエントリを自分のログに書き込む][scale=0.7]

リーダーがエントリをログに書き込んだら、フォロワーに対してエントリ追加のリクエストである@<b>{AppendEntries RPC}を発行します(@<img>{logrep3})。このリクエストを受け取ったフォロワーは自分のログにエントリを追加します。これでリーダーのエントリがフォロワーに複製されたといえます。AppendEntries RPCという言葉が出てきたとおり、RaftサーバーはRPCを使用して通信を行っています。基本的なコンセンサスアルゴリズムには2種類のRPCのみを必要とします。AppendEntries RPCと@<b>{RequestVote RPC}です。まだ説明していない@<b>{RequestVote RPC}はのちほど紹介します。RPCの定義はRaftの論文@<fn>{raft}にまとまっているので読んでみてください。

//image[logrep3][リーダーがフォロワーにエントリを保存させる][scale=0.7]

フォロワーがログにエントリを追加できたらリーダーにレスポンスを返します(@<img>{logrep4})。リーダーは過半数のフォロワーからリクエストが返ってきたらログがフォロワーに複製されと分かる、すなわちエントリが@<b>{コミット}されたと分かります。Raftの文脈では「エントリがState machineに適用されても安全である」とコンセンサスが取れた場合、そのエントリは@<b>{コミット済み}といいます。データストアなどの例では、コミット済みのエントリ（+10円など）のみを実際の命令としてデータストアに適用できます。

//image[logrep4][リーダーは過半数のフォロワーからレスポンスが返ってきたら、エントリがコミットされたとみなす][scale=0.7]

リーダーはすべてのフォロワーが最終的に自分がもつすべてのエントリを保存するまでAppendEntries RPCを無制限に再実行します。

=== リーダー選挙

先ほどの説明で、ログ複製においてリーダーに重要な権限が与えられていることが分かりました。しかしリーダーが障害にあったらどうなるのでしょうか。そのときにはリーダー選挙が行われます。リーダー選挙もRaftにおける重要な要素です。リーダー選挙が発生する流れを見てみましょう。実はリーダーは常にエントリを持たないAppendEntries RPCを使ってハートビートをフォロワーに送っています(@<img>{election1})。

//image[election1][リーダーは常にハートビートをフォロワーに送っている][scale=0.6]

リーダーからのハートビートが来なくなったらフォロワーは「リーダーが死んでいる」と判断します(@<img>{election2})。そのタイムアウト時間を@<b>{選挙タイムアウト}と言います。フォロワーがハートビートを受け取ると、フォロワーは選挙タイムアウトを再度ランダムに設定します。

//image[election2][選挙タイムアウトになるまでリーダーからのハートビートを待つ][scale=0.5]

選挙タイムアウトを過ぎるとフォロワーという状態から@<b>{候補者}という状態に遷移します(@<img>{election3})。Raftにおいて各メンバーはリーダー、フォロワー、候補者のいずれかの状態になり、リーダーは最大で一人です。ちなみに起動時の一番初めは全員がフォロワーの状態からスタートします。候補者は自分に1票を投じ、また自分への投票を別のメンバーに@<b>{RequestVote RPC}を使ってリクエストし、過半数から投票されたら新しいリーダーとして選出されます(@<img>{election4})。

//image[election3][選挙が開始され、候補者は自分に1票を投じた後、他のノードへ自分への投票を要求する][scale=0.5]

//image[election4][過半数の票が集まったので候補者からリーダーになる][scale=0.5]

=== タームとログの構成

リーダー選挙のサイクルをまとめるとRaftアルゴリズムは(@<img>{cycle})なサイクルを持ちます。

//image[cycle][各タームは選挙で開始する。選挙が成功した後、タームが終わるまで1人のリーダーが存在する][scale=0.7]

Raftでは@<b>{ターム}という時間の分割単位があり、選挙開始から次の選挙の開始までを1タームとします。ログはエントリを受信した時のターム番号も一緒に格納しています。ログは@<img>{log}のような構成になっています。エントリを追加する際にターム番号も一緒に保存することでRaftはログの不一致や、ログの遅れなどをAppendEntries RPCを通して検知できます。

//image[log][ログの構成。エントリをターム番号と一緒に格納する][scale=0.8]

@<img>{log}ではコミット済みの（過半数のサーバーに複製された）エントリはindex番号4までのエントリです。ここまではState machineにエントリを適用しても安全というわけです。Raftではログに安全性を確保するための２つの重要な特性があります。

 1. 同じindex番号とタームがある場合、それらは同じコマンドを保持している。
 2. 同じindex番号とタームがある場合、それ以前のすべてのエントリにおいてログは同一である。

この特性により、もしリーダーがフォロワーのタームとエントリが不一致する箇所を検知したら一致する最新のエントリまでさかのぼり、そこまでをリーダーのもつエントリで上書きしていきます。一致する最新のエントリ以前はチェックしなくてもログの一致が保証されているからです。これらのすべての動作は AppendEntries RPCによって実行されます。

=== Raftをもっと理解する為に

ここまでRaftの概要を見てきました。Raftにはもう少し細かいルールがあります（たとえばログにコミット済みエントリがすべて含まれていないと候補者は選挙に勝てないなど）。しかし一応Goに関する本ですので、Raftの詳細な動作に関しては別の優れた文献に任せます。もっとRaftを勉強するためには当然Raftの論文@<fn>{raft}がオススメですが、この論文の日本語訳のブログ@<fn>{raft-ja}もあります。Raftの動作をstep-by-stepでビジュアライズしながら説明するサイト@<fn>{raft-viz}もあるのでこちらも理解を進めるのに便利です。またRaftに関するスライド@<fn>{slide}もあり、こちらは日本語で解説されています。

//footnote[raft-ja][@<href>{https://hazm.at/mox/distributed-system/algorithm/transaction/raft/index.html}]
//footnote[raft-viz][@<href>{http://thesecretlivesofdata.com/raft/}]
//footnote[slide][@<href>{https://www.slideshare.net/pfi/raft-36155398}]

== Goでシンプルなキーバリューストアを実装する

この節では分散キーバリューストアを実装する前段階&Goの復習も兼ねてシンプルなキーバリューストアを作ってみましょう。シンプルなキーバリューストアの完成形は@<code>{https://github.com/po3rin/kvraft/pull/1}にあります。HTTPリクエストを受けてデータを操作します。図にすると@<img>{kv}のようになります。

//image[kv][シンプルなキーバリューストア][scale=0.9]

この節では最終的には@<list>{tree1}のようなファイル構成になります。

//list[tree1][最終的な構成][]{
.
├── go.mod
├── go.sum
├── server
│   └── server.go
├── main.go
└── store
    └── store.go
//}

まずは@<b>{go module}を使うために@<list>{mod}コマンドを実行して、プロジェクトの準備をしましょう。

//list[mod][最終的な構成][]{
$ mkdir kvraft
$ cd kvraft

// ****** は置き換えてください
$ go mod init github.com/******/kvraft
//}

まずはキーバリューストアとなる@<b>{store}パッケージを作ります。@<code>{store/store.go}を作ります。(@<list>{store1})。

//list[store1][storeパッケージ][go]{
package store

// ...

type Store struct {
	mu      sync.RWMutex
	kvStore map[string]string
}

func New() *Store {
	return &Store{
		kvStore: make(map[string]string),
	}
}

func (s *Store) Lookup(key string) (string, bool) {
	s.mu.RLock()
	defer s.mu.RUnlock()
	v, ok := s.kvStore[key]
	return v, ok
}

func (s *Store) Save(key string, value string) error {
	s.mu.Lock()
	defer s.mu.Unlock()
	s.kvStore[key] = value
	return nil
}
//}

@<list>{store1}で実装した@<code>{Store}構造体の@<code>{mu}フィールドは排他制御のためのものです。@<code>{sync.RWMutex}構造体はゼロ値でそのまま使えるので、@<code>{New}関数内で明示的な初期化は必要ありません。続いてキーバリューストアにアクセスするためのAPIを作りましょう。@<code>{server/server.go}を作ります(@<list>{server1})。

//list[server1][serverパッケージ][go]{
package server

// ...

type Store interface {
	Lookup(key string) (string, bool)
	Save(k string, v string) error
}

type handler struct {
	store Store
}

type Server struct {
	server http.Server
}
//}

@<list>{server1}では@<code>{handler}型の内部で@<code>{Store}インターフェースを保持します。これで@<code>{Store}の実装を切り替えやすくなり、モックテストも行いやすくなります。続いて@<code>{handler}にメソッドを定義します(@<list>{server2})。今回は簡略化のためにフレームワークである@<b>{Gin}@<fn>{gin}を使います。

//footnote[gin][@<href>{https://github.com/gin-gonic/gin}]

//list[server2][serverパッケージのhandler][go]{
import (
	// ...
	"github.com/gin-gonic/gin"
)

type Request struct {
	Key   string `json:"key"`
	Value string `json:"value"`
}

func (h *handler) Get(c *gin.Context) {
	key := c.Param("key")
	v, ok := h.store.Lookup(key)
	if !ok {
		c.JSON(http.StatusNotFound, gin.H{
			"error": "not found",
		})
		return
	}
	c.JSON(http.StatusOK, gin.H{
		key: v,
	})
}

func (h *handler) Put(c *gin.Context) {
	var req Request
	c.BindJSON(&req)
	err := h.store.Save(req.Key, string(req.Value))
	if err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{
			"error": err,
		})
		return
	}
	c.JSON(http.StatusOK, gin.H{
		req.Key: string(req.Value),
	})
}
//}

今回はGET、PUTメソッドでそれぞれデータの保存、取得を行っています。続いてAPIを立ち上げるメソッドを準備しましょう(@<list>{server.run})。

//list[server.run][serverパッケージの*Server.Runメソッド][go]{
import (
	// ...
	"golang.org/x/sync/errgroup"
)

func New(port int, kv Store) *Server {
	h := &handler{
		store: kv,
	}
	r := gin.Default()
	r.GET("/:key", h.Get)
	r.PUT("/", h.Put)

	return &Server{
		server: http.Server{
			Addr:    ":" + strconv.Itoa(port),
			Handler: r,
		},
	}
}

func (s *Server) Run(ctx context.Context) error {
	// errorを返したらコンテキストをキャンセル
	eg, ctx := errgroup.WithContext(ctx)
	eg.Go(func() error {
		return s.server.ListenAndServe()
	})

	// コンテキストキャンセルを受けたらサーバーのシャットダウン
	<-ctx.Done()
	sCtx, sCancel := context.WithTimeout(
		context.Background(), 10*time.Second,
	)
	defer sCancel()
	if err := s.server.Shutdown(sCtx); err != nil {
		return err
	}

	// gorutineが終了するまで待つ
	return eg.Wait()
}
//}

@<list>{server.run}ではAPIの起動に必要な@<code>{Server}の初期化を@<code>{New}関数で行い、@<code>{Server.Run}メソッドで実際の立ち上げを行います。@<code>{Server.Run}では呼び出し元がコンテキストを経由で@<code>{ListenAndServe}をとめられるようにしています。ここでは準標準パッケージの@<code>{golang.org/x/sync/errgroup}を使っています。これにより1つのサブタスクでエラーが発生した場合に、@<code>{errgroup.WithContext}関数で生成したコンテキストをキャンセルできます。そしてgorutineで出したエラーを@<code>{*Server.Run}の戻り値として呼び出し元に返すことができます。@<code>{errgroup}の使い方やExampleはドキュメント@<fn>{errgroup}をご覧ください。

//footnote[errgroup][@<href>{https://pkg.go.dev/golang.org/x/sync/errgroup?tab=doc}]

最後にmain.goを実装しましょう(@<list>{main})。

//list[main][mainパッケージの関数][go]{
package main

import (
	// ...

	//パッケージ名は変えてください
	"github.com/******/kvraft/server"
	"github.com/******/kvraft/store"
)

func main() {
	port := flag.Int("port", 3000, "key-value server port")
	flag.Parse()

	s := store.New()
	srv := server.New(*port, s)

	// とりあえずここでは context.Background() を渡す。
	log.Println(srv.Run(context.Background()))
}
//}

これで超シンプルなキーバリューストアができました。動作を確認しましょう。

//list[try1][シンプルなキーバリューストアの動作確認][go]{
$ go run main.go

$ curl -X PUT http://localhost:3000 \
   -H "Content-Type: application/json" \
   -d '{"key": "hello", "value": "golang"}'
#結果
{"hello":"golang"}

$ curl http://localhost:3000/hello
#結果
{"hello":"golang"}
//}

ちゃんとPUTで保存したデータがGETで取得できています。ここからがいよいよ本番です。次の節からキーバリューストアをRaftに対応させていきます。

== Raftパッケージ比較

Raftアルゴリズムをゼロから実装するのはかなり骨の折れる作業です。ありがたいことにGoにおいて、様々なRaftパッケージが存在します。今回は既存のRaftパッケージを使った分散キーバリューストアの開発方法を紹介します。本番利用もされているRaftパッケージでは@<b>{github.com/etcd-io/etcd/raft}@<fn>{etcd}や@<b>{github.com/hashicorp/raft}@<fn>{hashi}があります。etcdのraftはその名のとおりKubernetesの内部などで使われている@<b>{etcd}@<fn>{etcd}での利用を目的として作られたRaftパッケージです。hashicorpのraftパッケージはネットワーキングサービスである@<b>{Consul}@<fn>{consul}などで利用されています。筆者が2つのパッケージを比較したところ、etcd/raftの方がRaftアルゴリズムのコアのみにフォーカスして作られており、最小限の設計哲学にしたがっています。そのため今回の開発ではRaftの勉強もかねるという観点からetcd/raftを使ってみましょう。

//footnote[etcd][@<href>{https://github.com/etcd-io/etcd}]
//footnote[hashi][@<href>{https://github.com/hashicorp/raft}]
//footnote[consul][@<href>{https://github.com/hashicorp/consul}]

== Raftを利用するraftalgパッケージを実装する

ここからのコードの変更は@<code>{https://github.com/po3rin/kvraft/pull/2}に差分が置いてあります。

=== 実装方針

一気に完璧なRaftを構築する前に必要な機能をstep-by-stepで実装しつつ動作確認できたら次の機能を実装という形で進めていきます。
先ほどお話したとおり、今回使う@<b>{etcd/raft}パッケージはRaftアルゴリズムのコアのみにフォーカスして実装されています。ログのコンセンサスをRaftにお願いする手前までや、エントリの保存、コミット済みのエントリの扱いなどはこちらで実装する必要があります。その為、まずはRaftアルゴリズムを利用する責務をもつ@<b>{raftalg}パッケージを作ります。ここでは外部から依頼を受けてRaftアルゴリズムに処理を渡す部分と、Raftアルゴリズムからの結果を処理する仕事があります。少しコードが複雑になるので図にしてみました(@<img>{arch})。

//image[arch][大雑把にraftalgパッケージが行うことの構成図][scale=1]

@<img>{arch}をみて分かるとおりRaftで別のノードと通信する必要があるのでそれらの管理もこの@<b>{raftalg}パッケージの責務です。etcd/raftが本当にRaftアルゴリズムのコアだけに集中して作られたパッケージだということが見て取れます。この@<b>{raftalg}パッケージを使うと先ほどの実装(@<img>{kv})が@<img>{arch2}のように変わります。

//image[arch2][storeからraftalgパッケージを利用する][scale=1]

この節では最終的に@<list>{tree2}のような構成になります。

//list[tree2][最終的な構成][go]{
.
├── go.mod
├── go.sum
├── handler
│   └── handler.go
├── raftalg
│   └── raftalg.go
├── main.go
└── store
    └── store.go
//}

=== Raftノードの起動準備

まずは@<code>{raftalg/raftalg.go}に構造体と関数を定義します。

//list[raftalg1][raftalgパッケージ][go]{
package raftalg

import (
	// ...

	"github.com/coreos/etcd/raft"
	"github.com/coreos/etcd/rafthttp"
	"github.com/coreos/etcd/wal"
)

type RaftAlg struct {
	commitC         chan string
	doneRestoreLogC chan struct{}

	node        raft.Node
	raftStorage *raft.MemoryStorage
	wal         *wal.WAL
	transport   *rafthttp.Transport

	id     int
	peers  []string
	waldir string
}


func New(id int, peers []string) *RaftAlg {
	return &RaftAlg{
		commitC:         make(chan string),
		doneRestoreLogC: make(chan struct{}),

		raftStorage: raft.NewMemoryStorage(),

		id:     id,
		peers:  peers,
		waldir: fmt.Sprintf("kvraft-%d", id),
	}
}
//}

ここで@<list>{err}のエラーが出る人は@<code>{go.mod}に@<list>{gomod}の記述を追加してバージョンを固定してあげると治ります。詳細はissue@<fn>{issue}へ。

//list[err][importで発生するエラー][txt]{
github.com/coreos/go-systemd/journal: no matching versions for query "latest"
//}

//list[gomod][go.modに追加][go]{
replace github.com/coreos/go-systemd => //紙面上では長いので改行
	github.com/coreos/go-systemd/v22 v22.0.0
//}

//footnote[issue][@<href>{https://github.com/coreos/go-systemd/issues/321}]

@<list>{raftalg1}ではエントリを保存するためのメモリストレージを@<code>{raft.NewMemoryStorage}関数で生成しています。@<code>{id}はノードのIDで@<code>{peers}はクラスター内で通信するメンバーのURLです。@<code>{RaftAlg}のフィールドは実装しながら順次補足します。続いて@<code>{*RaftAlg.Run}メソッドを実装します。これはノードの起動に対する責務を持ちます。そしてRaftの結果がチャネルで渡ってくるのでそれの待ち受けや、ノードが相互に通信するHTTPのListenもRunが起動します。まずはノードを起動するための設定を用意します@<list>{raftalg2}。

//list[raftalg2][raftalg.Run][go]{
func (r *RaftAlg) Run(ctx context.Context) error {
    c := &raft.Config{
        ID:              uint64(r.id),
        ElectionTick:    10,
        HeartbeatTick:   1,
        Storage:         r.raftStorage,
        MaxSizePerMsg:   1024 * 1024,
        MaxInflightMsgs: 256,
        Logger: &raft.DefaultLogger{
            Logger: log.New(
                os.Stderr,
                "[Raft-debug]",
                0,
            ),
        },
    }

    // ...
}
//}

そしてクラスターのメンバーのURLを@<b>{etcd/raft}パッケージで使う用の型に変換します(@<list>{peers})。最終的にノードを起動するための関数の引数に渡すためです。

//list[peers][peersの型変換][go]{
func (r *RaftAlg) Run(ctx context.Context) error {
    // ...続き

    rpeers := make([]raft.Peer, len(r.peers))
    for i := range rpeers {
        rpeers[i] = raft.Peer{ID: uint64(i + 1)}
    }

    // ...
}
//}

これでクラスタのノードの起動に必要なもの（@<code>{raft.Config}型と@<code>{[]raft.Peer}型）が準備できました。

=== Raftノードの起動とWALのリプレイ

さっそくノードを起動します。Raftノードの起動には2パターンがあり、新規起動と再起動の２つがあります。どちらを使うかはWALの保存用ディレクトリが存在するかどうかで決定しましょう。@<b>{WAL}（Write-Ahead Log）は文字どおり、writeする前にlogを保存しておくことです。Raftの文脈においてはログと同じ意味です。State machine への入力をエントリとしてログに保存すると説明したのを思い出してください。WALのI/Oに関しては便利な@<code>{etcd/wal}パッケージが提供されています。WAL保存用のディレクトリの存在チェックとログのリプレイを行った後にノードを起動するようにします(@<list>{wal})。

//list[wal][WALのreplayとノードの起動][go]{
func (r *RaftAlg) Run(ctx context.Context) error {
    // ...続き

    // wal保存用ディレクトリがすでにあるか確認
    oldwal := wal.Exist(r.waldir)

    // 再起動のためにWALに保存されたエントリを適用します。
    w, err := r.replayWAL(ctx) // あとで実装
    if err != nil {
        return err
    }
    r.wal = w

    if oldwal {
        r.node = raft.RestartNode(c)
    } else {
        r.node = raft.StartNode(c, rpeers)
    }

    // ...
}
//}

続いてWALをリプレイする為の@<code>{*RaftAlg.replayWAL}メソッドを定義しましょう(@<list>{replayWAL})。少しエラーハンドリングが多いのでここでは省略しています。

//list[replayWAL][replayWALの実装][go]{

import (
	// ...
	"github.com/coreos/etcd/wal/walpb"
	// ...
)

func (r *RaftAlg) replayWAL(ctx context.Context) (*wal.WAL, error) {
	// WALディレクトリの存在チェック。無ければ作る。
	if !wal.Exist(r.waldir) {
		_ = os.Mkdir(r.waldir, 0750)
		w, _ := wal.Create(r.waldir, nil)
		w.Close()
	}

	// WALに保存されたログを取得
	w, _ := wal.Open(r.waldir, walpb.Snapshot{})
	_, _, ents, _ := w.ReadAll()

	// Raftノードが適切なログからスタートできるようにエントリをログに追加
	_ = r.raftStorage.Append(ents)

	select {
	// replayの終了を通知する
	case r.doneRestoreLogC <- struct{}{}:
	case <-time.After(10 * time.Second):
		return nil, errors.New(
			"timeout(10s) receiving done restore channel",
		)
	case <-ctx.Done():
		return nil, ctx.Err()
	}

	// エントリをState machineに適用する
	_ = r.publishEntries(ctx, ents) // あとで実装

	return w, nil
}
//}

@<list>{replayWAL}から分かるとおり今回WALはfileで保存しています。WALに保存されたログを取得して、メモリストレージに移します。その後に、WALのreplayを待っているストアにチャネルで終了を通知します。これでノード再起動時などにログを復元できます。ログがメモリストレージに移せたら、キーバリューストアにデータ操作行うメソッドである@<code>{*RaftAlg.publishEntries}をコールします。ちなみにselect文ではreplayの終了を通知するチャネルの送信と同時にタイムアウトと@<b>{context}パッケージによる割り込みを可能にしています。タイムアウトに関してはreplayを通知する相手がまだ立ち上がっていないのは何らかの問題があるとして処理を中断する必要があるからです。これから何回か出てくる実装パターンなので覚えておきましょう。


=== エントリの配送

それではエントリをState machineに適用する為の@<code>{*RaftAlg.publishEntries}メソッドを実装しましょう(@<list>{publish})。

//list[publish][publishEntriesの実装][go]{

import (
	// ...
	"github.com/coreos/etcd/raft/raftpb"
)

func (r *RaftAlg) publishEntries(
	ctx context.Context, ents []raftpb.Entry,
) error {
	for i := range ents {
		if ents[i].Type != raftpb.EntryNormal ||
			len(ents[i].Data) == 0 {
			// EntryNormal 型のエントリのみをサポートする
			// 他のタイプについては後述
			continue
		}
		s := string(ents[i].Data)

		select {
		// 適用して良いエントリをチャネル経由で通知する
		case r.commitC <- s:
		case <-time.After(10 * time.Second):
			return errors.New(
				"timeout(10s) sending committed channel",
			)
		case <-ctx.Done():
			return ctx.Err()
		}
	}
	return nil
}
//}

@<list>{publish}では@<code>{commitC}チャネルにエントリのデータを配送します。今回の実装ではストア側でエントリを@<code>{commitC}チャネル経由で受け取り適用（今回の例でいえば@<code>{map[string]string}へのappend）を行います。のちほど説明しますが、etcd/raftにおいてエントリには数種類があります。今は@<code>{raftpb.EntryNormal}型のエントリだけを扱うようにしますが、あとで別のエントリのタイプもサポートします。


=== Raftノードの通信用サーバーの立ち上げ

ノードが相互に通信する仕組みも必要です（エントリ追加要求や、ハートビート、投票要求などをしなければなりません）。これはetcd/raftが提供している@<code>{rafthttp.Transporter}というインターフェースが担います。@<code>{*RaftAlg.Run}メソッドの続きを実装しましょう(@<list>{transport})。

//list[transport][rafthttp.Transportの利用][go]{
import (
	// ...
	"github.com/coreos/etcd/etcdserver/stats"
	"github.com/coreos/etcd/pkg/types"
	// ...
)

func (r *RaftAlg) Run(ctx context.Context) error {
	// 続き...

	r.transport = &rafthttp.Transport{
		ID:          types.ID(r.id),
		ClusterID:   0x1000,
		// raft.Raftインターフェース(あとでRaftAlgに実装)
		Raft:        r,
		// 通信の統計を記録するために使用されます
		ServerStats: stats.NewServerStats("", ""),
		//リーダーがフォロワーとの通信の統計を記録するために使用されます
		LeaderStats: stats.NewLeaderStats(strconv.Itoa(r.id)),
		ErrorC:      make(chan error),
	}

	err = r.transport.Start()
	if err != nil {
		return err
	}

	for i := range r.peers {
		if i+1 != r.id {
			r.transport.AddPeer(
				types.ID(i+1), []string{r.peers[i]},
			)
		}
	}

	// ...
}
//}

@<code>{rafthttp.Transport}がすでに@<code>{rafthttp.Transporter}インターフェースを実装しておりノード間の通信を行う際にはこれを使うのが簡単です。そして、クラスターを組むノードのURLを@<code>{rafthttp.Transport.AddPeer}メソッドで教えてあげます。@<code>{rafthttp.Transporter}には@<code>{Handler}メソッドが生えておりこちらからGopherお馴染み@<code>{http.Handler}インターフェースを取得できます。これを@<code>{http.Server}構造体に渡してListenします(@<list>{serve})。

//list[serve][serveRaftHTTPの実装][go]{
func (r *RaftAlg) serveRaftHTTP(ctx context.Context) error {
	url, err := url.Parse(r.peers[r.id-1])
	if err != nil {
		return fmt.Errorf("failed parsing URL: %w", err)
	}
	srv := http.Server{Addr: url.Host, Handler: r.transport.Handler()}

	eg, ctx := errgroup.WithContext(ctx)
	eg.Go(func() error {
		return srv.ListenAndServe()
	})

	// コンテキストキャンセルを受けたらシャットダウン
	<-ctx.Done()
	sCtx, sCancel := context.WithTimeout(
		context.Background(), 10*time.Second,
	)
	defer sCancel()
	if err := srv.Shutdown(sCtx); err != nil {
		return err
	}

	return eg.Wait()
}
//}

@<list>{serve}では@<list>{server.run}と同じように、@<code>{*RaftAlg.serveRaftHTTP}メソッドの呼び出し元がコンテキスト経由でサーバーを終了できるようにしています。@<code>{rafthttp.Transport}インターフェースには@<code>{raft.Raft}インターフェースを満たした実装を渡す必要があります。@<code>{raft.Raft}インターフェースがもつ4つのメソッドを@<code>{*RaftAlg}に実装しましょう(@<list>{implRaft})。

//list[implRaft][raft.Raftインターフェースを実装する][go]{
func (r *RaftAlg) Process(ctx context.Context, m raftpb.Message) error {
	return r.node.Step(ctx, m)
}
func (r *RaftAlg) IsIDRemoved(id uint64) bool {
    return false
}
func (r *RaftAlg) ReportUnreachable(id uint64) {}
func (r *RaftAlg) ReportSnapshot(id uint64, status raft.SnapshotStatus) {}
//}

@<code>{*RaftAlg.Run}の最後では、別のRaftノードと通信する為の @<code>{*RaftAlg.serveRaftHTTP}の呼び出しを行います。同時にキーバリューストアとのやりとりする為のチャネル処理用の@<code>{*RaftAlg.serveChannels}を呼び出しも行います(@<list>{gorun})。

//list[gorun][rafthttp.Transportの利用][go]{
func (r *RaftAlg) Run(ctx context.Context) error {
	// 続き...

	eg, ctx := errgroup.WithContext(ctx)
	eg.Go(func() error {
		return r.serveRaftHTTP(ctx)
	})
	eg.Go(func() error {
		return r.serveChannels(ctx)
	})

	if err := eg.Wait(); err != nil {
		return fmt.Errorf("tryraft: stop serving Raft: %w", err)
	}
	return nil
}
//}

=== Raftや外部パッケージとのやりとりをチャネルでさばく

続いてRaftとチャネル経由でやりとりする為の@<code>{*RaftAlg.serveChannels}を実装します(@<list>{serveChannels})。ここでもエラーハンドリングが多いのである程度省略します。

//list[serveChannels][serveChannelsの実装][go]{
func (r *RaftAlg) serveChannels(ctx context.Context) error {
	ticker := time.NewTicker(100 * time.Millisecond)
	defer ticker.Stop()

	for {
		select {
		case <-ticker.C:
			r.node.Tick()

		case rd := <-r.node.Ready():
			// WALとメモリストレージにエントリを保存
			_ = r.wal.Save(rd.HardState, rd.Entries)
			_ = r.raftStorage.Append(rd.Entries)

			r.transport.Send(rd.Messages)

			// コミット済みエントリを通知
			err := r.publishEntries(ctx, rd.CommittedEntries)
			if err != nil {
				return err
			}
			r.node.Advance()

		case err := <-r.transport.ErrorC:
			return err

		case <-ctx.Done():
			return ctx.Err()
		}
	}
}
//}

ここではコンテキストキャンセルや、トランスポーターからのエラーを受ける以外に、重要な２つの@<code>{case}を扱っています。１つ目の@<code>{case}は定期的に@<code>{time.Ticker}で実行しなければならない@<code>{raft.Node.Tick}メソッドを実行するケースです。Raftには、ハートビートと選挙タイムアウトの2つの重要なタイムアウトがあると説明しました。@<b>{etcd/raft}パッケージの内部では、時間はTickで抽象化されています。たとえば@<list>{raftalg2}において@<code>{raft.Config.HeartbeatTick}フィールドの値は1に設定しましたが、この数字の単位は何なのかは説明しませんでした。これは@<code>{raft.Node.Tick}メソッドが1回呼び出されるたびにハートビートを送るという意味になっています。2つ目の@<code>{case}は@<code>{raft.Node.Ready}メソッド経由でチャネルを受け取る処理です。これは現在のノードの状態を受け取ります。これにはコミット済みのエントリ(@<code>{raft.Ready.rd.CommittedEntries})も含んでいます。このコミット済みエントリはState Machine（今回の例ではキーバリューストア）に適用できるので、先ほど実装した@<code>{*RaftAlg.publishEntries}に引数として渡してあげます。続いての実装では@<b>{raftalg}パッケージを利用するパッケージ（今回の例では@<b>{store}パッケージ）がチャネルを直で触らなくてよいようにチャネルをメソッドでラップしてあげます(@<list>{wrapChannel})。このパターンは筆者が勝手に@<b>{method wrap channel}パターンと呼んでいます。標準パッケージの@<code>{context.Done}メソッドなどはまさにこのパターンで実装されており、チャネルを直で触らなくてもよい実装になっています。

//list[wrapChannel][チャネルを関数でWrap][go]{
func (r *RaftAlg) Commit() <-chan string {
	return r.commitC
}

func (r *RaftAlg) DoneReplayWAL() <-chan struct{} {
	return r.doneRestoreLogC
}
//}

@<b>{raftalg}パッケージの最後に@<b>{store}パッケージがRaftを利用できるようにメソッドを準備してあげます。@<code>{Propose}メソッドが内部でログのコンセンサスをとってくれます。

//list[propose][チャネルを関数でWrap][go]{
func (r *RaftAlg) Propose(prop []byte) error {
	ctx, cancel := context.WithTimeout(
        context.Background(), 3*time.Second,
    )
	defer cancel()
	return r.node.Propose(ctx, prop)
}
//}

@<list>{wrapChannel}において@<code>{raft.Node.Propose}関数の結果は先ほど実装した@<code>{*RaftAlg.serveChannels}が@<code>{raft.Node.Ready}関数で受け取る形になっています。これで@<b>{raftalg}パッケージの実装が完了しました。

== キーバリューストアをraftalgパッケージでRaftに対応させる

Raftを扱う為の@<b>{raftalg}パッケージができたので、@<b>{store}パッケージを修正して、先ほど作ったキーバリューストアをRaftに対応させます。

=== 既存のキーバリューストアの改修

目指す形はもう一度@<img>{arch2}で確認しておきましょう。

コミットしたエントリがチャネル経由で通知されるので、これをgorutineで待ち構えて実際のデータを保存する場所(@<code>{map[string]string})にappendしてあげます。まずは@<code>{Store}構造体にこちらで定義した@<code>{Raft}インターフェースを埋め込むようにします(@<list>{store2})。

//list[store2][storeパッケージの修正][go]{
type Store struct {
	mu      sync.RWMutex
	kvStore map[string]string
	Raft // 追加
}

type Raft interface {
	Propose(prop []byte) error
	Commit() <-chan string
	DoneReplayWAL() <-chan struct{}
}

type kv struct {
	Key   string
	Val string
}

func New(raft Raft) *Store {
	return &Store{
		kvStore: make(map[string]string),
		Raft:    raft,
	}
}
//}

そして@<code>{*Store.Save}メソッドを修正します(@<list>{store3})。

//list[store3][Saveメソッドの修正][go]{
func (s *Store) Save(key string, value string) error {
	var buf bytes.Buffer
	err := gob.NewEncoder(&buf).Encode(kv{key, value})
	if err != nil {
		return err
	}

	err = s.Propose(buf.Bytes())
	if err != nil {
		return err
	}
	return nil
}
//}

先ほど実装した@<code>{*RaftAlg.Propose}メソッドを叩いています。ここでは標準パッケージである@<code>{encoding/gob}@<fn>{gob}パッケージを使っています。簡単に説明すると@<code>{encoding/gob}は任意の構造体をバイト配列にエンコードし、その配列をデコードして構造体に戻すことができるGo特化のエンコーダ/デコーダです。これでAPIが受け取った処理に対してRaftがコンセンサスをとってくれます。その結果を受けたり、別のノードからのエントリ追加要求を受ける仕組みが必要です。これを@<code>{*Store.RunCommitReader}メソッドとして実装しましょう(@<list>{store4})。

//footnote[gob][@<href>{https://golang.org/pkg/encoding/gob/}]

//list[store4][RunCommitReaderの実装][go]{
func (s *Store) RunCommitReader(ctx context.Context) error {
	// 起動時にWALのreplayを待つ
	select {
	case <-s.Raft.DoneReplayWAL():
	case <-ctx.Done():
		return ctx.Err()
	case <-time.After(10 * time.Second):
		return errors.New(
			"timeout(10s) receiving done replay channel",
		)
	}

	for {
		select {
		// コミット済みエントリ（適用していいエントリ）を受けて
		// map[string]stringにappendする
		case data := <-s.Raft.Commit():
			var kvdata kv
			dec := gob.NewDecoder(bytes.NewBufferString(data))
			if err := dec.Decode(&kvdata); err != nil {
				return err
			}

			s.mu.Lock()
			s.kvStore[kvdata.Key] = kvdata.Val
			s.mu.Unlock()

		case <-ctx.Done():
			return ctx.Err()
		}
	}
}
//}

最後にmain.goを修正します。まずはoptionの追加と引数に必要なものを渡すような修正を行います(@<list>{main2})。

//list[main2][main.goの修正①][go]{

import (
	// ...

	//パッケージ名は変えてください
	"github.com/******/kvraft/raftalg"
)

func main() {
	port := flag.Int("port", 3000, "key-value server port")
    cluster := flag.String(
        "cluster", "http://127.0.0.1:9021", "comma separated cluster peers",
    )
    id := flag.Int("id", 1, "node ID")
    flag.Parse()

    ra := raftalg.New(*id, strings.Split(*cluster, ","))
    s := store.New(ra)
    srv := server.New(*port, s)

    // ...
}
//}

そして最後に今まで作った関数をgorutineで走らせます(@<list>{main3})。

//list[main3][main.goの修正②][go]{
func main() {
	// 続き...

	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()

	eg, ctx := errgroup.WithContext(ctx)
	eg.Go(func() error {
		return ra.Run(ctx)
	})
	eg.Go(func() error {
		return s.RunCommitReader(ctx)
	})
	eg.Go(func() error {
		return srv.Run(ctx)
	})

	quit := make(chan os.Signal, 1)
	defer close(quit)
	signal.Notify(quit, syscall.SIGINT, syscall.SIGTERM)

	select {
	case <-quit:
		// シグナル受けたらコンテキストキャンセルして
		// 全てのgorutineを止める
		log.Println("recieved signal")
		cancel()
	case <-ctx.Done():
	}

	if err := eg.Wait(); err != nil {
		log.Println(err)
	}

	log.Println("done")
}
//}

ここまでの実装で気付きでしょうが、@<list>{main3}においてgorutineで走らせているすべての関数はcontextでキャンセル可能になっています。これにより特定のシグナルもしくはerrgroupがエラーを検知したらコンテキストキャンセルが行われ、すべてのgorutineで起動しているサブタスクをきっちり終了できます(@<img>{cancel})。当然@<code>{SIGTERM}などのシグナルを受け取ったら親が責任を持ってコンテキストキャンセルを行い終了させます。

//image[cancel][エラー発生時にエラーを親まで返して、コンテキストキャンセルを呼んでもらうことで全てのgorutineを片付ける][scale=0.9]

どのエラーがノードを終了し、どのエラーがノードの起動状態を継続するかを議論することは多くの誌面を使ってしまうので今回はこのようなシンプルなエラーハンドリングにしています。おめでとうございます。ひとまずRaftアルゴリズムを用いた分散キーバリューストアが完成しました。

=== 動作確認で理解するRaft

まずはここまでの動作確認をしましょう。ノードを一個一個立ち上げるのは面倒なのでクラスターを一発で立ち上げる為の@<b>{goreman}を入れておきましょう。

//list[try2][goreman][go]{
$ go get github.com/mattn/goreman
//}

そして@<code>{Procfile}(@<list>{try3})を書きます。

//list[try3][Procfileの作成][go]{
# Use goreman to run `go get github.com/mattn/goreman`
kvraft1: go run main.go --id 1 --cluster \
    http://127.0.0.1:12379,http://127.0.0.1:22379,http://127.0.0.1:32379 \
    --port 12380

kvraft2: go run main.go --id 2 --cluster \
    http://127.0.0.1:12379,http://127.0.0.1:22379,http://127.0.0.1:32379 \
    --port 22380

kvraft3: go run main.go --id 3 --cluster \
    http://127.0.0.1:12379,http://127.0.0.1:22379,http://127.0.0.1:32379 \
    --port 32380
//}

当然goremanを使わなくても@<list>{try3}のコマンド
それではクラスターを立ち上げましょう。まずはRaft-debug logを見たいので@<code>{grep Raft-debug}をつけて実行してみましょう(@<list>{goreman})。

//list[goreman][クラスター立ち上げ][go]{
goreman -logtime=false start | grep Raft-debug
//}

立ち上げた時点では、リーダー選挙の様子がログで見れます。その中でkvraft2のログの重要部分だけを抽出してみます(@<list>{log})。

//list[log][クラスター立ち上げ時のkvraft2のログ][go]{
2 became follower at term 1                                  // ①
2 is starting a new election at term 1                       // ②
2 became candidate at term 2                                 // ③
2 received MsgVoteResp from 2 at term 2                      // ④
2 [logterm: 1, index: 3] sent MsgVote request to 1 at term 2 // ⑤
2 [logterm: 1, index: 3] sent MsgVote request to 3 at term 2 // ⑥
2 received MsgVoteResp from 1 at term 2                      // ⑦
2 has received 2 MsgVoteResp votes and 0 vote rejections     // ⑧
2 became leader at term 2                                    // ⑨
//}

ログを見るとリーダー選挙でkvraft2が選ばれていることがわかります。リーダー選挙の項で説明した流れになっているのを確認してください(@<list>{election})。

//list[election][クラスター立ち上げ時のログの意味][go]{
① term1でfollower になる
② term1で選挙タイムアウト
③ term2でのリーダー選挙が始まり kvraft2 が候補者になる
④ kvraft2 が kvraft2 （自分自身） に投票要求
⑤ kvraft2 が kvraft1に投票要求
⑥ kvraft2 が kvraft3に投票要求
⑦ kvraft2 が kvraft1から投票を受ける
⑧ 過半数が投票された
⑨ term2でkvraft2がリーダーになる
//}

続いてキーバリューストアをAPI経由で操作してみます(@<list>{try5})。

//list[try4][Raftアルゴリズムが正しく動いていることを確認①][go]{
$ curl -X PUT localhost:12380 -d '{"key": "hello", "value": "raft"}'
# 結果
{"hello":"raft"}

$ curl -X GET localhost:12380/hello
# 結果
{"hello":"raft"}

# 先ほどとは違うエンドポイントにGETしてみる
$ curl -X GET localhost:22380/hello
# 結果
{"hello":"raft"}
//}

clusterのノードすべてにデータが正しく保存されていることがわかります。ここでkvraft2をdownさせた後にデータを更新した後、kvraft2を再起動してみます。どうなるでしょうか(@<list>{try5})。

//list[try5][Raftアルゴリズムが正しく動いていることを確認②][go]{
# kvraft2を止める
# Graceful Shutdownで少し時間かかる
$ goreman run stop kvraft2

$ curl -X PUT localhost:12380 -d '{"key": "hello", "value": "golang"}'
# 結果
{"hello":"golang"}

# kvraft2を再起動
$ goreman run start kvraft2

$ curl -X GET localhost:22380/hello
# 結果
{"hello":"golang"}

# Ctrl+Cやらでクラスターを止めたらWALのおかたずけ
$ rm -r ./kvraft-*
//}

kvraft2がdownしていた時に更新が入ったのにもかかわらず、kvraft2のノードのストアのデータも正しく更新されています。Raftで正しく分散システムが構築できています。また今回のパッケージの切り方に注目してください。@<b>{etcd/raft}パッケージの利用は@<b>{raftalg}パッケージのみに閉じ込めることに成功しており、@<b>{store}パッケージに関しては標準パッケージ以外に依存していません。もちろん@<b>{server}パッケージも@<b>{store}パッケージに依存していないのでテストやstoreなどの実装の差し替えも容易になっています。

== クラスターの動的な構成変更

今までの実装ではクラスターのノードを起動時に固定しなければなりません。たとえば、運用中にノード数を3から5にしたい時は、またクラスターごと再起動しなければいけません。これでは困るので動的にクラスターのノード数を変更できるようにしてあげましょう。ここからのコードの変更は@<code>{https://github.com/po3rin/kvraft/pull/3}に差分が置いてあります。

=== キーバリューストアを動的な構成変更に対応させる

論文にもあるとおり、Raftには構成変更に関するルールが組み込まれています。そのため、当然etcd/raftにもその実装が用意されています。まずはRaftノード起動時にこの起動はクラスターへの新規参加なのかを判別しなければいけません。そのため、@<code>{main}関数にjoinフラグを追加してあげます(@<list>{main4})。

//list[main4][joinフラグの追加][go]{
func main() {
	// ...

	// joinフラグを追加
	join := flag.Bool("join", false, "join an existing cluster")
	flag.Parse()

	// ...

	eg.Go(func() error {
		return ra.Run(ctx, *join) //　引数追加
	})

	// ...
}
//}

joinフラグの値を@<code>{*RaftAlg.Run}に渡してあげています。それでは@<code>{*RaftAlg.Run}を修正しましょう(@<list>{run4})。

//list[run4][joinフラグの追加][go]{
// 引数に追加
func (r *RaftAlg) Run(ctx context.Context, join bool) error {
	// ...

	if oldwal {
		r.node = raft.RestartNode(c)
	} else if join { // joinフラグが有効だった場合
		r.node = raft.RestartNode(c)
	} else {
		r.node = raft.StartNode(c, rpeers)
	}
}
//}

これでjoinが渡ったときにRestatとして起動するようになりました。あとはクラスターの設定を変えられるようにする必要があります。これは@<b>{raft/etcd}パッケージによって提供される@<code>{raft.RaftNode.ProposeConfChange}が担います。これに適切な命令を構造体として渡してあげるようにすればクラスターの設定（ノード数など）を変えてくれます@<list>{ChangeConf}。

//list[ChangeConf][ChangeConfメソッドの追加][go]{
func (r *RaftAlg) ChangeConf(op string, nodeID uint64, url string) error {
	var t raftpb.ConfChangeType
	switch op {
	case "add":
		t = raftpb.ConfChangeAddNode
	case "remove":
		t = raftpb.ConfChangeRemoveNode
	default:
		return fmt.Errorf("unsupported operation %v", op)
	}

	cc := raftpb.ConfChange{
		Type:    t,
		NodeID:  nodeID,
		Context: []byte(url),
	}
	ctx, cancel := context.WithTimeout(
		context.Background(), 5*time.Second,
	)
	defer cancel()
	return r.node.ProposeConfChange(ctx, cc)
}
//}

@<list>{ChangeConf}ではとりあえずノードの追加と削除をサポートしました。この関数をAPIから@<b>{store}パッケージ経由でコールできるようにすれば良さそうです。@<code>{raft.RaftNode.ProposeConfChange}の結果はチャネル経由で@<code>{[]raftpb.Entry}型として受け取れます。そのため@<code>{[]raftpb.Entry}を処理している@<code>{*RaftAlg.publishEntries}メソッドも全面的に修正してあげます(@<list>{publishEntries})。少しエラーハンドリングが多いので省略しています。

//list[publishEntries][publishEntriesの修正][go]{
func (r *RaftAlg) publishEntries(
	ctx context.Context, ents []raftpb.Entry,
) error {
	for i := range ents {
		switch ents[i].Type {
		case raftpb.EntryNormal:
			if len(ents[i].Data) == 0 {
				continue
			}
			s := string(ents[i].Data)

			select {
			case r.commitC <- s:
			case <-time.After(10 * time.Second):
				return errors.New(
					"timeout(10s) sending committed",
				)
			case <-ctx.Done():
				return ctx.Err()
			}

		// 先ほどスルーしていたaftpb.EntryConfChange型の処理
		case raftpb.EntryConfChange:
			var cc raftpb.ConfChange
			if err := cc.Unmarshal(ents[i].Data); err != nil {
				return err
			}

			r.node.ApplyConfChange(cc)
			switch cc.Type {
			case raftpb.ConfChangeAddNode:
				if len(cc.Context) > 0 {
					r.transport.AddPeer(
						types.ID(cc.NodeID),
						[]string{string(cc.Context)},
					)
				}
			case raftpb.ConfChangeRemoveNode:
				if cc.NodeID == uint64(r.id) {
					// 終了するノードが自分ならエラー返す
					return errors.New(
						"cluster removes this node",
					)
				}
				r.transport.RemovePeer(types.ID(cc.NodeID))
			}
		}
	}
	return nil
}
//}

これで@<b>{raftalg}パッケージの修正が終わりました。続いて@<b>{store}パッケージを修正します。具体的には@<code>{*RaftAlg.ChangeConf}をcallするようにします(@<list>{store5})。

//list[store5][storeパッケージの修正][go]{
type Raft interface {
	Propose(prop []byte) error
	ChangeConf(op string, id uint64, url string) error // 追加
	Commit() <-chan string
	DoneReplayWAL() <-chan struct{}
}

// Configメソッドを追加
func (s *Store) Conf(op string, id uint64, url string) error {
	return s.ChangeConf(op, id, url)
}
//}

続いて@<code>{server}パッケージでクラスタの設定を変更するエンドポイントを追加してあげます(@<list>{server3})。

//list[server3][serverパッケージの修正①][go]{
type Store interface {
	Lookup(key string) (string, bool)
	Save(k string, v string) error
	Conf(op string, id uint64, url string) error // 追加
}

// ...

func New(port int, kv Store) *http.Server {
	// ...

	// ２つ追加
	r.POST("/", h.Post)
	r.DELETE("/", h.Delete)

	// ...
}
//}

@<list>{server3}ではPOSTとDELETEを追加しています。それぞれノードの追加と削除です。最後に@<code>{*server.handler}を追加してあげます(@<list>{server4})。

//list[server4][serverパッケージの修正②][go]{

type ConfigRequest struct {
	ID    string `json:"id"`
	URL   string `json:"url"`
}

func (h *handler) Post(c *gin.Context) {
	var req ConfigRequest
	c.BindJSON(&req)

	nodeID, err := strconv.ParseUint(req.ID, 0, 64)
	if err != nil {
		c.JSON(http.StatusBadRequest, gin.H{
			"error": err.Error(),
		})
		return
	}

	err = h.store.Conf("add", nodeID, req.URL)
	if err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{
			"error": err.Error(),
		})
		return
	}
	c.JSON(http.StatusOK, gin.H{
		"add node": "ok",
	})
}

func (h *handler) Delete(c *gin.Context) {
	var req ConfigRequest
	c.BindJSON(&req)
	nodeID, err := strconv.ParseUint(req.ID, 0, 64)
	if err != nil {
		c.JSON(http.StatusBadRequest, gin.H{
			"error": err.Error(),
		})
		return
	}

	err = h.store.Conf("remove", nodeID, req.URL)
	if err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{
			"error": err.Error(),
		})
		return
	}
	c.JSON(http.StatusOK, gin.H{
		"remove node": "ok",
	})
}
//}

これで完成です。

=== クラスター構成の動的変更を動作確認

動作確認をしましょう。まずは先ほどと同様にクラスターを3ノードで立ち上げます@<list>{try6}。

//list[try6][クラスター立ち上げ][go]{
$ goreman -logtime=false start | grep Raft-debug
//}

続いてノードの追加要求を行います。

//list[try7][ノードの追加要求][go]{
$ curl -X POST localhost:12380 \
	-d '{"id": "4", "url": "http://127.0.0.1:42379"}'
//}

これでノードの設定が追加されました。クラスターがノードを探しているので、実際のノードを立ち上げてあげましょう。

//list[try8][ノードの立ち上げ][go]{
$ go run main.go --id 4 --cluster \
http://127.0.0.1:12379,http://127.0.0.1:22379,\
http://127.0.0.1:32379,http://127.0.0.1:42379 \
--port 42380 --join
//}

これでノードが4台になりました。実際にPUT、GETでデータが4台のノードに正しく入っていることを確認してください。クラスターからのノードの削除は@<list>{try9}で実行できます。

//list[try9][ノードの削除][go]{
curl -X DELETE localhost:12380 \
    -d '{"id": "4", "url": "http://127.0.0.1:42379"}'
//}

これでクラスターの動的な設定変更機能が完成しました。さらなる応用として、etcdはv3.4からRaftに「リーダー、フォロワー、候補者」に次ぐ4つ目のロールである@<b>{learner（学習者）}@<fn>{learner}を実装しています。これはノードの新規追加時にリーダーのログに追いつくまで非投票メンバーとしてクラスターに参加させるというアイデアです。この方法はログ複製でリーダーが過負荷になるのを防ぎ、クラスター再構成の安全性を高めます。詳しくはetcdのドキュメントをご覧ください@<fn>{learner}

//footnote[learner][@<href>{https://github.com/etcd-io/etcd/blob/master/Documentation/learning/design-learner.md}]

== Next Step

誌面の都合上、具体的な実装は紹介しませんが、Next StepとしてRaft論文にも紹介されている@<b>{Log compaction}をサポートする必要があります。Raftのログは際限なく大きくなっていくので、メモリを食い、replayに多くの時間がかかるなどのいろいろな問題が発生します。そのため、ログに蓄積された不要な情報を破棄する何らかのメカニズムが必要です。Raftではある一定の状態のスナップショットを取得して、それ以前のエントリを破棄することで問題を解決しています。スナップショットの機構を正しく動作させるためのRPCである@<b>{InstallSnapshot RPC}も定義されているので、もしよければ読んで見てください。当然、etcd/raftにもスナップショットの機能があり、使い方もetcd/raftのexample@<fn>{raftexample}に実装があるのでこれを参考にするとよいでしょう。

//footnote[raftexample][@<href>{https://github.com/etcd-io/etcd/tree/master/contrib/raftexample}]

== 終わりに

本章ではGoとコンセンサスアルゴリズムのRaftによる分散システムの実装方法を紹介しました。Goで分散システムを作る際にはぜひ参考にしてみてください。
