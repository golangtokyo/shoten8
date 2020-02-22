= GoにおけるSOLIDの原則
@<tt>{@budougumi0617}@<fn>{bd617_twitter}です。
オブジェクト指向設計の原則を5つまとめた@<kw>{SOLIDの原則}@<fn>{solid}（@<tti>{the SOLID principles}）という設計指針があることはみなさんご存じでしょう。
本章ではSOLIDの原則にのっとったGoの実装について考えます。

//footnote[bd617_twitter][@<href>{https://twitter.com/budougumi0617}]
//footnote[solid][@<href>{https://ja.wikipedia.org/wiki/SOLID}]

== @<kw>{SOLIDの原則}（@<tti>{the SOLID principles}）とは
SOLIDの原則は次の用語リストに挙げた5つのソフトウェア設計の原則の頭文字をまとめたものです。

#@# textlint-disable
 : @<kw>{単一責任の原則}（@<kw>{SRP}, @<tti>{Single responsibility principle}）
    クラスを変更する理由は1つ以上存在してはならない。@<fn>{agile_srp}
 : @<kw>{オープン・クローズドの原則}（@<kw>{OCP}, @<tti>{Open–closed principle}）
    ソフトウェアの構成要素構成要素（クラス、モジュール、関数など）は拡張に対して開いて（オープン: Open）いて、修正に対して閉じて（クローズド: Closed）いなければならない。@<fn>{agile_ocp}
 : @<kw>{リスコフの置換原則}（@<kw>{LSP}, @<tti>{Liskov substitution principle}）
    @<tt>{S}型のオブジェクト@<tt>{o1}の各々に、対応する@<tt>{T}型のオブジェクト@<tt>{o2}が1つ存在し、@<tt>{T}を使って定義されたプログラム@<tt>{P}に対して@<tt>{o2}の代わりに@<tt>{o1}を使っても@<tt>{P}の振る舞いが変わらない場合、@<tt>{S}は@<tt>{T}の派生型であると言える。@<fn>{agile_lsp}
 : インターフェイス分離の原則（@<kw>{ISP}, @<tti>{Interface segregation principle}）
    クライアントに、クライアントが利用しないメソッドへの依存を強制してはならない。@<fn>{agile_isp}
 : @<kw>{依存関係逆転の原則}（@<kw>{DIP},  @<tti>{Dependency inversion principle}）
    上位のモジュールは下位のモジュールに依存してはならない。どちらのモジュールも「抽象」に依存すべきである。「抽象」は実装の詳細に依存してはならない。実装の詳細が「抽象」に依存すべきである。@<fn>{agile_dip}
#@# textlint-enable

//footnote[agile_srp][アジャイルソフトウェア開発の奥義 第2版 8.1より]
//footnote[agile_ocp][アジャイルソフトウェア開発の奥義 第2版 9.1より]
//footnote[agile_lsp][アジャイルソフトウェア開発の奥義 第2版 10.1より]
//footnote[agile_isp][アジャイルソフトウェア開発の奥義 第2版 12.3より]
//footnote[agile_dip][アジャイルソフトウェア開発の奥義 第2版 11.1より]

これらの原則は、ソフトウェアをより理解しやすく、より柔軟でメンテナンス性の高いものにする目的で考案されました。
各々の原則の原案者や発表時期は異なりますが、この5つの原則から頭文字のアルファベットを1つずつ取り@<tt>{SOLIDの原則}としてまとめたのが@<i>{Robert C. Martin}です。@<fn>{getting_a_slid_start}。
メーリングリストに投稿された@<i>{Robert C. Martin}のコメント@<fn>{ttc_of_oop}を見ると、1995年時点でオープンクローズドの原則（@<tt>{The open/closed principle}）などの命名がされていることがわかります。
また、SOLIDの原則として5つの原則がまとめて掲載された日本語書籍は、「アジャイルソフトウェア開発の奥義」@<fn>{amzn_agile}になります。
SOLIDの原則自体についてもっと知りたい場合は、@<tt>{@nazonohito51}さんの「@<tt>{「SOLIDの原則って何ですか？」って質問に答えたかった}@<fn>{whats-solid-principle}」を読んでみるとよいでしょう。

