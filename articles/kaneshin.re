= Goにおける初期化処理

本章では、Goでコードを書くときに知っておくと役立つ、コード実行時の初期化について紹介します。

Goでは@<code>{main}関数が実行されるまでにパッケージの変数や@<code>{init}関数を初期化して実行処理を効率的に行っています。Goにはファイル単位でのスコープは存在せず、パッケージのスコープで変数や関数が参照される仕様のため、コンパイラの仕様に依存して実行時の初期化が行われています。基本的には、このコンパイラの呼び出し順序の仕様に依存しない初期化や依存関係を考慮した実装をすべきです。よい設計をするために知っておいて損はない処理だと考えています。

== init関数

@<code>{init}関数は実行時の初期化で呼び出される特殊な関数です。

//list[init_function][init関数][go]{
package main

import "log"
import "os"

var logger = log.New(os.Stdout, "main.go: ", 0)

func init() {
  logger.Println("init")
}

func main() {
  logger.Println("main")
}
//}

//cmd{
$ go run main.go
main.go: init
main.go: main
//}

@<code>{main}関数が実行される前に定義された@<code>{init}関数が実行されます。@<code>{init}関数はこのように特殊な関数ですが、@<code>{init}関数を複数定義することも可能な特殊関数です。

//list[multiple_init_function][複数のinit関数][go]{
package main

import "log"
import "os"

var logger = log.New(os.Stdout, "main.go: ", 0)

func init() {
  logger.Println("1st init")
}

func init() {
  logger.Println("2nd init")
}

func main() {
  logger.Println("main")
}
//}

//cmd{
$ go run main.go
main.go: 1st init
main.go: 2nd init
main.go: main
//}

Goはパッケージスコープの仕様のため、同一パッケージ内にファイルが別であっても同じ関数名の関数を定義していた場合、ビルドは失敗します。それを実現しているのは、コンパイラが@<code>{init}関数に識別子を付与してビルドされているからです。

//list[multiple_init_function_renamed][init関数の識別子][go]{
package main

import "log"
import "os"
import "runtime"

var logger = log.New(os.Stdout, "main.go: ", 0)

var pcs [1]uintptr

func init() {
  runtime.Callers(1, pcs[:])
  logger.Println(runtime.FuncForPC(pcs[0]).Name())
}

func init() {
  runtime.Callers(1, pcs[:])
  logger.Println(runtime.FuncForPC(pcs[0]).Name())
}

func main() {}
//}

//cmd{
$ go run main.go
main.go: main.init.0
main.go: main.init.1
//}

アセンブリからも読み解くことができます。

//cmd{
$ go tool compile -S main.go | grep "init\.[0-9]\+" | head -n 6
"".init.0 STEXT size=224 args=0x0 locals=0x40
    0x0000 00000 (main.go:13)       TEXT    "".init.0(SB), ABIInternal, $64-0
    0x0021 00033 (main.go:13)       FUNCDATA        $3, "".init.0.stkobj(SB)
"".init.1 STEXT size=224 args=0x0 locals=0x40
    0x0000 00000 (main.go:18)       TEXT    "".init.1(SB), ABIInternal, $64-0
    0x0021 00033 (main.go:18)       FUNCDATA        $3, "".init.1.stkobj(SB)
//}

== パッケージの初期化

Goの作法ではパッケージの初期化と、パッケージ間の依存関係を意識することになります。実行時にパッケージの変数を初期化することによって変数の確保や処理時間を効率化できます。
パッケージの初期化が多いと@<code>{main}関数が実行されるまで待ち時間やメモリのスコープが関数ではなくパッケージスコープとなることによって変数を参照のコストが高くなることがあります。
このようなデメリットを理解したうえで初期化をすることは効率性を向上させるために重要です。

=== 初期化の順序

@<code>{init}関数の実行順序は上から順に実行されますが、複数のファイルやパッケージ変数、依存パッケージが存在するときの順序について知らない人もいるのではないでしょうか。パッケージの初期化はパッケージ変数の初期化の後に@<code>{init}関数の実行という順序で行われます。

//list[initialize_package_main][main.go][go]{
package main

import "runtime"

var _ = submain()

func init() {
  runtime.Callers(1, pcs[:])
  logger.main.Println(runtime.FuncForPC(pcs[0]).Name())
}

func submain() int {
  runtime.Callers(1, pcs[:])
  logger.a.Println(runtime.FuncForPC(pcs[0]).Name())
  return 0
}

func main() {
  runtime.Callers(1, pcs[:])
  logger.main.Println(runtime.FuncForPC(pcs[0]).Name())
}
//}

//list[initialize_package_a][a.go][go]{
package main

import "runtime"

var _ = suba()

func init() {
  runtime.Callers(1, pcs[:])
  logger.a.Println(runtime.FuncForPC(pcs[0]).Name())
}

func suba() int {
  runtime.Callers(1, pcs[:])
  logger.a.Println(runtime.FuncForPC(pcs[0]).Name())
  return 0
}
//}

