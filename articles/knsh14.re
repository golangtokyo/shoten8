= templateのlintツールを作ろう

Goは標準パッケージでテンプレート機能をサポートしています。
テンプレート機能とは、出力形式をあらかじめ定義しておき、出力時に定義された形式にしたがって出力を組み立てる機能です。

テンプレート機能はWebサービスやコード生成などさまざまな場面で利用されています。
@<code>{go list}コマンドでも結果を出力する際にテンプレート機能を利用することで、利用者が出力結果を自由に変更できます。

Goのテンプレート機能はブロックの定義や、自作関数の適用など複雑なテンプレートも作成できます。
テンプレートで複雑な処理を行うと実行した時にどのような形式になるか予測しづらくなります。
テンプレートによる出力を実行する前に、ミスがないかチェックする必要があります。

本章ではテンプレートの中でwith式を使った時にその変数が使われているかをチェックするツール@<tt>{withcheck}を作ります。
加えて、text/templateパッケージ内ではどのようにテンプレートの解析処理が行われているのかを説明します。

== チェックツールの概要
text/templateパッケージのテンプレートではwith句@<fn>{link_actions}という構文があります。
これは引数の変数がnilではない場合に@<code>{true}になる条件分岐の構文です。

//footnote[link_actions][@<href>{https://golang.org/pkg/text/template/#hdr-Actions}]

この構文が@<code>{true}になる場合、その変数を利用していなければ意味がない条件になります。
意味のない条件を防ぐためにテンプレートをチェックし実行する前にエラーを出します。

具体的な例を用いて説明します。
with句を使ったテンプレートを使うコードを@<list>{with_statement_example}に示します。

//listnum[with_statement_example][with句を使ったテンプレートのサンプル]{
package main

import (
  "log"
  "os"
  "text/template"
)

type Person struct {
  Name string
  Age  int
}

const (
  goodTpl = `hello, {{ with .Name }}my name is {{ .Name }}{{ else }}, nice to meet you{{ end }}.`
  badTpl  = `hello, {{ with .Name }}I am {{ .Bar }} years old{{ else }}, nice to meet you{{ end }}.`
)

func Good() {
  v := Person{
    Name: "knsh14",
    Age:  2020,
  }
  tmpl, err := template.New("good").Parse(goodTpl)
  if err != nil {
    log.Fatal(err)
  }
  err = tmpl.Execute(os.Stdout, v)
  if err != nil {
    log.Fatalf("execution: %s", err)
  }
}

func Bad() {
  v := Person{
    Name: "knsh14",
    Age:  2020,
  }
  tmpl, err := template.New("bad").Parse(badTpl)
  if err != nil {
    log.Fatal(err)
  }
  err = tmpl.Execute(os.Stdout, v)
  if err != nil {
    log.Fatalf("execution: %s", err)
  }
}
//}

Goodの場合ではwith句でチェックした@<code>{Name}変数があれば、それを出力しています。
Badの場合ではwith句で@<code>{Name}変数をチェックしたにもかかわらず、@<code>{Age}変数を出力しています。
Badのようなテンプレートの場合警告が出るようにチェックします。

テンプレートの静的解析は、Goコードの静的解析のように実行する前に別のツールとして実行するのはたいへんです。
そのため、コードを実行する際に@<code>{text/template.Template}型の変数に対してチェックします。
もしwith句が@<code>{true}の場合にその変数を使用していなければ、@<code>{ErrNotFound}を返すようなチェッカーを作成します。
@<list>{example_withcheck_execute}のように@<code>{template.Execute}関数を実行する前にチェックします。

//list[example_withcheck_execute][withcheckの実行例][go]{
input := `{{ with .Foo}}{{.Bar}}{{end}}`
tpl, err := template.New("test").Parse(input)
if err != nil {
  log.Fatal(err)
}

if err := withcheck.Check(tpl); err != nil { // not found
  log.Fatal(err)
}

if err := tpl.Execute(os.Stdout, v); err != nil {
  log.Fatalf("execution: %s", err)
}
//}

== withckeckの設計
このチェックツールをwithcheckとしてGitHub@<fn>{withcheck_github_link}に公開しています。
実際のコードを試してみたい方は参照してください。

//footnote[withcheck_github_link][@<href>{https://github.com/knsh14/withcheck}]

withcheckは大きく2つのステップに別れます。

1. with句の条件部分に指定されている変数を探し出す。
2. 指定された変数が、with句が@<code>{true}になった場合のテンプレートで使われているか探し出す。

=== With句の抽象構文木

text/templateの@<code>{template.Template}型は内部にテンプレートの抽象構文木を持っています。
templateの抽象構文木のノードはtext/template/parseパッケージ内@<fn>{parse_package_link}で定義されています。

with句は@<code>{text/template/parse.WithNode}@<fn>{withnode_document_link}という型で表されます。
@<code>{WithNode}は@<code>{BranchNode}@<fn>{branchnode_document_link}という型を埋め込んでいます。
@<code>{BranchNode}は条件分岐のための汎用的なノードです。