//footnote[getting_a_slid_start][@<href>{https://sites.google.com/site/unclebobconsultingllc/getting-a-solid-start}]
//footnote[ttc_of_oop][@<href>{https://groups.google.com/forum/m/#!msg/comp.object/WICPDcXAMG8/EbGa2Vt-7q0J}]
//footnote[amzn_agile][@<href>{https://www.amazon.co.jp/dp/4797347783}]
//footnote[nazonohito51][@<href>{https://twitter.com/nazonohito51}]
//footnote[whats-solid-principle][@<href>{https://speakerdeck.com/nazonohito51/whats-solid-principle}]

== Goとオブジェクト指向プログラミング
本題へ入る前に、Goとオブジェクト指向プログラミングの関係を考えてみます。
そもそもオブジェクト指向に準拠したプログラミング言語であることの条件とは何でしょうか。
さまざまな主張はありますが、本章では次の3大要素を備えることが「オブジェクト指向に準拠したプログラミング言語であること」とします。

 1. カプセル化（@<i>{Encapsulation}）
 2. 多態性（ポリモフィズム）（@<i>{Polymorphism}）
 3. 継承（@<i>{Inheritance})


#@# textlint-disable
ではGoはオブジェクト指向言語なのでしょうか。Go公式サイトには@<kw>{Frequently Asked Questions (FAQ)}@<fn>{q_and_a}という「よくある質問と答え」ページがあります。
この中の@<i>{Is Go an object-oriented language?}（Goはオブジェクト指向言語ですか？）という質問に対する答えとして、次の公式見解が記載されています。
#@# textlint-enable

//quote{
Yes and no. Although Go has types and methods and allows an object-oriented style of programming, there is no type hierarchy. The concept of “interface” in Go provides a different approach that we believe is easy to use and in some ways more general. There are also ways to embed types in other types to provide something analogous—but not identical—to subclassing. Moreover, methods in Go are more general than in C++ or Java: they can be defined for any sort of data, even built-in types such as plain, “unboxed” integers. They are not restricted to structs (classes).

Also, the lack of a type hierarchy makes “objects” in Go feel much more lightweight than in languages such as C++ or Java.
//}

//footnote[q_and_a][@<href>{https://golang.org/doc/faq#Is_Go_an_object-oriented_language}]

あいまいな回答にはなっていますが、「Yesであり、Noでもある。」という回答です。
Goはオブジェクト指向の3大要素を一部しか取り入れていないため、このような回答になっています。

=== Goはサブクラシング（@<tti>{subclassing}）に対応していない
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

このような継承関係（ポリモフィズム）を表現するとき、Goはインターフェースを使うでしょう。
Goは@<list>{person}のような実像クラス（あるいは抽象クラス）を親とするようなサブクラシングによる継承の仕組みを言語仕様としてサポートしていません。
Goでは埋込み（@<i>{Embedding}@<fn>{embedding}）を使って別の型に実装を埋め込むアプローチもあります。
しかし、これは多態性や共変性・反変性@<fn>{convariance}を満たしません。
よって、埋め込みはオブジェクト指向で期待される継承ではなくコンポジションにすぎません。
@<list>{go_person}は@<list>{person}をGoで書き直したものです。
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
以上の例以外にも、@<kw>{リスコフの置換原則}などの一部のSOLIDの原則をそのままGoに適用することはできません。
しかし、SOLIDの原則のベースとなる考えを取り入れることでよりシンプルで可用性の高いGoのコードを書くことは可能です。

それでは、次節よりGoのコードにSOLIDの原則を適用していくとどうなっていくか見ていきます。


//footnote[convariance][@<href>{https://docs.microsoft.com/ja-jp/dotnet/csharp/programming-guide/concepts/covariance-contravariance/}]
//footnote[embedding][@<href>{https://golang.org/doc/effective_go.html#embedding}]

