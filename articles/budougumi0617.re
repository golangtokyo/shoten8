= GoにおけるSOLIDの原則
@<tt>{@budougumi0617}@<fn>{bd617_twitter}です。
オブジェクト指向設計の原則を5つまとめた@<kw>{SOLIDの原則}@<fn>{solid}（@<i>{the SOLID principles}）という設計指針があることはみなさんご存じでしょう。
本章ではSOLIDの原則にのっとった@<tt>{Go}の実装について考えます。

//footnote[bd617_twitter][@<href>{https://twitter.com/budougumi0617}]
//footnote[solid][@<href>{https://ja.wikipedia.org/wiki/SOLID}]

== @<kw>{SOLIDの原則}（@<tt>{the SOLID principles}）とは
SOLIDの原則は次の用語リストに挙げた5つのソフトウェア設計の原則の頭文字をまとめたものです。

#@# textlint-disable
 : @<kw>{単一責任の原則}（@<kw>{SRP}, @<tt>{Single responsibility principle}）
    クラスを変更する理由は1つ以上存在してはならない。@<fn>{agile_srp}
 : @<kw>{オープン・クローズドの原則}（@<kw>{OCP}, @<tt>{Open–closed principle}）
    ソフトウェアの構成要素構成要素（クラス、モジュール、関数など）は拡張に対して開いて（オープン: Open）いて、修正に対して閉じて（クローズド: Closed）いなければならない。@<fn>{agile_ocp}
 : @<kw>{リスコフの置換原則}（@<kw>{LSP}, @<tt>{Liskov substitution principle}）
    @<tt>{S}型のオブジェクト@<tt>{o1}の各々に、対応する@<tt>{T}型のオブジェクト@<tt>{o2}が1つ存在し、@<tt>{T}を使って定義されたプログラム@<tt>{P}に対して@<tt>{o2}の代わりに@<tt>{o1}を使っても@<tt>{P}の振る舞いが変わらない場合、@<tt>{S}は@<tt>{T}の派生型であると言える。@<fn>{agile_lsp}
 : インターフェイス分離の原則（@<kw>{ISP}, @<tt>{Interface segregation principle}）
    クライアントに、クライアントが利用しないメソッドへの依存を強制してはならない。@<fn>{agile_isp}
 : @<kw>{依存関係逆転の原則}（@<kw>{DIP},  @<tt>{Dependency inversion principle}）
    上位のモジュールは下位のモジュールに依存してはならない。どちらのモジュールも「抽象」に依存すべきである。「抽象」は実装の詳細に依存してはならない。実装の詳細が「抽象」に依存すべきである。@<fn>{agile_dip}
#@# textlint-enable

//footnote[agile_srp][アジャイルソフトウェア開発の奥義 第2版 8.1より]
//footnote[agile_ocp][アジャイルソフトウェア開発の奥義 第2版 9.1より]
//footnote[agile_lsp][アジャイルソフトウェア開発の奥義 第2版 10.1より]
//footnote[agile_isp][アジャイルソフトウェア開発の奥義 第2版 12.3より]
//footnote[agile_dip][アジャイルソフトウェア開発の奥義 第2版 11.1より]

これらの原則は、ソフトウェアをより理解しやすく、より柔軟でメンテナンス性の高いものにする目的で考案されました。
各々の原則の原案者や発表時期は異なりますが、この5つの原則から頭文字のアルファベットを1つずつ取り@<tt>{SOLIDの原則}としてまとめたのが@<i>{Robert C. Martin}氏です。@<fn>{getting_a_slid_start}。
メーリングリストに投稿された@<i>{Robert C. Martin}氏のコメント@<fn>{ttc_of_oop}を見ると、1995年時点でオープンクローズドの原則（@<tt>{The open/closed principle}）などの命名がされていることがわかります。
また、SOLIDの原則として5つの原則がまとめて掲載された日本語書籍は、「アジャイルソフトウェア開発の奥義」@<fn>{amzn_agile}になります。
SOLIDの原則の成り立ち自体についてもっと知りたい場合は、@<tt>{@nazonohito51}さんの「@<tt>{「SOLIDの原則って何ですか？」って質問に答えたかった}@<fn>{whats-solid-principle}」を読んでみるとよいでしょう。

//footnote[getting_a_slid_start][@<href>{https://sites.google.com/site/unclebobconsultingllc/getting-a-solid-start}]
//footnote[ttc_of_oop][@<href>{https://groups.google.com/forum/m/#!msg/comp.object/WICPDcXAMG8/EbGa2Vt-7q0J}]
//footnote[amzn_agile][@<href>{https://www.amazon.co.jp/dp/4797347783}]
//footnote[nazonohito51][@<href>{https://twitter.com/nazonohito51}]
//footnote[whats-solid-principle][@<href>{https://speakerdeck.com/nazonohito51/whats-solid-principle}]

== Goとオブジェクト指向プログラミング
本題へ入る前に、@<tt>{Go}とオブジェクト指向プログラミングの関係を考えてみます。
そもそもオブジェクト指向に準拠したプログラミング言語であることの条件とは何でしょうか。
さまざまな主張はありますが、本章では次の3大要素を備えることが「オブジェクト指向に準拠したプログラミング言語であること」とします。

 1. カプセル化（@<i>{Encapsulation}）
 2. 多態性（ポリモフィズム）（@<i>{Polymorphism}）
 3. 継承（@<i>{Inheritance})


#@# textlint-disable
では@<tt>{Go}はオブジェクト指向言語なのでしょうか。@<tt>{Go}公式サイトには@<kw>{Frequently Asked Questions (FAQ)}@<fn>{q_and_a}という「よくある質問と答え」ページがあります。
この中の@<i>{Is Go an object-oriented language?}（@<tt>{Go}はオブジェクト指向言語ですか？）という質問に対する答えとして、次の公式見解が記載されています。
#@# textlint-enable

