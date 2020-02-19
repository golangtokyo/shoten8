= GoにおけるSOLID

@<tt>{@budougumi0617}@<fn>{bd617_twitter}です。
#@# textlint-disable
オブジェクト指向設計の原則を5つまとめた@<kw>{SOLIDの原則}@<fn>{solid}（@<tti>{the SOLID principles}）という設計指針があることはみなさんご存じかと思います。
#@# textlint-enable
本章ではSOLIDの原則にのっとったGoの実装について考えます。

//footnote[bd617_twitter][@<href>{https://twitter.com/budougumi0617}]
//footnote[solid][@<href>{https://ja.wikipedia.org/wiki/SOLID}]


== @<kw>{SOLIDの原則}（@<tti>{the SOLID principles}）とは
SOLIDの原則は次の用語リストに挙げた5つのソフトウェア設計の原則の頭文字をまとめたものです。

#@# textlint-disable
 : @<kw>{単一責任の原則}（@<kw>{SRP}, @<tti>{Single responsibility principle}）
    クラスを変更する理由は1つ以上存在してはならない。@<fn>{agile_srp}
 : @<kw>{開放閉鎖の原則}（@<kw>{OCP}, @<tti>{Open–closed principle}）
    ソフトウェアの構成要素構成要素（クラス、モジュール、関数など）は拡張に対して開いて（オープン: Oepn）いて、修正に対して閉じて（クローズド: Closed）いなければならない。@<fn>{agile_ocp}
 : @<kw>{リスコフの置換原則}（@<kw>{LSP}, @<tti>{Liskov substitution principle}）
    @<tt>{S}型のオブジェクト@<tt>{o1}の各々に、対応する@<tt>{T}型のオブジェクト@<tt>{o2}が1つ存在し、@<tt>{T}を使って定義されたプログラム@<tt>{P}に対して@<tt>{o2}の代わりに@<tt>{o1}を使っても@<tt>{P}の振る舞いが変わらない場合、@<tt>{S}は@<tt>{T}の派生型であると言える。@<fn>{agile_lsp}
 : インターフェイス分離の原則（@<kw>{ISP}, @<tti>{Interface segregation principle}）
    クライアントに、クライアントが利用しないメソッドへの依存を強制してはならない。@<fn>{agile_isp}
 : @<kw>{依存性逆転の原則}（@<kw>{DIP},  @<tti>{Dependency inversion principle}）
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
//footnote[nazonohito51][https://twitter.com/nazonohito51]
//footnote[whats-solid-principle][https://speakerdeck.com/nazonohito51/whats-solid-principle]

== Goとオブジェクト指向プログラミング
本題へ入る前に、Goとオブジェクト指向プログラミングの関係を考えてみます。
そもそもオブジェクト指向に準拠したプログラミング言語であることの条件とは何でしょうか。
さまざまな主張はありますが、本章では次の3大要素を備えることが「オブジェクト指向に準拠したプログラミング言語であること」とします。

 1. カプセル化（@<tt>{Encapsulation}）
 2. 多態性（ポリモフィズム）（@<tt>{Polymorphism}）
 3. 継承（@<tt>{Inheritance})


#@# textlint-disable
Go公式サイトには@<kw>{Frequently Asked Questions (FAQ)}@<fn>{q_and_a}という「よくある質問と答え」ページがあります。
この中の@<i>{Is Go an object-oriented language?}（Goはオブジェクト指向言語ですか？）という質問で、公式見解が述べられています。
#@# textlint-enable

//quote{
Yes and no. Although Go has types and methods and allows an object-oriented style of programming, there is no type hierarchy. The concept of “interface” in Go provides a different approach that we believe is easy to use and in some ways more general. There are also ways to embed types in other types to provide something analogous—but not identical—to subclassing. Moreover, methods in Go are more general than in C++ or Java: they can be defined for any sort of data, even built-in types such as plain, “unboxed” integers. They are not restricted to structs (classes).

Also, the lack of a type hierarchy makes “objects” in Go feel much more lightweight than in languages such as C++ or Java.
//}

//footnote[q_and_a][@<href>{https://golang.org/doc/faq#Is_Go_an_object-oriented_language}]