===[column] 実装よりもコンポジションを選ぶ
あるクラスが他の具象クラス（抽象クラス）を拡張した場合の継承を、@<tt>{Java}の世界では@<kw>{実装継承}（@<i>{implemebtation inheritance}）と呼びます。
あるクラスがインターフェースを実装した場合や、インターフェースが他のインターフェースを拡張した場合の継承を@<kw>{インターフェース継承}（@<i>{interface inheritance}）と呼びます。
Goがサポートしている継承はJavaの言い方を借りるならば@<kw>{インターフェース継承}のみです。
クラスの親子関係による@<kw>{実装継承}はカプセル化を破壊する危険も大きく、深い継承構造はクラスの構成把握を困難にするという欠点もあります。
このことは代表的なオブジェクト指向言語である@<kw>{Java}の名著、「@<kw>{Effective Java}」の「@<kw>{項目16 継承よりコンポジションを選ぶ}」でも言及されています（筆者が所有している@<kw>{Effective Java}は第2版ですが、2018年に@<tt>{Java 9}対応の<kw>{Effective Java 第3版}が発売されています）。
Goが@<kw>{実装継承}をサポートしなかった理由は明らかになっていませんが、筆者は以上の危険性があるためサポートされていないと考えています。

===[/column]

== @<kw>{単一責任の原則}（@<kw>{SRP}, @<tti>{Single responsibility principle}）
//quote{
クラスを変更する理由は1つ以上存在してはならない。
//}

ここでは@<tt>{アジャイルソフトウェア開発の奥義}の原文どおりに「クラスを…」とそのまま引用しましたが、
@<kw>{単一責任の原則}はさまざまな大小の粒度で考えなければいけない問題です。
関数・メソッド単位の粒度ならば比較的考えやすそうです。
ドメインやパッケージの単位の単一責任になってくると一概にいえるものではなくなり、議論が始まるでしょう。

変更理由が1つしか存在しないということは、そのクラスの役割（責任）がひとつであることを意味します。
では、「このクラスの役割が1つである」と判断するにはどうすればよいのでしょうか。
たとえば、「コンロ」をモデリングするとき、コンロには「火を付ける」機能と「火を消す」機能の2つが与えるでしょう。
これは「役割が2つある」状態でしょうか。

#@# textlint-disable
#@# ここから下は書けていない。

@<tt>{アジャイルソフトウェア開発の奥義}では、この判断基準を

Adaptorパターン、Decoratorパターン、Compositeパターン。

== @<kw>{オープン・クローズドの原則}（@<kw>{OCP}, @<tti>{Open–closed principle}）
//quote{
ソフトウェアの構成要素構成要素（クラス、モジュール、関数など）は拡張に対して開いて（オープン: Oepn）いて、修正に対して閉じて（クローズド: Closed）いなければならない。
//}

　たいていのソースコードは完成後に（あるいは作成中でも）機能追加・仕様の変更・あるいは仕様漏れの対応で機能の修正あるいは拡張が発生します。
このとき「硬い」設計のソフトウェアは変更に対して大きな修正・そして副作用が発生します。
変更・拡張に対して柔軟である一方で、修正の副作用に対して強固であるべきです。
それを表す原則が@<kw>{オープン・クローズドの原則}（@<tt>{OCP}）の原則です。

　@<tt>{OCP}の原則は


ソフトウェアエンティティは、拡張に大して開いていなければならず、変更に対して閉じていなければならない。
#@# textlint-enable
#@# textlint-disable
「拡張に対して開いている」。これは、モジュールの振る舞いを拡張できることを意味する。
アプリケーションの要求が変化したら、それらの変更内容を満たす新しいふるまいでモジュールを拡張できる。言い換えれば、モジュールが実行することを変更できるのである。
「変更に対して閉じている」。モジュールの振る舞いを拡張した結果として、モジュールのソースやバイナリコードで変更が発生しない。モジュールのバイナリコードは、リンク可能なライブラリなのか、DLIなのか、Javaの@<tt>{.jar}なのかにかかわらず、変更されないままとなる。
#@# textlint-enable
#@# textlint-disable





== @<kw>{リスコフの置換原則}（@<kw>{LSP}, @<tti>{Liskov substitution principle}）
//quote{
@<tt>{S}型のオブジェクト@<tt>{o1}の各々に、対応する@<tt>{T}型のオブジェクト@<tt>{o2}が1つ存在し、@<tt>{T}を使って定義されたプログラム@<tt>{P}に対して@<tt>{o2}の代わりに@<tt>{o1}を使っても@<tt>{P}の振る舞いが変わらない場合、@<tt>{S}は@<tt>{T}の派生型であると言える。
//}