//quote{
Yes and no. Although Go has types and methods and allows an object-oriented style of programming, there is no type hierarchy. The concept of “interface” in Go provides a different approach that we believe is easy to use and in some ways more general. There are also ways to embed types in other types to provide something analogous—but not identical—to subclassing. Moreover, methods in Go are more general than in C++ or Java: they can be defined for any sort of data, even built-in types such as plain, “unboxed” integers. They are not restricted to structs (classes).

Also, the lack of a type hierarchy makes “objects” in Go feel much more lightweight than in languages such as C++ or Java.
//}

//footnote[q_and_a][@<href>{https://golang.org/doc/faq#Is_Go_an_object-oriented_language}]

あいまいな回答にはなっていますが、「Yesであり、Noでもある。」という回答です。
Goはオブジェクト指向の3大要素を一部しか取り入れていないため、このような回答になっています。

==={no_subclassing} @<tt>{Go}はサブクラシング（@<tt>{subclassing}）に対応していない
多くの方がオブジェクト指向言語に期待する仕組みの1つとして、先ほど引用した回答内にもある@<kw>{サブクラシング}（@<i>{subclassing}）が挙げられるでしょう。
もっと平易な言葉で言い直すと、@<tt>{クラス（型）の階層構造（親子関係）による継承}です。
代表的なオブジェクト指向言語である@<tt>{Java}でサブクラシングの例を書いたコードが@<list>{person}です。
@<list>{person}は、親となる@<code>{Person}クラスと子となる@<code>{Japanese}クラスの定義と、@<code>{Person}クラスを引数に取るメソッドを含んでいます。

//list[person][Javaで表現されたPersonクラスを継承するJapaneseクラス][c#]{
class Person {
  String name;
  int age;
}

// Personクラスを継承したJapaneseクラス
class Japanese extends Person {
  int myNumber;
}

class Main {
  // Personクラスを引数にとるメソッド
  public static void Hello(Person p) {
    System.out.println("Hello " + p.name);
  }
}
//}

@<code>{Person}クラスを継承した@<code>{Japanese}クラスのオブジェクトは、ポリモフィズムによって@<code>{Person}変数に代入できます。
また、同様に@<code>{Person}クラスのオブジェクトを引数にとるメソッドに対して代入することもできます（@<list>{person2}）。

//list[person2][Javaにおけるクラス継承を利用したポリモフィズムな代入と呼び出し]{
Japanese japanese = new Japanese();
person.name = "budougumi0617";
Person person = japanese;
Main.Hello(japanese);
//}

このようなポリモフィズムを目的とした継承関係を表現するとき、@<tt>{Go}ではインターフェースを使うでしょう。
しかし、@<tt>{Go}は@<list>{person}のような具象クラス（あるいは抽象クラス）を親とするようなサブクラシングによる継承の仕組みを言語仕様としてサポートしていません。

@<tt>{Go}では埋込み（@<i>{Embedding}@<fn>{embedding}）を使って別の型に実装を埋め込むアプローチもあります。
しかし、これは多態性や共変性・反変性@<fn>{convariance}を満たしません。
よって、埋め込みはオブジェクト指向で期待される継承ではなくコンポジションにすぎません。
@<list>{go_person}は@<list>{person}を@<tt>{Go}で書き直したものです。
@<list>{go_person}の例では、@<code>{Japanese}型のオブジェクトは@<code>{Hello}関数に利用することはできません。

//list[go_person][Goで@<list>{person}のような親子関係を表現する場合]{
type Person struct {
  Name string
  Age  int
}

// Personを埋め込んだJapanese型。
type Japanese struct {
  Person
  MyNumber int
}

func Hello(p Person) {
  fmt.Println("Hello " + p.Name)
}
//}
以上の例以外にも、@<kw>{リスコフの置換原則}などの一部のSOLIDの原則はそのまま@<tt>{Go}に適用することはできません。
しかし、SOLIDの原則のベースとなる考えを取り入れることでよりシンプルで可用性の高い@<tt>{Go}のコードを書くことは可能です。

それでは、次節より@<tt>{Go}のコードにSOLIDの原則を適用していくとどうなっていくか見ていきます。
なお、@<tt>{Go}とSOLIDの原則については、@<i>{Dave Cheney}氏も2016年に@<tt>{GolangUK}で@<tt>{SOLID Go Design}というタイトルで発表されています。

//footnote[dave_solid][@<href>{https://dave.cheney.net/2016/08/20/solid-go-design}]

//footnote[convariance][@<href>{https://docs.microsoft.com/ja-jp/dotnet/csharp/programming-guide/concepts/covariance-contravariance/}]
//footnote[embedding][@<href>{https://golang.org/doc/effective_go.html#embedding}]

===[column] 実装よりもコンポジションを選ぶ
あるクラスが他の具象クラス（抽象クラス）を拡張した場合の継承を、@<tt>{Java}の世界では@<kw>{実装継承}（@<i>{implemebtation inheritance}）と呼びます。
あるクラスがインターフェースを実装した場合や、インターフェースが他のインターフェースを拡張した場合の継承を@<kw>{インターフェース継承}（@<i>{interface inheritance}）と呼びます。
@<tt>{Go}がサポートしている継承は@<tt>{Java}の言い方を借りるならば@<kw>{インターフェース継承}のみです。
クラスの親子関係による@<kw>{実装継承}はカプセル化を破壊する危険も大きく、深い継承構造はクラスの構成把握を困難にするという欠点もあります。
このことは代表的なオブジェクト指向言語である@<kw>{Java}の名著、「@<kw>{Effective Java}」の「@<kw>{項目16 継承よりコンポジションを選ぶ}」でも言及されています（筆者が所有している@<kw>{Effective Java}は第2版ですが、2018年に@<tt>{Java 9}対応の@<kw>{Effective Java 第3版}が発売されています）。
@<tt>{Go}が@<kw>{実装継承}をサポートしなかった理由は明らかになっていませんが、筆者は以上の危険性があるためサポートされていないと考えています。