//footnote[parse_package_link][@<href>{https://golang.org/pkg/text/template/parse/}]
//footnote[withnode_document_link][@<href>{https://golang.org/pkg/text/template/parse/#WithNode}]
//footnote[branchnode_document_link][@<href>{https://golang.org/pkg/text/template/parse/#BranchNode}]

=== with句でチェックする変数を探す

最初のステップとしてチェック対象の変数を探し出します。
with句の条件部分に記述できる形式を@<list>{example_with_statements}に示します。

//listnum[example_with_statements][with句の条件式の例]{
{{ with .Foo }}
{{ with . }}
{{ with .Foo .Bar }}
{{ with $x = 123 }}
{{ with $x := getFoo }}
{{ with $x := "huga" | println }}
//}

チェックする変数は@<code>{parse.WithNode}型の条件式部分を表現している@<code>{Pipe}フィールドから取り出します。
@<code>{Pipe}フィールドは@<code>{parse.PipeNode}型です。
@<code>{parse.PipeNode}型には@<code>{[]*parse.VariableNode}型の@<code>{Decl}フィールドと、@<code>{[]*parse.CommandNode}型の@<code>{Cmds}フィールドがあります。
@<list>{example_with_statements}の4〜6行目は@<code>{Decl}フィールドで確認できます。
1〜3行目は@<code>{Cmds}フィールドから確認できます。
@<code>{*parse.CommandNode}型のスライスですので、ループでチェックしていきます。
@<code>{*parse.CommandNode}型の引数を表す@<code>{Args}フィールドには、さまざまな種類のノードが入る可能性があります。
しかし実際にチェックする必要があるのは、@<code>{*parse.FieldNode}型か@<code>{*parse.DotNode}型のみです。
@<code>{*parse.DotNode}型はバリエーションがないため、シンプルです。
@<code>{*parse.FieldNode}型は@<code>{.X.Y.Z}のような要素の各フィールド名がスライスになっています。
このスライスをドットでつなぐことで要素名として復元します。

=== 変数が使われているかをチェックする
変数が実際に使われているかは@<code>{parse.WithNode}型の@<code>{List}フィールドをチェックします。
@<code>{List}フィールドは@<code>{*parse.ListNode}型で@<code>{parse.WithNode}型の@<code>{Pipe}フィールドが正の場合に実行されるノードです。

このノードを再帰的にチェックして入力された変数名が実際に使われているか調べます。
関数やパイプラインの引数をそれぞれチェックするのではなく、その引数のノードの型をチェックし、その中身をチェックします。
対象となるのは@<code>{*parse.FieldNode}や@<code>{*parse.IdentifierNode}などです。
それぞれの型によって比較する方法が異なります。

@<code>{*parse.FieldNode}や@<code>{*parse.VariableNode}、@<code>{*parse.ChainNode}は要素名がスライスになっているので、ドットでつなげてひとつにします。
@<code>{*parse.IdentifierNode}はそのまま使います。
得られた対象の変数の先頭がチェックすべき変数と同じか調べます。
なぜなら、対象の変数の中のフィールドを使う可能性があるからです。

他の要素としては@<code>{*parse.DotNode}があります。
これはドットだけの特別なノードです。
このノードの場合は完全に同じかチェックします。

=== 結果を返す
with句でチェックした変数が@<code>{nil}でない場合にその変数が正しく使われているかチェックすることはできました。
最後にその結果をわかりやすく返す必要があります。

withcheckでは@<code>{error}型で返すようにしました。
なぜなら、@<code>{bool}型を使うよりも@<code>{error}型を利用することで失敗時の情報をより多く返すことができます。
どの変数が見つからなくて失敗しているかまで返すことができるため、よりハンドリングしやすくなります。

本章を執筆している時点ではまだ単にエラーを返すだけですが、今後改修していく予定です。

== 最後に
本章ではtext/templateパッケージで生成したテンプレートを静的解析し、with句でチェックした変数が使われているかを確認するlintツールを作成しました。

テンプレートは独自の文法を持った小さな言語と見ることができます。
静的解析するための環境もGo本体ほどではありませんが、そろっています。

ぜひテンプレート機能を使う際には静的解析ツールも作成してレビューを楽にしてミスを減らしましょう。

#@# === テンプレートをパースする仕組み
#@# 
#@# text/template/parseパッケージに実装がある
#@# その中でtemplate用のインタプリタが動作していて、ASTを構築する
#@# 実際に実行する場合はまた別の話
#@# 
#@# text/template.Tempalte.Parseでは何が行われているのか見ていく。
#@# インタプリタをパースしていくようにお手製のパーサーが存在し、ASTにしていく。
#@# 
#@# ここは頑張ってどういうふうにトークンを拾っていくかとかを見ていく
#@# 
#@# 最後に「Writing An Interpreter in Go」を勧めておくといい感じだと思う、
#@# 
#@# === テンプレートを実行する仕組み
#@# 
#@# ここはおまけ
#@# 
#@# https://github.com/golang/go/blob/master/src/text/template/exec.go#L206
#@# 
#@# ここらへんを読んでいくのがいい気がする
#@# 