SがTの派生型であるとすれば、T型のオブジェクトをS型のオブジェクトと置き換えたとしても、プログラムは動作し続けるはずである。
Barbara Liskov

反変性、共変性。契約設計。コントラクト。事前条件と事後条件。
事前条件を派生型で強化することはできない
事後条件は派生型で緩和することはできない。
基底型の普遍条件は派生型でも維持されなければならない。
新しい例外は許可されない

DaveはどうやってGoに紐付けたか。

== インターフェイス分離の原則（@<kw>{ISP}, @<tti>{Interface segregation principle}）

//quote{
クライアントに、クライアントが利用しないメソッドへの依存を強制してはならない。
//}

ファサード。


#@# ここから上まで書けていない
#@# textlint-enable

== @<kw>{依存関係逆転の原則}（@<kw>{DIP}, @<tti>{Dependency inversion principle}）
//quote{
上位のモジュールは下位のモジュールに依存してはならない。どちらのモジュールも「抽象」に依存すべきである。「抽象」は実装の詳細に依存してはならない。実装の詳細が「抽象」に依存すべきである。
//}

ここまで見てきたとおり、拡張性が高く副作用に強いソフトウェアを実現するための鍵は構造化と境界です。
対象を型あるいはパッケージとして構造化し、それぞれの境界を疎結合にすることで柔軟な設計を実現できます。
型同士、またはパッケージ同士を疎結合にするための考え方が@<kw>{依存関係逆転の原則}です。
Goの標準パッケージ内で具体例を確認します。

=== @<code>{database/sql/driver}パッケージと@<kw>{DIP}

　Goの標準パッケージでは、@<code>{database/sql/driver}パッケージが@<kw>{DIP}を利用した典型的な設計です。
通常のGopherはGoから@<tt>{MySQL}などの@<tt>{RDBMS}を操作する際は@<code>{database/sql}パッケージを介した操作をします。
この@<code>{database/sql}パッケージに各ベンダー、OSSの個別仕様に対応する具体的な実装は含まれていません。
では、どのように@<tt>{MySQL}や@<tt>{PostgreSQL}を操作するかというと
@<list>{import_mysql}のように各@<tt>{RDBMS}に対応したドライバパッケージを@<tt>{import}します。


//list[import_mysql][GoでMySQLを操作する際の@<code>{import}文]{
import (
  "database/sql"
  _ "github.com/go-sql-driver/mysql"
)
//}
#@# textlint-disable
@<code>{github.com/go-sql-driver/mysql}のようなドライバパッケージは@<code>{database/sql/driver}パッケージ内の@<code>{database/sql/driver.Driver}インターフェースなどを実装しています。
#@# textlint-enable
RDBMSごとの@<kw>{実装の詳細}が上位概念が提供している@<kw>{インターフェースに依存}しています。
（ほぼありえないでしょうが、）もし@<code>{database/sql/driver}パッケージのインターフェースが変更された場合、すべてのドライバパッケージがインターフェースの変更に追従を迫られるでしょう。
このような下位の実装の詳細が上位概念（@<code>{database/sql/driver}パッケージ）の抽象へ依存している関係を@<kw>{依存関係逆転の原則}と呼びます。


=== @<kw>{DIP}に準拠した実装
　では、実際の我々の設計や書く実装コードで@<tt>{DIP}の考えを活かすとどのようになるのでしょうか。
よく耳にする実装パターンとしては、依存性の注入（@<kw>{DI}, @<kw>{Dependency Injection}）パターン@<fn>{wiki_di}やリポジトリパターン@<fn>{repository}でしょう。
リポジトリパターンは永続化データへのアクセスに関する@<tt>{DI}と呼べるでしょう。


//footnote[wiki_di][@<href>{https://en.wikipedia.org/wiki/Dependency_injection}]
//footnote[repository][@<href>{https://martinfowler.com/eaaCatalog/repository.html}]