===[/column]

== @<kw>{単一責任の原則}（@<kw>{SRP}, @<tt>{Single responsibility principle}）
//quote{
クラスを変更する理由は1つ以上存在してはならない。
//}

ここでは@<tt>{アジャイルソフトウェア開発の奥義}の原文どおりに「クラスを…」とそのまま引用しましたが、
@<kw>{単一責任の原則}はさまざまな大小の粒度で考えなければいけない問題です。
関数・メソッド単位の粒度ならば比較的考えやすそうです。
ドメインやパッケージの単位の単一責任になってくると解釈が一概にいえるものではなくなります。
同じ事象に対して見方によって複数の分割方法が生まれるでしょう。

=== @<tt>{単一責任の法則}とパッケージ名
@<tt>{Effective Go}の@<tt>{Package names}@<fn>{effective_pkg_name}では@<tt>{Go}のパッケージ名は短い1単語を推奨しています。
複数の単語をアンダースコアでつなげたりするような名前を推奨していません。
このポリシーを言い換えると、ある1つの英単語に象徴される機能のみに凝集したパッケージを提供すべきであるということです。
@<tt>{SRP}と@<tt>{Go}の言語思想である単純さが求める姿が近いといえるのではないでしょうか。
実際に、@<tt>{Go}の標準パッケージの命名付を確認@<fn>{go_pkgs}すると、そのほとんどが1単語のパッケージです。

//footnote[effective_pkg_name][@<href>{https://golang.org/doc/effective_go.html#package-names}]
//footnote[go_pkgs][@<href>{https://golang.org/pkg/}]

変更理由が1つしか存在しないということは、その型の責務（責任）が1つであることを意味します。
では、「この型の役割が1つである」と判断するにはどうすればよいのでしょうか。
たとえば、「コンロ」をモデリングするとき、コンロには「火を付ける」機能と「火を消す」機能の2つが与えるでしょう。
「ネットワーク」に関する機能をモデリングするときは「送信」機能と「受信」機能が必要になるでしょう。
これらは「役割が2つある」状態でしょうか。

@<tt>{アジャイルソフトウェア開発の奥義}では、この判断基準を@<tt>{アプリケーションが今後どのように変更されるかどうか次第}としています。
「それでは何も答えになっていないじゃないか」と思われる方もいるでしょう。
ですが、「どこまでがこれ以上分解できない単位なのか」はそのシステム全体に依存します。
「スマートフォン」モデルは単一責任として「電話」機能を抽出しておくべきでしょうか。
それは「スマートフォン」がそのシステムでどのように扱われるのかによるでしょう。
@<tt>{不必要な複雑さ}は避けるべきです。
同書籍には@<tt>{変更の理由が変更の理由たるのは、実際に変更の理由が生じた場合だけである}と述べられています。
はやすぎる抽象化は@<tt>{不必要な複雑さ}を生み出します。

=== インターフェースを使った単一責任への依存
型やパッケージを凝集するのは設計の永遠の課題でしょう。
それと比較して、@<tt>{Go}の場合インターフェースを経由して@<tt>{対象の型の特定の役割のみを利用する}ことは簡単です@<fn>{link_isp}。
@<tt>{Go}は具象型にインターフェース名を記載しない暗黙的インターフェース実装を採用しているため、利用者側でインターフェースを定義できます。
利用者がある型に対して特定の責務のみを単一責任として利用したい場合、@<list>{depend_only_recv}のようにインターフェースを定義できます。
このようなインターフェース定義は@<tt>{Effective Go}でも推奨@<fn>{interface_name}されています。
@<tt>{-er}という1つのメソッド定義のみ所有するインターフェースを作るのは、@<tt>{Go}の中ではポピュラーなプラクティスです。
//footnote[link_isp][インターフェースについては、@<hd>{isp}でも触れます。]
//footnote[interface_name][@<href>{https://golang.org/doc/effective_go.html#interface-names}]

//list[depend_only_recv][@<tt>{Modem}型の@<tt>{Recv}責務のみに注目したインターフェース定義]{
type Modem struct {}
func (Modem) Dial() {}
func (Modem) Hangup() {}
func (Modem) Sender() {}
func (Modem) Recv() {}

// Recvメソッドのみに注目したインターフェース定義
type Receiver interface {
	Recv()
}
//}

=== @<tt>{単一責任の原則}と@<tt>{Go}
@<tt>{SRP}は一番純な原則です。
しかし、実際にコードや問題に向き合ったときに「ある役割の塊」や「単一責任」を定義するのは難しいです。
これはドメイン駆動開発やマイクロサービスにおける「境界」探しの難しさにも通じています。
同じ処理（ドメイン）でもフレームワーク（組織構造）などの環境やそのときの状況によって「単一責任」の定義が変わるでしょう。
@<tt>{SRP}を考えることは設計の本質です。
正解はないからこそ、経験やほかの原則を駆使して「単一責任」の定義に立ち向かう必要があります。
@<tt>{UNIXという考えかた}@<fn>{unix}でもある「ひとつのことをうまくやれ」も@<tt>{SRP}と同様の思想です。
「ひとつのこと」「ドメイン」など様々な文脈の「単一責任」を学ぶことが@<tt>{SRP}を実践していくために必要です。
@<tt>{Go}における@<tt>{SRP}も、標準パッケージの構成などから学ぶことができるでしょう。



//footnote[unix][@<href>{https://www.amazon.co.jp/dp/4274064069}]