//list[initialize_package_z][z.go][go]{
package main

import "runtime"

var _ = subz()

func init() {
  runtime.Callers(1, pcs[:])
  logger.z.Println(runtime.FuncForPC(pcs[0]).Name())
}

func subz() int {
  runtime.Callers(1, pcs[:])
  logger.z.Println(runtime.FuncForPC(pcs[0]).Name())
  return 0
}
//}

a.goやz.goのファイル名は、ファイル名の昇順でコンパイルされる仕様にのっとりわかりやすいものを定義しています。出力と呼び出された関数オブジェクトの名前を取得するために次の変数を定義しています。

//list[initialize_package_logger][][go]{
var logger = struct {
  a    *log.Logger
  z    *log.Logger
  main *log.Logger
}{
  a:    log.New(os.Stdout, "   a.go: ", 0),
  z:    log.New(os.Stdout, "   z.go: ", 0),
  main: log.New(os.Stdout, "main.go: ", 0),
}

var pcs [1]uintptr
//}

//cmd{
$ go build -o main . && ./main
   a.go: main.suba
   a.go: main.submain
   z.go: main.subz
   a.go: main.init.0
main.go: main.init.1
   z.go: main.init.2
main.go: main.main
//}

それぞれのファイルにある変数の初期化が処理され、その後に@<code>{init}関数が処理されています。@<code>{main}関数のあるファイルが最後に初期化処理されるわけではありません。特別なように見えるmain.goやinit.goというファイル名を作ってもファイル名による優先処理はなくファイル名の昇順でコンパイルがされます。

==== 依存パッケージの初期化

あるパッケージに別のパッケージが依存している場合、依存パッケージが先に初期化処理をするように実行されます。この初期化は再帰的に行われるため、依存の依存パッケージが存在すれば先に依存の依存パッケージの初期化処理がされます。

//list[initialize_package_dep_a][dep/a.go][go]{
package dep

import "log"
import "os"
import "runtime"

var logger = log.New(os.Stdout, " d/a.go: ", 0)

var pcs [1]uintptr

var _ = suba()

func init() {
  runtime.Callers(1, pcs[:])
  logger.Println(runtime.FuncForPC(pcs[0]).Name())
}

func suba() int {
  runtime.Callers(1, pcs[:])
  logger.Println(runtime.FuncForPC(pcs[0]).Name())
  return 0
}
//}

@<code>{import _ "github.com/kaneshin/foo/bar/dep"}のようにブランクインポートさせると次のように先に依存パッケージの初期化処理が行われます。

//cmd{
$ go build -o main . && ./main
 d/a.go: github.com/kaneshin/foo/bar/dep.suba
 d/a.go: github.com/kaneshin/foo/bar/dep.init.0
   a.go: main.suba
   a.go: main.submain
   z.go: main.subz
   a.go: main.init.0
main.go: main.init.1
   z.go: main.init.2
main.go: main.main
//}

==== 初期化のコントロール

初期化を完全に管理下に置きたい場合はinit.goなどにまとめるのがよいですが、変数や@<code>{init}関数、そして依存パッケージも上から順に再帰的に処理されます。ファイルが複数存在すると、順序関係を意識する必要のある初期化処理や、ほかの初期化処理に依存する初期化処理が必要となる設計はそもそも見直しが必要です。すべてを実行時に初期化処理できればよいですが、ある程度妥協することも設計の上では大事になります。複雑性を解決するためのGoが、逆に複雑性を上げてしまうことはよいとはいえません。

== パッケージスコープの変数宣言と初期化

パッケージスコープの変数を宣言させることはよいことなのか。Goは並行処理される言語のためパッケージスコープの変数へは競合状態を気にする必要があります。パッケージスコープな変数は@<code>{goroutine}間での処理を意識しながら開発と実行をしなくてはならなくなるため、密結合の温床であり依存性の高いコードとなるのであまり推奨されません。

しかし、パッケージスコープの変数は処理の効率性を上げる場合などによって必要不可欠です。そのためパッケージスコープの変数を宣言するときは設計として意味を定義しておくことが重要です。

=== エラーの変数宣言

エラーは値です。エラーをハンドリングするためには値として宣言された変数で検証をすることがエラーの適切な扱いへの一歩となります。

//list[init_var_error][strings.Readerのエラーハンドリング][go]{
package main

import "fmt"
import "io"
import "strings"

func main() {
  r := strings.NewReader("Hello")
  b := make([]byte, 8)
  for {
    n, err := r.Read(b)
    fmt.Printf("n = %v err = %v b = %v\n", n, err, b)
    if err == io.EOF {
      break
    }
  }
}
//}

//cmd{
$ go run main.go
n = 5 err = <nil> b = [72 101 108 108 111 0 0 0]
n = 0 err = EOF b = [72 101 108 108 111 0 0 0]
//}