非常にあいまいな回答にはなっていますが、Goはオブジェクト指向の3大要素を一部しか取り入れていないため、このような回答になっています。
オブジェクト指向言語に期待されるで仕組みの1つとして、クラスの階層構造による継承があります。
Goはそのような継承の仕組みを言語仕様としてサポートしていません。
埋込み型による疑似継承のアプローチもありますが、これは多態性や共変性、反変性を満たさないのでオブジェクト指向で期待される@<kw>{実装継承}（@<i>{Implemebtation inheritance}）ではなくコンポジションにすぎません。
#@# できたらここで代入できないサンプルコードを書く。
そのため、@<kw>{リスコフの置換原則}などの一部のSOLIDの原則をそのままGoに適用することはできません。
しかし、SOLIDの原則のベースとなる考えを取り入れることでよりシンプルで可用性の高いGoのコードを書くことは可能です。

==[column] 実装継承（@<i>{implemebtation inheritance}）について
あるクラスが他のクラスを拡張した場合の継承を@<kw>{実装継承}（@<i>{implemebtation inheritance}）と呼びます。
あるクラスがインターフェースを実装した場合や、インターフェースが他のインターフェースを拡張した場合の継承を@<kw>{インターフェース継承}（@<i>{interface inheritance}）と呼びます。
Goがサポートしている継承は@<kw>{インターフェース継承}のみです。
クラスの親子関係による@<kw>{実装継承}はカプセル化を破壊する危険も大きく、深い継承構造はクラスの構成把握を困難にするという欠点もあります。
このことは代表的なオブジェクト指向言語である@<kw>{Java}の名著、「@<kw>{Effective Java}」@<fn>{amzn_java}@<fn>{effective_java}の「@<kw>{項目16 継承よりコンポジションを選ぶ}」でも言及されています。
Goが@<kw>{実装継承}をサポートしなかった理由は明らかになっていませんが、筆者は以上の危険性があるためサポートされていないと考えています。

//footnote[amzn_java][@<href>{https://www.amazon.co.jp/dp/4621066056}]
//footnote[effective_java][筆者が所有している@<kw>{Effective Java}は第2版ですが、2018年に@<tt>{Java 9}対応の<kw>{Effective Java 第3版}が発売されています。]

== ここから書けていない。

#@# textlint-disable
雑メモの塊なので静的解析対象外。
一番の大きな差異としてGoは

== SRP
Adaptorパターン、Decoratorパターン、Compositeパターン

== オープンクローズドの法則
アジャイル開発の奥義

ソフトウェアエンティティは、拡張に大して開いていなければならず、変更に対して閉じていなければならない。
#@# textlint-enable
#@# textlint-disable
「拡張に対して開いている」。これは、モジュールの振る舞いを拡張できることを意味する。
アプリケーションの要求が変化したら、それらの変更内容を満たす新しいふるまいでモジュールを拡張することが可能である。言い換えれば、モジュールが実行することを変更できるのである。
「変更に対して閉じている」。モジュールの振る舞いを拡張した結果として、モジュールのソースやバイナリコードで変更が発生しない。モジュールのバイナリコードは、リンク可能なライブラリなのか、DLIなのか、Javaの@<tt>{.jar}なのかにかかわらず、変更されないままとなる。
#@# textlint-enable
#@# textlint-disable
== リスコフ

SがTの派生型であるとすれば、T型のオブジェクトをS型のオブジェクトと置き換えたとしても、プログラムは動作し続けるはずである。
Barbara Liskov

反変性、共変性。契約設計。コントラクト。事前条件と事後条件。
事前条件を派生型で強化することはできない
事後条件は派生型で緩和することはできない。
基底型の普遍条件は派生型でも維持されなければならない。
新しい例外は許可されない

DaveはどうやってGoに紐付けたか。

== インターフェース分離の原則
ファサード。

== 依存性反転の原則
poor man Adaptorパターン。Responsible Ownerパターン、Factory Isolationパターン
Illegitimate Injectionパターン。
ここまで雑メモ
#@# textlint-enable



== 参考文献
 * アジャイルソフトウェア開発の奥義 第2版 オブジェクト指向開発の神髄と匠の技
 ** @<href>{https://www.amazon.co.jp/dp/4797347783}
 * Clean Code　アジャイルソフトウェア達人の技
 ** @<href>{https://www.amazon.co.jp/dp/B078HYWY5X}
 * C#実践開発手法　デザインパターンとSOLID原則によるアジャイルなコーディング
 ** @<href>{https://www.amazon.co.jp/dp/B010A8WHFC}
 * Adaptive Code ~ C#実践開発手法　第2版
 ** @<href>{https://www.amazon.co.jp/dp/B07DJ2BL4Y}