== @<kw>{オープン・クローズドの原則}（@<kw>{OCP}, @<tt>{Open–closed principle}）
//quote{
ソフトウェアの構成要素構成要素（クラス、モジュール、関数など）は拡張に対して開いて（オープン: Oepn）いて、修正に対して閉じて（クローズド: Closed）いなければならない。
//}

たいていのソースコードは完成後に（あるいは作成中でも）機能追加・仕様の変更・あるいは仕様漏れの対応で機能の修正あるいは拡張が発生します。
このとき「硬い」設計のソフトウェアは変更に対して大きな修正・そして副作用が発生します。
変更・拡張に対して柔軟である一方で、修正の副作用に対して強固であるべきです。
それを表す原則が@<kw>{オープン・クローズドの原則}（@<tt>{OCP}）です。

=== @<tt>{コンポジション}で体感する@<tt>{OCP}
@<tt>{Go}で@<tt>{OCP}を体感するのは、まず埋込み型によるコンポジションです。
利用者側がインターフェース越しに構造体を利用していた場合、@<list>{ocp_decorator}のように拡張できます。
コンポジションされている@<tt>{Item}構造体には何も修正をしていないので、変更に対して閉じているといえます。

//list[ocp_decorator][@<tt>{Decorator}パターンで@<tt>{User}を拡張する]{
type Item struct {
  name  string
  price int
}

func (i *Item) Price() int { return i.price }

func (i *Item) SetPrice(p int) { i.price = p }

type ItemWithTax struct {
  Item
}

func (i *ItemWithTax) Price() int {
  return int(float64(i.Item.Price()) * 1.1)
}
//}

=== @<tt>{switch}文で体感する@<tt>{OCP}
ロジックの中でも@<tt>{OCP}の考えを適用できます。
@<list>{before_ocp}は（あまり意味のない関数ですが、）受け取った演算子を使って計算をする@<tt>{Operation}関数です。
@<tt>{Operation}関数は@<tt>{OCP}を満たしていません。
新しく@<tt>{"*"}演算子に対応しようと思ったときに@<tt>{switch}文の修正が必要になるからです。
これは、修正に対して閉じているとはいえません。

//list[before_ocp][修正に対して閉じていない@<tt>{Operation}関数]{
func Operation(x, y int, op string) int {
  var r int
  switch op {
  case "+":
    r = x + y
  case "-":
    r = x - y
  default:
    // 異常系は省略
  }
  return r
}
//}

@<list>{after_ocp}が@<tt>{OCP}の考えに基づいて書き直した@<tt>{Operation}関数です。
@<tt>{Operation}関数は修正に対して閉じているので、新しい演算子を追加しても修正が発生しません。
@<tt>{ops}変数のマップに新しい演算子と計算関数を追加できるので、拡張にも開かれています。

//list[after_ocp][@<tt>{OCP}に対応させた@<tt>{Operation}関数]{
var ops = map[string]func(int, int) int{
  "+": func(x int, y int) int {
    return x + y
  },
  "-": func(x int, y int) int {
    return x - y
  },
}

func Operation(x, y int, op string) int {
  if f, ok := ops[op]; ok {
    return f(x, y)
  }
  return 0 // 異常系は省略
}
//}

=== @<tt>{不必要な複雑さ}には注意すること
@<tt>{OCP}を守るとコードは拡張しやすく、修正の影響を受けない堅牢な設計をできます。
しかし最初からすべての変更を考慮することはできません。
また、考慮しても実際に変更がおきず「不発」に終わってしまうこともあります。
前節で修正した@<list>{after_ocp}のコードは@<tt>{OCP}に準拠しています。しかし、あのような変更でよかったのでしょうか。
演算子の数はたかだか数個です。
あのような変更をしなくても、素朴な実装だった修正前の@<list>{before_ocp}のほうがシンプルでわかりやすかったという見方もできます。

=== @<tt>{オープン・クローズドの原則}と@<tt>{Go}
過剰な@<tt>{OCP}の適用は@<tt>{不必要な複雑さ}を生み出します。
これは@<tt>{Go}の思想のベースにある@<tt>{単純さ}とも離れてしまいます。
@<tt>{アジャイルソフトウェア開発の奥義}の@<tt>{9.4.5}節では「最初は変更が起きないこと」を前提に設計するように述べられています。
そして、実際に変更が発生したときに@<tt>{OCP}に沿った修正をすべきと続いています@<fn>{accerate_change}。

//footnote[accerate_change][同時にテストファーストや、短いサイクルの開発とデモを行い早期の変化の発見を促すことも助言しています。]

== @<kw>{リスコフの置換原則}（@<kw>{LSP}, @<tt>{Liskov substitution principle}）
//quote{
@<tt>{S}型のオブジェクト@<tt>{o1}の各々に、対応する@<tt>{T}型のオブジェクト@<tt>{o2}が1つ存在し、@<tt>{T}を使って定義されたプログラム@<tt>{P}に対して@<tt>{o2}の代わりに@<tt>{o1}を使っても@<tt>{P}の振る舞いが変わらない場合、@<tt>{S}は@<tt>{T}の派生型であると言える。
//}

@<hd>{no_subclassing}で述べたとおり、@<tt>{Go}はクラスの親子関係を使った継承をサポートしていません。
@<tt>{Go}がサポートしているインターフェースによる継承関係も、@<code>{interface}と@<tt>{struct}を置換することは当然できません。
よって、クラスの親子関係による継承（サブクラシング）について言及している@<tt>{LSP}を@<tt>{Go}に直接適用することはできません。
しかし、@<tt>{LSP}はその原則のベースに@<kw>{振る舞い}と@<kw>{契約による設計}という哲学があります。
この哲学を@<tt>{Go}のコードに取り入れることは可能であり、よりよいコードを書くために役立つでしょう。

