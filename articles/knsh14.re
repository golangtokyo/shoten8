= templateのlintツールを作ろう

Goは標準パッケージでテンプレート機能をサポートしています。
テンプレート機能とは、出力形式をあらかじめ定義しておき、出力時に定義された形式にしたがって文字列を出力する機能です。

テンプレート機能はWebサービスやコード生成などさまざまな場面で利用されています。
@<code>{go list}コマンドでも結果を出力する際にテンプレート機能を利用することで、利用者が出力結果を自由に変更できます。

Goのテンプレート機能はブロックの定義や、自作関数の適用など複雑なテンプレートも作成できます。
テンプレートで複雑な処理を行うと実行した時にどのような形式になるか予測しづらくなります。
テンプレートによる出力を実行する前に、ミスをしていないかチェックする必要があります。

本章ではテンプレートの中でwith式を使った時にその変数が使われているかをチェックするツール@<tt>{withcheck}を作ります。
#@# 加えて、text/templateパッケージ内ではどのようにテンプレートの解析処理が行われているのかを説明します。

== チェックツールの概要
text/templateパッケージのテンプレートではwith句@<fn>{link_actions}という構文があります。
これは引数の変数がnilではない場合に@<code>{true}になる条件分岐の構文です。
さらにwith句の中ではスコープが変化し、with句でチェックした要素を内部ではドットになります。

//footnote[link_actions][@<href>{https://golang.org/pkg/text/template/#hdr-Actions}]

この構文が@<code>{true}になる場合、その変数を内部で利用していなければ意味がない条件になります。
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
  goodTpl = `hello,{{ with .Name }}my name is {{.}}{{ end }}.`
  badTpl  = `hello, {{ with .Name }}world{{ end }}.`
)

func main() {
  Good()
  Bad()
}

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
Badの場合ではwith句で@<code>{Name}変数をチェックしたにもかかわらず、その変数を使っていません。
Badのようなテンプレートの場合警告が出るようにチェックします。

テンプレートの静的解析は、Goコードの静的解析のように実行する前に別のツールとして実行するのはたいへんです。
そのため、コードを実行する際に@<code>{text/template.Template}型の変数に対してチェックします。
もしwith句が@<code>{true}の場合にその変数を使用していなければ、@<code>{ErrNotFound}を返すようなチェッカーを作成します。
@<list>{example_withcheck_execute}のように@<code>{template.Execute}関数を実行する前にチェックします。

//list[example_withcheck_execute][withcheckの実行例][go]{
input := `{{ with .Foo}}{{end}}`
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

実際にチェックするコードにすると@<list>{implementation_check_function}@<fn>{link_check_function}の実装になります。

//list[implementation_check_function][Check関数の実装]{
func Check(tmpl *template.Template) error {
  var err error
  templateutil.Inspect(tmpl.Tree.Root, func(node parse.Node) bool {
    if err != nil {
      return false
    }
    if n, ok := node.(*parse.WithNode); ok {
      v, e := getVariable(n.Pipe)
      if err != nil {
        err = e
        return false
      }
      e = checkVariable(n.List, v)
      if e != nil {
        err = e
      }
      return false
    }
    return true
  })
  return err
}
//}

//footnote[withcheck_github_link][@<href>{https://github.com/knsh14/withcheck}]
//footnote[link_check_function][@<href>{https://github.com/knsh14/withcheck/blob/master/check.go#L22-L43}]

withcheckはwith句に対して処理をします。
そのため、まずはwith句を探し出します。
その中で変数が使われているかをチェックします。
変数が使われているかのチェックは大きく2つのステップに別れます。

 1. with句の条件部分に指定されている変数を探し出す。
 2. 指定された変数が、with句が@<code>{true}になった場合のテンプレートで使われているか探し出す。

それぞれを@<code>{getVariable}、@<code>{checkVariable}関数として実装します。

=== with句の抽象構文木

text/templateパッケージの@<code>{template.Template}型は内部にテンプレートの抽象構文木を持っています。
templateの抽象構文木のノードはtext/template/parseパッケージ内@<fn>{parse_package_link}で定義されています。

with句は@<code>{parse.WithNode}@<fn>{withnode_document_link}型という型で表されます。
@<code>{WithNode}型は@<code>{BranchNode}@<fn>{branchnode_document_link}型という型を埋め込んでいます。
@<code>{BranchNode}型は条件分岐のための汎用的なノードです。

他にも@<code>{BranchNode}型を埋め込んでいる型として、@<code>{IfNode}型や@<code>{RangeNode}型があります。

//footnote[parse_package_link][@<href>{https://golang.org/pkg/text/template/parse/}]
//footnote[withnode_document_link][@<href>{https://golang.org/pkg/text/template/parse/#WithNode}]
//footnote[branchnode_document_link][@<href>{https://golang.org/pkg/text/template/parse/#BranchNode}]

=== with句でチェックする変数を探す
最初のステップとしてチェック対象の変数を探し出します。
実際の処理を@<list>{implementation_get_variable}に示します。

//list[implementation_get_variable][チェック対象の変数を取得する処理の実装]{
func getVariable(n *parse.PipeNode) ([]string, error) {
  if len(n.Decl) > 0 {
    if len(n.Decl) > 1 {
      return nil, ErrTooManyVariables
    }
    return []string{n.Decl[0].Ident[0], "."}, nil
  }
  if len(n.Cmds) > 0 {
    args := n.Cmds[0].Args
    if len(args) > 1 {
      return nil, ErrTooManyVariables
    }
    if _, ok := args[0].(*parse.FieldNode); ok {
      return []string{"."}, nil
    }
    if _, ok := args[0].(*parse.DotNode); ok {
      return []string{"."}, nil
    }
    return nil, ErrInvalid
  }
  return nil, ErrNotFound
}
//}