これは@<code>{io}パッケージに宣言されている@<code>{io.EOF}でエラーをハンドリングしている例です。
@<code>{io}パッケージには@<code>{EOF}以外にも複数のエラーをパッケージスコープとして宣言しています。

//list[io_init_var_errors][ioパッケージのエラー][go]{
package io

import "errors"

var ErrShortWrite = errors.New("short write")
var ErrShortBuffer = errors.New("short buffer")
var EOF = errors.New("EOF")
var ErrUnexpectedEOF = errors.New("unexpected EOF")
var ErrNoProgress = errors.New("multiple Read calls return no data or error")
//}

パッケージスコープのエラーは定式的に@<code>{ErrNoProgress}のように@<code>{Err}を接頭辞とすることが多いです。

このようにハンドリングする必要のあるエラーは関数内で生成して返却するのではなく、パッケージスコープとして宣言したものを返却することでパッケージの外からハンドリングできます。

=== 高コストな処理を初期化する

Goの@<code>{regexp}パッケージは正規表現を扱うパッケージです。常に改善されているパッケージですが、関数内で何度も変数として初期化される場合は初期化のコストを実行直後に寄せてしまうことも可能です。

//list[regexp_benchmark][regexpパッケージのベンチマーク][go]{
package main

import "regexp"
import "testing"

func BenchmarkRegexpMustCompile1(b *testing.B) {
  b.ResetTimer()
  for i := 0; i < b.N; i++ {
    re0 := regexp.MustCompile("[a-z]{3}")
    _ = re0.FindAllString(`Lorem ipsum dolor sit amet, consectetur..`, -1)
  }
}

var re = regexp.MustCompile("[a-z]{3}")

func BenchmarkRegexpMustCompile2(b *testing.B) {
  b.ResetTimer()
  for i := 0; i < b.N; i++ {
    _ = re.FindAllString(`Lorem ipsum dolor sit amet, consectetur..`, -1)
  }
}
//}

//cmd{
$ go test -bench . -benchmem
BenchmarkRegexpMustCompile1-16 495222 2413 ns/op 1618 B/op 23 allocs/op
BenchmarkRegexpMustCompile2-16 838984 1391 ns/op  289 B/op  9 allocs/op
//}

これは@<code>{regexp}を関数呼び出し時に生成するか、実行時に変数を初期化してしまうかをシミュレートしたベンチマークです。結果は実行時に初期化する方が2倍高速に処理ができます。このように@<code>{regexp}は正規表現が動的に変化しない場合はパッケージスコープの変数として宣言してしまうことで効率的に処理ができます。

@<code>{regexp}は内部処理ではスレッドセーフに設計されていることや検索結果をキャッシュしているため、実際にコーディングする場合は次のように記述することを推奨します。

//list[regexp_practice][regexpのプラクティス][go]{
package main

import "fmt"
import "regexp"

var re = regexp.MustCompile("[a-z]{3}")

func FindString(str string) []string {
  re := re.Copy()
  return re.FindAllString(str, -1)
}

func main() {
  fmt.Println(FindString("Hello, world"))
}
//}

//cmd{
$ go run main.
[ell wor]
//}

また、パッケージ変数として宣言している@<code>{re}変数を上書きされたくない場合は初期化時に関数実行をして隠蔽することもできます。

//list[regexp_practice_closure][regexpのプラクティス][go]{
var FindString = func() func(string) []string {
  re := regexp.MustCompile("[a-z]{3}")
  return func(str string) []string {
    re := re.Copy()
    return re.FindAllString(str, -1)
  }
}()

func main() {
  fmt.Println(FindString("Hello, world"))
}
//}

//cmd{
$ go run main.
[ell wor]
//}

このように宣言することによって、パッケージスコープの変数を減らすことができます。また、@<code>{re}変数が関数スコープになるためメモリアクセスも効率化されています。

=== その他

他にもさまざまな条件下で有効に活用されるパッケージスコープの変数が存在します。もちろん、ここに紹介していることに限らないのでメモリ効率のためには臆せず使うことも考えてみてください。

==== メモリキャッシュ

メモリキャッシュをするときもパッケージスコープに@<code>{map}や@<code>{map}を変数とした@<code>{struct}を変数としてキャッシュにすることもあります。並行処理で使われることが前提になるため競合状態のハンドリングを忘れずに実装する必要があります。@<code>{sync.Map}などを参考にしてみてください。

==== 読み取り専用のテーブル

@<code>{unicode}パッケージには様々な文字に対応するためのUnicodeテーブル@<fn>{unicode_tables}が定義されています。
//footnote[unicode_tables][@<href>{https://golang.org/src/unicode/tables.go}]

== おわりに

Goにおける初期化について、パッケージスコープの変数と@<code>{init}関数、初期化の順序について紹介しました。
これらを気にすることなくコーディングすることは可能ですが、細かいところまでケアしてコードを書くことによってメモリや処理の最適化がされますし、開発における設計の複雑性除去の判断にもつながると筆者は考えています。

#@# textlint-enable