=== @<tt>{サブクラシング}による継承と@<tt>{振る舞い}
（@<tt>{Go}の言語仕様にはありませんが、）クラスの親子関係で継承関係（@<tt>{サブクラシング}）をもたせるときを考えてみます。
子クラスは親クラスの機能や構造を子クラス自身に定義されているかのように振る舞うことができます。
場合によっては@<tt>{オーバーライド}によって振る舞いを変更することも可能です。
子クラスにとってメリットしかないように聞こえる継承（サブクラシング）ですが、本来@<tt>{子クラスは親クラスに期待される振る舞いを守る}義務があります。

振る舞いを守る義務の代表的な例として挙げられる@<fn>{rectangle}のは長方形と正方形の例です。
正方形クラスはその図形としての性質上@<tt>{横の長さ}の値が変更されたとき@<tt>{縦の長さ}も同じ値に変更する必要があります。
長方形クラスを継承した子クラスとして正方形クラスを定義したとき、正方形クラスは長方形クラスとしても扱われます。
長方形クラスの利用者が@<tt>{長方形クラスのオブジェクトの横の長さを変更しても、縦の長さは変更されない}ことを期待していた場合、正方形クラスのオブジェクトはシステムを破綻させます。

//footnote[rectangle][@<tt>{アジャイルソフトウェア開発の奥義}でもこの例が記載されています。]

もちろんこれは長方形クラスの利用者がどのような期待を長方形クラスにしているかによります。
@<tt>{アジャイルソフトウェア開発の奥義}の@<tt>{10.3.2}節、@<tt>{10.3.3}節では次のように述べられています。

#@# textlint-disable
//quote{
実際、無理にそのすべてに対応しようとすれば、「不必要な複雑さ」をシステムに織り込んでしまうのがオチである。
したがって、他の原則と同様、明らかに@<tt>{LSP}に違反しているものを優先的に改善し、その他のものに関しては「もろさ」が表面化するまで保留するという方法が有効だろう。
//}

//quote{
オブジェクト指向設計（@<tt>{OOD}）において@<tt>{LSP}の果たす役割は、誰もが合理的に仮定し、
クライアントが安心して依存するこのできる「振る舞い」を「@<tt>{IS-A}の関係」に持たせることなのだ。
//}
#@# textlint-enable

同書籍では子クラスに求められる親クラスの@<tt>{振る舞い}として@<kw>{契約による設計}が紹介されています。
@<tt>{Go}にはクラスの親子関係による継承（サブクラシング）はありませんが、インターフェースを利用した継承があります。
@<tt>{契約による設計}の哲学は@<tt>{Go}で継承関係を設計する上でも有用な知識です。