with句の条件部分に記述できる形式を@<list>{example_with_statements}に示します。

//listnum[example_with_statements][with句の条件式の例]{
{{ with .Foo }}
{{ with . }}
{{ with $x = 123 }}
{{ with $x := getFoo }}
{{ with $x := "huga" | println }}
//}

チェックする変数は@<code>{parse.WithNode}型の条件式部分を表現している@<code>{Pipe}フィールドから取り出します。
@<code>{Pipe}フィールドは@<code>{parse.PipeNode}型です。
@<code>{parse.PipeNode}型には@<code>{[]*parse.VariableNode}型の@<code>{Decl}フィールドと、@<code>{[]*parse.CommandNode}型の@<code>{Cmds}フィールドがあります。

@<list>{example_with_statements}の1〜2行目は@<code>{Cmds}フィールドから確認できます。
@<code>{Cmds}フィールドは@<code>{*parse.CommandNode}型のスライスです。
フィールドを複数記述した場合、最初の要素はメソッドとして扱われます。
このような場合は不適切な形式ですので、本章では無視します。

@<code>{*parse.CommandNode}型の引数を表す@<code>{Args}フィールドには、さまざまな型のノードが入ります。
その中でも@<code>{*parse.FieldNode}型もしくは@<code>{*parse.DotNode}型の２種類をチェックすれば必要な条件を探せます。
要素のフィールド名の場合はスコープが変化して@<code>{.}として扱われます。
なので検索する要素は@<code>{.}になります。

3行目から5行目のパターンは@<code>{Decl}フィールドから取り出します。
@<code>{Decl}フィールドは@<code>{*parse.VariableNode}型のスライスです。
@<code>{Decl}フィールドも、@<code>{Cmds}フィールドのように複数指定できません。
そのため、@<code>{Decl}フィールドも最初の要素を返すようにします。

with句で変数を定義した場合、ブロックの中では@<list>{example_with_variable_patterns}のように2種類の変数を利用できます。
どちらも@<tt>{foo}と表示されます。

//list[example_with_variable_patterns][with句のブロックで変数を使うパターン]{
{{ with $x = "foo" }}{{ $x }}{{end}}
{{ with $x = "foo" }}{{ . }}{{end}}
//}

このパターンを考慮する必要があります。
そのため、@<code>{*parse.VariableNode}型から変数を取得する場合は変数名とドットの2つを候補とします。

=== 変数が使われているかをチェックする
変数が実際に使われているかは@<code>{parse.WithNode}型の@<code>{List}フィールドをチェックします。
@<code>{List}フィールドは@<code>{*parse.ListNode}型で@<code>{parse.WithNode}型の@<code>{Pipe}フィールドが正の場合に実行されるノードです。

このノードを再帰的にチェックして入力された変数名が実際に使われているか調べます。
実装は
関数やパイプラインの引数をそれぞれチェックするのではなく、その引数のノードの型をチェックし、その中身をチェックします。
対象となるのは@<code>{*parse.FieldNode}型や@<code>{*parse.IdentifierNode}型などです。
それぞれの型によって比較する方法が異なります。

実装を@<list>{implementation_check_variable}@<fn>{link_check_variable}に示します。

//list[implementation_check_variable][変数が使われているかチェックする関数の実装]{
func checkVariable(list *parse.ListNode, variables []string) error {
  var found bool
  for _, target := range variables {
    templateutil.Inspect(list, func(node parse.Node) bool {
      f := false
      switch n := node.(type) {
      case *parse.FieldNode:
        v := "." + strings.Join(n.Ident, ".")
        f = strings.HasPrefix(v, target)
      case *parse.IdentifierNode:
        f = strings.HasPrefix(n.Ident, target)
      case *parse.VariableNode:
        v := strings.Join(n.Ident, ".")
        f = strings.HasPrefix(v, target)
      case *parse.ChainNode:
        v := "." + strings.Join(n.Field, ".")
        f = strings.HasPrefix(v, target)
      case *parse.DotNode:
        f = target == "."
      }
      found = found || f
      return true
    })
  }
  if found {
    return nil
  }
  return ErrNotFound
}
//}

//footnote[link_check_variable][@<href>{https://github.com/knsh14/withcheck/blob/master/check.go#L45-L73}]

@<code>{*parse.FieldNode}型や@<code>{*parse.VariableNode}型、@<code>{*parse.ChainNode}型は要素名がスライスになっているので、ドットでつなげてひとつにします。
@<code>{*parse.IdentifierNode}型はそのまま使います。
得られた対象の変数の先頭がチェックすべき変数と同じか調べます。
なぜなら、対象の変数の中のフィールドを使う可能性があるからです。

他の要素としては@<code>{*parse.DotNode}型があります。
このノードの場合は完全に同じかチェックします。

=== チェックの結果の返し方
with句でチェックした変数が@<code>{nil}でない場合にその変数が正しく使われているかチェックすることはできました。
最後にその結果をわかりやすく返す必要があります。

withcheckでは@<code>{error}型で返すようにしました。
なぜなら、@<code>{bool}型を使うよりも@<code>{error}型を利用することで失敗時の情報をより多く返すことができます。
自分たちで定義したエラーの型を使うことで利用者が、どこで失敗したか判別しやすくなります。

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