==== 依存性の注入（@<kw>{Dependency Injection}）
@<kw>{依存性の注入}（@<kw>{DI}）は@<kw>{DIP}を実施するためのオーソドックスな手段です。
JavaやC#などにはクラスのフィールド定義にアノテーションをつけるだけでオブジェクト（下位モジュールの詳細）をセット（注入）してくれるような、フレームワークが提供するデファクトな仕組みが存在します。
Goの場合はインターフェースで抽象を定義し、初期化時などに具体的な実装の詳細オブジェクトを設定することが大半です。

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

@<list>{di_setter}は@<code>{setter}メソッドを用意していおくことで、初期化と実処理の間に依存性を注入する方法です。

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

@<list>{di_method}はメソッド（関数）の引数として依存を渡す方法です。上位階層のオブジェクトのライフサイクルと、実装の詳細のオブジェクトの生成タイミングが異なるときはこの手法を取り巻す。
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
このほかにもGoでは次のような実装で@<tt>{DIP}を満たすことも可能です。


=== 埋込み型を利用した@<tt>{DIP}
Goは構造体の中に別の構造体やインターフェースを埋め込めます。
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
@<tt>{DIP}では型の継承関係を利用することが多いです。
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

Goは@<kw>{単純}であることが言語思想@<fn>{simplicity}にあるため、（ソースコードに書いてある以上の挙動を裏で実行するような）高度な@<kw>{DI}ツールはあまり使われていない印象です。
@<kw>{DI}用のコードを自動生成する@<tt>{google/wire}フレームワーク@<fn>{wire}も存在しますが、
これもコンストラクタインジェクション用のコードを自動生成するだけです。


//footnote[wire][@<href>{https://github.com/google/wire}]
//footnote[simplicity][@<href>{https://employment.en-japan.com/engineerhub/entry/2018/06/19/110000}]

== まとめ
　本章では、SOLIDの原則のおさらいをしました。
そして、SOLIDの原則の各原則がGoのプログラミングの中でどう表出されるのか確かめました。
Goは一般にオブジェクト指向言語と呼ばれるプログラミング言語がもつ特徴を十分に備えていません。
そのようなGoでもオブジェクト指向設計のプラクティスや原則を守ることで、よりよいいコードを書くことができます。

　ただし、早すぎたり過剰な抽象化はソースコードの可読性を下げたり、手戻りが発生しやすくなります。
特に、@<code>{interface}を使った過剰な抽象化には注意が必要です。
Goはインターフェースを使ったダックタイピングを採用しているため、次の箇条書きの情報を得るにはIDEなどの助けが必要になります。

 * ある@<tt>{struct}がどの@<code>{interface}を継承しているのか
 * ある@<code>{interface}を継承している@<tt>{struct}はどれだけ存在しているのか

@<list>{ensure_interface}は@<code>{Knight}型が@<code>{Jedi}インターフェースを満たしているか、コンパイル時に検証させる方法です@<fn>{ensure}。
このようなプラクティスはあるものの、過剰な抽象化をしないように気を付けましょう。

//list[ensure_interface][コンパイラを使った実装チェック]{
type Jedi interface {
    HasForce() bool
}

type Knight struct {}

// このままではコンパイルエラーになるので、継承関係がないことがわかる。
var _ Jedi = (*Knight)(nil)
//}

//footnote[ensure][@<href>{https://splice.com/blog/golang-verify-type-implements-interface-compile-time/}]



== 参考文献
最後に、本章を執筆するにあたって参考にした書籍を挙げておきます。

 * アジャイルソフトウェア開発の奥義 第2版 オブジェクト指向開発の神髄と匠の技
 ** @<href>{https://www.amazon.co.jp/dp/4797347783}
 * Effective Java 第3版
 ** @<href>{https://www.amazon.co.jp/dp/B07RHX1K53}
 * Clean Architecture　達人に学ぶソフトウェアの構造と設計
 ** @<href>{https://www.amazon.co.jp/dp/B07FSBHS2V}
 * C#実践開発手法　デザインパターンとSOLID原則によるアジャイルなコーディング
 ** @<href>{https://www.amazon.co.jp/dp/B010A8WHFC}
 * Adaptive Code ~ C#実践開発手法　第2版
 ** @<href>{https://www.amazon.co.jp/dp/B07DJ2BL4Y}