=== @<tt>{契約による設計}
@<tt>{契約による設計}（@<kw>{DBC}, @<tt>{Design By Controct}）は@<i>{Bertrand Meyer}氏による@<tt>{オブジェクト指向入門}@<fn>{ooi}で提唱されたテクニックです。
@<tt>{契約による設計}はシステムの利用者・ユーザーではなく@<fn>{contract_in_library}、その型、関数を利用するシステムの開発者に対して事前条件、事後条件、不変条件などを明示することです。
契約は開発者に求める制約・条件なのでユーザー入力などの検証に使われる条件ではありません。
そのため、実装で契約を表現する場合はコンパイルオプションなどでリリース物からは無効化します。
@<tt>{C++}などでは@<tt>{assert}関数を使って表現します。
@<tt>{C#}では@<tt>{契約プログラミング}として事前条件、事後条件、およびオブジェクト不変条件をコードで指定できます @<fn>{csharp_contract}。
@<tt>{Go}の場合@<tt>{assert}関数に相当する機能はないため、コードコメントによって表現することになります。

//footnote[ooi][@<href>{https://www.amazon.co.jp/dp/4798111112}]
//footnote[contract_in_library][ライブラリに関して言えば、ライブラリ利用者（開発者）に対して契約を求めることになります。]
//footnote[csharp_contract][@<href>{https://docs.microsoft.com/ja-jp/dotnet/framework/debug-trace-profile/code-contracts}]

=== @<tt>{Go}の標準インターフェースから読み解く@<tt>{契約による設計}
たとえば、@<code>{io.Reader}インターフェース@<fn>{io_reader}に記載されているコメントを見てみましょう。
次の引用は@<code>{io.Reader}インターフェースのコメントの一部です。

//footnote[io_reader][@<href>{https://golang.org/pkg/io/#Reader}]

#@# textlint-disable
//quote{
Reader is the interface that wraps the basic Read method.

Read reads up to len(p) bytes into p. It returns the number of bytes read (0 <= n <= len(p)) and any error encountered. Even if Read returns n < len(p), it may use all of p as scratch space during the call. If some data is available but not len(p) bytes, Read conventionally returns what is available instead of waiting for more.

...
//}
#@# textlint-enable

コメントにはインタフェースを実装する際に守るべき@<tt>{振る舞い}や事後条件が記載されています。
コメントに記載があることと違う動き・事後状態になるならば、実装が契約を守っていないこと（実装側の不備）になります。
事前条件を満たさないままインターフェースを操作していたならば、利用者側の問題と判断できます。

=== @<tt>{リスコフの置換原則}と@<tt>{Go}
@<tt>{LSP}をそのまま@<tt>{Go}のコードに適用はできません。
しかし、@<tt>{LSP}の哲学は@<tt>{Go}の標準パッケージの中にも垣間見えます。
そして我々が@<tt>{Go}を使って設計・実装する上でも取り込むべき原則の1つです。
インターフェースを定義するときは実装に期待する振る舞いを明記しましょう。
また、インターフェースを利用するときは事前条件や事後条件を確認し、正しい利用方法を確認しましょう。

=={isp} インタフェース分離の原則（@<kw>{ISP}, @<tt>{Interface segregation principle}）
//quote{
クライアントに、クライアントが利用しないメソッドへの依存を強制してはならない。
//}

@<tt>{Go}は@<code>{ISP}に強く影響を受けている言語といえます。
大きなインターフェース、複数のメソッドに依存していると他者の変更の影響を受ける可能性が高くなります。
依存する対象が少なければ少ないほど、凝集性が高く疎結合な設計ができたといえるでしょう。
@<tt>{Go}のプラクティスのひとつに@<tt>{インターフェースは可能な限り小さく作る}というものがあります。
標準パッケージの@<code>{io}パッケージ@<fn>{io_pkg}に含まれるインターフェースを見てみましょう。
@<list>{io_reader}は@<code>{io}パッケージに含まれるインターフェースの定義の一部です。

//footnote[io_pkg][@<href>{https://golang.org/pkg/io}]

//list[io_reader][@<code>{io}パッケージのインターフェース]{
type Reader interface {
    Read(p []byte) (n int, err error)
}

type Closer interface {
    Close() error
}

type ReadCloser interface {
    Reader
    Closer
}

type ReadWriteSeeker interface {
    Reader
    Writer
    Seeker
}
//}

ベースとなる@<code>{io.Reader}インターフェースや@<code>{io.Closer}インターフェースは1つのメソッド定義しかないシンプルなインターフェースです。
2つを合成した入力ストリーム操作用の@<code>{io.ReadCloser}インターフェースなども定義されています。
@<code>{Read}操作と@<code>{Close}操作は入力ストリーム処理に対する必要最小限の機能のため、@<code>{io.ReadCloser}インターフェースも@<tt>{単一責任の原則}も満たしている最小限の依存を提供になります。

また、@<tt>{Go}は具象型にインターフェース名を記載しない暗黙的インターフェース実装を行うため、@<tt>{ISP}を考慮した設計が得意です。
@<list>{sql_db}は@<code>{database/sql.DB}型の定義@<fn>{doc_sql_db}を抜粋したものです。

//footnote[doc_sql_db][@<href>{https://golang.org/pkg/database/sql/#DB}]

//list[sql_db][title]{
type DB struct{
  func (db *DB) Begin() (*Tx, error)
  func (db *DB) Conn(ctx context.Context) (*Conn, error)
  func (db *DB) Exec(query string, args ...interface{}) (Result, error)
  func (db *DB) Ping() error
  func (db *DB) Query(query string, args ...interface{}) (*Rows, error)
  // more methods...
}
//}

依存を最小にするため、インターフェースを経由して@<code>{database/sql.DB}の@<code>{Query}メソッドを扱いたいとします。
@<code>{database/sql.DB}型には多くのメソッドが実装されていますが、必要なメソッドはひとつだけです。
ここで、@<tt>{Go}は標準パッケージだとしても自分で定義したインターフェースを使って利用できます。
よって@<code>{Query}メソッドしか利用しないならば、@<list>{gueryer}のように@<code>{Query}メソッドのみもつ@<code>{Queryer}インターフェースを定義すれば依存を最小限にできます。


//list[gueryer][@<code>{Query}メソッドのみに依存して@<code>{sql.DB}を利用する]{
type Querer interface {
  Query(query string, args ...interface{}) (*Rows, error)
}

func GetAllUsers(q Querer) ([]User, error) {
  rows, err :=  q.Query("SELECT id, name, ....")
  if err != nil {
    //...
}

func main() {
   db, _ := sql.Open("mysql", "...")
   users, err := GetAllUsers(db)
}
//}

こうしておくことで、@<code>{sql.DB}型のほかのメソッドが変更されたときでも
@<code>{GetAllUsers}関数はその影響を受けることなく@<code>{sql.DB}型を利用できます。

=== @<tt>{インタフェース分離の原則}と@<tt>{Go}
以上のような設計方針が言語思想の中に存在し、@<tt>{Go}で@<kw>{ISP}は守りやすいです。
3rdパーティ製ライブラリなどもこの方針に合わせて実装されている場合が多いです。 
@<tt>{Go}の世界では、次の2点を厳守できていれば@<kw>{ISP}を守ったよいコードになるでしょう。

 * 自分が使わないメソッドへの依存を避ける
 * インターフェースは呼び出し元が都合の最小の設計にすること


//footnote[change_db][@<tt>{Go}は1.X系の間は破壊的変更は入らないので、実際にAPIインターフェースが変わることはないです。]

== @<kw>{依存関係逆転の原則}（@<kw>{DIP}, @<tt>{Dependency inversion principle}）
//quote{
上位のモジュールは下位のモジュールに依存してはならない。どちらのモジュールも「抽象」に依存すべきである。「抽象」は実装の詳細に依存してはならない。実装の詳細が「抽象」に依存すべきである。
//}

ここまで見てきたとおり、拡張性が高く副作用に強いソフトウェアを実現するための鍵は構造化と適切な境界定義です。
対象を型あるいはパッケージとして構造化し、それぞれの境界を疎結合にすることで柔軟な設計を実現できます。
型同士、またはパッケージ同士を疎結合にするための考え方が@<kw>{依存関係逆転の原則}です。

@<tt>{Go}は具象型にインターフェース名を記載しない暗黙的インターフェース実装のみをサポートしています。
そのため、実装の詳細を利用する利用者側がインターフェースを定義します。
逆にいうと、実装の詳細側はインターフェース定義を参照する必要がない（自身がどのように抽象化されているか利用されているか知らない）状態です。
しかし、@<tt>{Go}の中にも特定のインターフェースを経由して利用されることを想定した実装（抽象に依存した実装）も存在します。
たとえば、次節に挙げる@<code>{database/sql/driver}パッケージのインターフェースと各データベースドライバパッケージの実装です。

=== @<code>{database/sql/driver}パッケージと@<kw>{DIP}
@<tt>{Go}の標準パッケージでは、@<code>{database/sql/driver}パッケージが@<kw>{DIP}を利用した典型的な設計です。
通常、@<tt>{Go}から@<tt>{MySQL}などの@<tt>{RDBMS}を操作する際は@<code>{database/sql}パッケージを介した操作をします。
この@<code>{database/sql}パッケージに各ベンダー、OSSの個別仕様に対応する具体的な実装は含まれていません。
では、どのように@<tt>{MySQL}や@<tt>{PostgreSQL}を操作するかというと
@<list>{import_mysql}のように各@<tt>{RDBMS}に対応したドライバパッケージを@<tt>{import}します。
@<code>{github.com/go-sql-driver/mysql}のようなドライバパッケージは@<code>{database/sql/driver.Driver}インターフェースなどを実装しています。

//list[import_mysql][@<tt>{Go}でMySQLを操作する際の@<code>{import}文]{
import (
  "database/sql"
  _ "github.com/go-sql-driver/mysql"
)
//}

これはRDBMSごとの@<kw>{実装の詳細}が上位概念から提供されている@<kw>{インターフェースに依存}している状態です。
（ほぼありえないでしょうが、）もし@<code>{database/sql/driver}パッケージのインターフェースが変更された場合、すべてのドライバパッケージがインターフェースの変更に追従を迫られるでしょう。
このほかにも、Webアプリケーションの各ミドルウェアは@<code>{net/http}パッケージの@<code>{http/HandlerFunc}型@<fn>{godoc_handlerfunc}に合わせた実装になっています。これも@<tt>{DIP}に沿った設計方針といえます。
このような下位の実装の詳細が上位概念（@<code>{database/sql/driver}パッケージ）の抽象へ依存している関係を@<kw>{依存関係逆転の原則}と呼びます。

//footnote[godoc_handlerfunc][@<href>{https://golang.org/pkg/net/http/#HandlerFunc}]

=== @<kw>{DIP}に準拠した実装
では、実際の我々の設計や書く実装コードで@<tt>{DIP}の考えを活かすとどのようになるのでしょうか。
よく耳にする実装パターンとしては、依存性の注入（@<kw>{DI}, @<kw>{Dependency Injection}）パターン@<fn>{wiki_di}やリポジトリパターン@<fn>{repository}でしょう。
リポジトリパターンは永続化データへのアクセスに関する@<tt>{DI}と呼べるでしょう。


//footnote[wiki_di][@<href>{https://en.wikipedia.org/wiki/Dependency_injection}]
//footnote[repository][@<href>{https://martinfowler.com/eaaCatalog/repository.html}]

==== 依存性の注入（@<kw>{Dependency Injection}）
@<kw>{依存性の注入}（@<kw>{DI}）は@<kw>{DIP}を実施するためのオーソドックスな手段です。
@<tt>{Java}や@<tt>{C#}などにはクラスのフィールド定義にアノテーションをつけるだけでオブジェクト（下位モジュールの詳細）をセット（注入）してくれるような、フレームワークが提供するデファクトな仕組みが存在します。
@<tt>{Go}の場合はインターフェースで抽象を定義し、初期化時などに具体的な実装の詳細オブジェクトを設定することが大半です。

代表的な@<kw>{DI}の実装としては、次のような実装パターンがあります。

 * オブジェクト初期化時に@<kw>{DI}する方法
 * @<code>{setter}を用意しておいて、@<kw>{DI}する方法
 * メソッド（関数）呼び出し時に@<kw>{DI}する方法

@<list>{di_const}はほかの言語ではコンストラクタインジェクションと呼ばれる手法です。
上位階層のオブジェクトを初期化する際に@<tt>{DI}を実行します。
こちらだけ覚えておくだけでも十分に役立つでしょう。

//list[di_const][オブジェクト初期化時に@<kw>{DI}する方法]{
// 実装の詳細
type ServiceImpl struct{}

func (s *ServiceImpl) Apply(id int) error { return nil }

// 上位階層が定義する抽象
type OrderService interface {
  Apply(int) error
}

// 上位階層の利用者側の型
type Application struct {
  os OrderService
}

// 他言語のコンストラクタインジェクションに相当する実装
func NewApplication(os OrderService) *Application {
  return &Application{os: os}
}

func (app *Application) Apply(id int) error {
  return app.os.Apply(id)
}

func main() {
  app := NewApplication(&ServiceImpl{})
  app.Apply(19)
}
//}

@<list>{di_setter}は@<code>{setter}メソッドを用意しておくことで、初期化と実処理の間に依存性を注入する方法です。

//list[di_setter][@<code>{setter}を用意しておいて、@<kw>{DI}する方法]{
func (app *Application) Apply(id int) error {
  return app.os.Apply(id)
}


func (app *Application) SetService(os OrderService) {
  app.os = os
}

func main() {
  app := &Application{}
  app.Set(&ServiceImpl{})
  app.Apply(19)
}
//}

@<list>{di_method}はメソッド（関数）の引数として依存を渡す方法です。上位階層のオブジェクトのライフサイクルと、実装の詳細のオブジェクトの生成タイミングが異なるときはこの手法を取ります。
//list[di_method][メソッド（関数）呼び出し時に@<kw>{DI}する方法]{
func (app *Application) Apply(os OrderService, id int) error {
  return os.Apply(id)
}

func main() {
  app := &Application{}
  app.Apply(&ServiceImpl{}, 19)
}
//}

以上のような@<tt>{DI}は他の言語でもみられる共通手法のような実装です。
このほかにも@<tt>{Go}では次のような実装で@<tt>{DIP}を満たすことも可能です。


=== 埋込み型を利用した@<tt>{DIP}
@<tt>{Go}は構造体の中に別の構造体やインターフェースを埋め込めます。
インターフェースを埋め込むことで、抽象に依存した型を定義できます。
インターフェースのメソッドが呼び出されるまでに何らかの方法で実装への依存を注入することで、実装の詳細が呼ばれます。

//list[label][title]{
type OrderService interface {
  Apply(int) error
}

type ServiceImpl struct{}

func (s *ServiceImpl) Apply(id int) error { return nil }

type Application struct {
  OrderService // 埋め込みinterface
}

func (app *Application) Run(id int) error {
  return app.Apply(id)
}

func main() {
  // 初期化時の宣言はオブジェクト初期化時にDIする方法と変わらない。
  app := &Application{OrderService: &ServiceImpl{}}
  app.Run(19)
}
//}

=== @<tt>{interface}を利用しない@<tt>{DIP}
@<tt>{DIP}ではクラスの親子関係やインターフェースの継承関係を利用することが多いです。
しかし、構造体を定義せずに関数型を用意するだけでも実現が可能です。
@<list>{di_func}は@<code>{Application}構造体に@<code>{func(int) error}型の@<code>{Apply}フィールドを定義しています。
@<code>{Apply}フィールドは実装に依存しない抽象です。
@<code>{Apply}フィールドに実行時に関数の実装を注入することで@<tt>{DIP}を実現しています。

//list[di_func][関数型を利用した@<tt>{DI}]{
func CutomApply(id int) error { return nil }

type Application struct {
  Apply func(int) error
}

func (app *Application) Run(id int) error {
  return app.Apply(id)
}

func main() {
  app := &Application{Apply: CutomApply}
  app.Run(19)
}
//}

=== 高度なツールやフレームワークを使った@<tt>{DIP}
@<tt>{Go}は@<kw>{単純}であることが言語思想@<fn>{simplicity}にあるため、（ソースコードに書いてある以上の挙動を裏で実行するような）高度な@<kw>{DI}ツールはあまり使われていない印象です。
@<kw>{DI}用のコードを自動生成する@<tt>{google/wire}フレームワーク@<fn>{wire}も存在しますが、
これもコンストラクタインジェクション用のコードを自動生成するだけです。


//footnote[wire][@<href>{https://github.com/google/wire}]
//footnote[simplicity][@<href>{https://employment.en-japan.com/engineerhub/entry/2018/06/19/110000}]

=== @<tt>{依存関係逆転の原則}と@<tt>{Go}
@<tt>{DIP}を使うことでパッケージ間、構造体間の結合度を下げることができます。
ただし、早すぎたり過剰な抽象化はソースコードの可読性を下げたり、手戻りが発生しやすくなります。
他言語で@<tt>{DI}ツールが重宝されるのは次の理由もあります。

 * @<tt>{DLL}ファイルや@<tt>{Jar}ファイルから実行時に動的にクラスをロードできる
 * フレームワークや@<tt>{UI}に依存した実装を避けたい

データベースに依存させたくないなどの理由がある場合など、@<tt>{DI}の利用は適切に用法用量を守って使いましょう。

特に、@<code>{interface}を使った過剰な抽象化には注意が必要です。
@<tt>{Go}は具象型にインターフェース名を記載しない暗黙的インターフェース実装を採用しているため、次の箇条書きの情報を得るにはIDEなどの助けが必要になります。

 * ある@<tt>{struct}がどの@<code>{interface}を継承しているのか
 * ある@<code>{interface}を継承している@<tt>{struct}はどれだけ存在しているのか

現実的な解法の1つとして、ある構造体（実装の詳細）が特定のインターフェースを実装しているかコンパイラにチェックさせる方法があります@<fn>{ensure}。
@<list>{ensure_interface}は@<code>{Knight}型が@<code>{Jedi}インターフェースを満たしているか、コンパイル時に検証させる方法です。
このようなプラクティスはあるものの、過剰な抽象化をしないように気を付けましょう。

//list[ensure_interface][コンパイラを使った実装チェック]{
type Jedi interface {
    HasForce() bool
}

type Knight struct {}

// このままではコンパイルエラーになるので、実装が不十分なことがわかる。
var _ Jedi = (*Knight)(nil)
//}

//footnote[ensure][@<href>{https://splice.com/blog/golang-verify-type-implements-interface-compile-time/}]

== 終わりに
本章では、SOLIDの原則のおさらいをしました。
そして、SOLIDの原則の各原則が@<tt>{Go}のプログラミングの中でどう表出されるのか確かめました。
@<tt>{Go}は一般にオブジェクト指向言語と呼ばれるプログラミング言語がもつ特徴を十分に備えていません。
そのような@<tt>{Go}でもオブジェクト指向設計のプラクティスや原則を守ることで、よりよいいコードを書くことができます。

== 参考文献
最後に、本章を執筆するにあたって参考にした書籍を挙げておきます。

 * アジャイルソフトウェア開発の奥義 第2版 オブジェクト指向開発の神髄と匠の技
 ** @<href>{https://www.amazon.co.jp/dp/4797347783}
 * オブジェクト指向入門 第2版 原則・コンセプト
 ** @<href>{https://www.amazon.co.jp/dp/4798111112}
 * Effective Java 第3版
 ** @<href>{https://www.amazon.co.jp/dp/B07RHX1K53}
 * Clean Architecture 達人に学ぶソフトウェアの構造と設計
 ** @<href>{https://www.amazon.co.jp/dp/B07FSBHS2V}
 * C#実践開発手法 デザインパターンとSOLID原則によるアジャイルなコーディング
 ** @<href>{https://www.amazon.co.jp/dp/B010A8WHFC}
 * Adaptive Code ~ C#実践開発手法 第2版
 ** @<href>{https://www.amazon.co.jp/dp/B07DJ2BL4Y}