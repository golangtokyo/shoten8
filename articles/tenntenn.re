= Go1.14で導入されたdeferのインライン展開

@<code>{defer}とは関数呼び出しを遅延させるGoの機能です。
@<code>{defer}文で予約された関数呼び出しは、
@<code>{defer}文が記述された関数の終了時に呼び出されます。

本章では、@<code>{defer}の基本から@<code>{defer}の仕組み、そして
Go1.14で導入された@<code>{defer}分のインライン展開について解説します。

== deferとは

@<code>{defer}は、ファイルなどのリソースの開放やロックの解除など、
忘れてしまうと問題になる場合に使うと便利です。

たとえば、関数終了時にファイルを閉じたい場合は、
@<list>{defer-file}のように記述できます。

//list[defer-file][関数終了時にファイルを閉じる][go]{
func main() {
  f, err := os.Open("file.txt")
  if err != nil {
    fmt.Fprintln(os.Stderr, "Error:", err)
    os.Exit(1)
  }
  defer f.Close()

  // ...(略)...
}
//}

@<code>{defer f.Close()}はすぐに実行される訳ではなく、
@<code>{main}関数終了時に実行されます。

1つの関数に複数@<code>{defer}文が記述できます。
その場合、上から下に記述した順に呼び出される訳ではありません。
最初に書かれた@<code>{defer}文の関数呼び出しが最後に、
最後に書かれた@<code>{defer}文の関数呼び出しが最初に実行されます。
つまり、スタック（LIFO; Last In Fist Out）形式になります。

また、@<code>{defer}文で関数実行を遅延させる際の引数は、
実行時に評価される訳ではなく@<code>{defer}文が実行された時点で評価されます。

たとえば、@<list>{defer-multi}のように1つの関数で
複数の@<code>{defer}文を記述した場合を考えます。

//list[defer-multi][複数のdefer文と引数の扱い][go]{
func main() {
  msg := "!!!"
  defer fmt.Println(msg)
  msg = "world"
  defer fmt.Println(msg)
  fmt.Println("hello")
}
//}

@<code>{defer}文で呼び出される@<code>{fmt.Println}関数は
引数として変数@<code>{msg}の値を取ります。

変数@<code>{msg}の値は、@<code>{main}関数中で変化していきます。
実際に@<code>{defer}文で遅延実行される@<code>{fmt.Println}関数の引数となるのは、
1つ目が@<code>{"!!!"}で2つ目が@<code>{"world"}になります。

そのため、@<list>{defer-multi}を実行すると次のような結果になります。
@<code>{defer}文で呼び出されていない@<code>{fmt.Println("hello")}が最初に実行され、
次に引数@<code>{"world"}で実行され、
そして最後に引数@<code>{"!!!"}で@<code>{fmt.Println}関数の実行されます。

//cmd{
hello
world
!!!
//}

== deferのアンチパターン

@<code>{defer}文で予約された関数呼び出しは、
@<code>{defer}文が記述された関数の終了時に呼び出されます。
そのため、@<code>{for}文の繰り返し処理で@<code>{defer}文を用いても、
関数終了時まで実行されません。

たとえば、@<list>{defer-for}のように繰り返し処理でファイルを開き、
@<code>{defer}文でファイルを閉じています。

//list[defer-for][繰り返し処理でdefer文を用いるアンチパターン][go]{
func main() {
  fileNames := os.Args[1:]
  for _, fn := range fileNames {
    f, err := os.Open(fn)
      if err != nil {
        // エラー処理
      }
    defer f.Close()
    // fを使った処理
  }
}
//}

@<code>{defer f.Close()}で予約された@<code>{f.Close()}の呼び出しは、
1回の繰り返しの度に呼ばれる訳ではなく、関数の終了時に呼ばれます。
そのため、不要になったファイルでも関数が終了するまでは開きっぱなしになってしまいます。

そこで、@<list>{defer-for-func}のように、ファイルを開いて閉じるまでの処理を関数に抜き出します。
関数単位になることでファイルを使わなくなった時点で閉じることができます。

//list[defer-for-func][関数に分けてdeferを呼び出す][go]{
func main() {
  fileNames := os.Args[1:]
  for _, fn := range fileNames {
    if err := readFile(fn); err != nil {
	  /* エラー処理 */
	}
  }
}

func readFile(fn string) error {
  f, err := os.Open(fn)
  if err != nil {
    return err
  }
  // readFile関数が終了する際に呼ばれる
  defer f.Close()

  // fを使った処理  
}
//}

== deferの仕組み

@<code>{defer}文で予約される関数呼び出しは、
どのような仕組みで遅延実行されているのでしょうか。

@<code>{defer}文を使った遅延実行は、2つの処理に分けられます。
1つ目は、関数と引数を登録する処理で、ランタイムで定義されている
@<code>{runtime.deferproc}関数が用いられます。
@<code>{runtime.deferproc}関数は、遅延実行させる関数や引数へのポインタをヒープに確保します。
しかし、Go1.13からは可能な場合は@<code>{runtime.deferprocStack}関数を用いることによって、
スタックに確保するようにしています。

2つ目は、実際に予約した関数を実行する処理で、@<code>{runtime.deferreturn}関数で行れます。
@<code>{defer}文が複数ある場合は、逆順で@<code>{runtime.deferreturn}が実行されます。

たとえば、@<list>{defer-a-b}のように2つの関数、@<code>{a}と@<code>{b}を
それぞれ引数@<code>{10}と@<code>{20}で@<code>{defer}文で呼び出していた場合を考えます。

//list[defer-a-b][関数aとbをdefer文で呼び出す][go]{
func f() {
  defer a(10)
  defer b(20)

  // ...(略)...
}
//}

@<list>{defer-a-b}のコードは、
コンパイラによって@<list>{deferprocStack-deferretrun}のような処理に分解されます。
@<list>{deferprocStack-deferretrun}に示したコードは擬似コードですが、
コンパイラが生成するオブジェクトコードも似たような処理になります。

//list[deferprocStack-deferretrun][runtime.deferprocStack関数とruntime.deferretrun関数][go]{
// ※このコードは擬似コード

runtime.deferprocStack(a)
runtime.deferprocStack(b)

// ...(略)...

runtime.deferreturn(b)
runtime.deferreturn(a)
//}

== 逆アセンブルして比較する

Goコンパイラによって生成されたバイナリを逆アセンブルすることで、
@<code>{runtime.deferproc}関数や@<code>{runtime.deferretrun}関数が
呼ばれているか確認できます。

たとえば、@<list>{simple}のようなコードをビルドし、
@<code>{go tool objdump}コマンドで逆アセンブルを行います。

//list[simple][defer文で関数を呼び出すシンプルな例][go]{
package main

func main() {
  defer print("hoge")
}
//}

@<code>{go tool objdump}コマンドは
次のように@<code>{-s}オプションで正規表現を指定することで
マッチするシンボルだけを逆アセンブルすることができます。@<fn>{objdump}

//footnote[objdump][@<href>{https://golang.org/cmd/objdump/}]

//cmd{
$ go1.12.17 build -o main.o /tmp/main.go
$ go1.12.17 tool objdump -s "main\.main$" main.o
TEXT main.main(SB) /tmp/main.go
  main.go:3  0x104ea20  65488b0c2530000000  MOVQ GS:0x30, CX
  main.go:3  0x104ea29  483b6110            CMPQ 0x10(CX), SP
  main.go:3  0x104ea2d  765f                JBE 0x104ea8e
  main.go:3  0x104ea2f  4883ec28            SUBQ $0x28, SP
  main.go:3  0x104ea33  48896c2420          MOVQ BP, 0x20(SP)
  main.go:3  0x104ea38  488d6c2420          LEAQ 0x20(SP), BP
  main.go:4  0x104ea3d  c7042410000000      MOVL $0x10, 0(SP)
  main.go:4  0x104ea44  488d05ad2a0200      LEAQ go.func.*+60(SB), AX
  main.go:4  0x104ea4b  4889442408          MOVQ AX, 0x8(SP)
  main.go:4  0x104ea50  488d0596e60100      LEAQ go.string.*+237(SB), AX
  main.go:4  0x104ea57  4889442410          MOVQ AX, 0x10(SP)
  main.go:4  0x104ea5c  48c744241804000000  MOVQ $0x4, 0x18(SP)
  main.go:4  0x104ea65  e8b631fdff          CALL runtime.deferproc(SB)
  main.go:4  0x104ea6a  85c0                TESTL AX, AX
  main.go:4  0x104ea6c  7510                JNE 0x104ea7e
  main.go:5  0x104ea6e  90                  NOPL
  main.go:5  0x104ea6f  e83c3afdff          CALL runtime.deferreturn(SB)
  main.go:5  0x104ea74  488b6c2420          MOVQ 0x20(SP), BP
  main.go:5  0x104ea79  4883c428            ADDQ $0x28, SP
  main.go:5  0x104ea7d  c3                  RET
  main.go:4  0x104ea7e  90                  NOPL
  main.go:4  0x104ea7f  e82c3afdff          CALL runtime.deferreturn(SB)
  main.go:4  0x104ea84  488b6c2420          MOVQ 0x20(SP), BP
  main.go:4  0x104ea89  4883c428            ADDQ $0x28, SP
  main.go:4  0x104ea8d  c3                  RET
  main.go:3  0x104ea8e  e8cd84ffff          CALL runtime.morestack_noctxt(SB)
  main.go:3  0x104ea93  eb8b                JMP main.main(SB)
  :-1        0x104ea95  cc                  INT $0x3
  :-1        0x104ea96  cc                  INT $0x3
  :-1        0x104ea97  cc                  INT $0x3
  :-1        0x104ea98  cc                  INT $0x3
  :-1        0x104ea99  cc                  INT $0x3
  :-1        0x104ea9a  cc                  INT $0x3
  :-1        0x104ea9b  cc                  INT $0x3
  :-1        0x104ea9c  cc                  INT $0x3
  :-1        0x104ea9d  cc                  INT $0x3
  :-1        0x104ea9e  cc                  INT $0x3
  :-1        0x104ea9f  cc                  INT $0x3
//}

なお、@<code>{go1.12.17}コマンドは@<code>{go}コマンドのバージョンが1.12.17であることを示しています。
バージョンごとのGoのツールチェインをインストールしたい場合、次のようなコマンドを実行すると
必要なものがダウンロードとインストールされます。

//cmd{
$ go get golang.org/dl/go1.12.17
$ go1.12.17 download
//}

Go1.12では、@<code>{runtime.deferprocStack}関数が導入されていないため、@<code>{runtime.deferproc}関数が呼ばれていることが分かります。

一方、Go1.13のコンパイラでコンパイルすると、@<code>{runtime.deferprocStack}関数が用いられていることが分かります。

//cmd{
$ go1.13.8 build -o main.o /tmp/main.go
$ go1.13.8 tool objdump -s "main\.main$" main.o
TEXT main.main(SB) /tmp/main.go
  main.go:3  0x1051560  65488b0c2530000000  MOVQ GS:0x30, CX
  main.go:3  0x1051569  483b6110            CMPQ 0x10(CX), SP
  main.go:3  0x105156d  7669                JBE 0x10515d8
  main.go:3  0x105156f  4883ec58            SUBQ $0x58, SP
  main.go:3  0x1051573  48896c2450          MOVQ BP, 0x50(SP)
  main.go:3  0x1051578  488d6c2450          LEAQ 0x50(SP), BP
  main.go:4  0x105157d  c744241010000000    MOVL $0x10, 0x10(SP)
  main.go:4  0x1051585  488d057c440200      LEAQ go.func.*+64(SB), AX
  main.go:4  0x105158c  4889442428          MOVQ AX, 0x28(SP)
  main.go:4  0x1051591  488d0575fd0100      LEAQ go.string.*+237(SB), AX
  main.go:4  0x1051598  4889442440          MOVQ AX, 0x40(SP)
  main.go:4  0x105159d  48c744244804000000  MOVQ $0x4, 0x48(SP)
  main.go:4  0x10515a6  488d442410          LEAQ 0x10(SP), AX
  main.go:4  0x10515ab  48890424            MOVQ AX, 0(SP)
  main.go:4  0x10515af  e8dc1ffdff          CALL runtime.deferprocStack(SB)
  main.go:4  0x10515b4  85c0                TESTL AX, AX
  main.go:4  0x10515b6  7510                JNE 0x10515c8
  main.go:5  0x10515b8  90                  NOPL
  main.go:5  0x10515b9  e8d225fdff          CALL runtime.deferreturn(SB)
  main.go:5  0x10515be  488b6c2450          MOVQ 0x50(SP), BP
  main.go:5  0x10515c3  4883c458            ADDQ $0x58, SP
  main.go:5  0x10515c7  c3                  RET
  main.go:4  0x10515c8  90                  NOPL
  main.go:4  0x10515c9  e8c225fdff          CALL runtime.deferreturn(SB)
  main.go:4  0x10515ce  488b6c2450          MOVQ 0x50(SP), BP
  main.go:4  0x10515d3  4883c458            ADDQ $0x58, SP
  main.go:4  0x10515d7  c3                  RET
  main.go:3  0x10515d8  e81381ffff          CALL runtime.morestack_noctxt(SB)
  main.go:3  0x10515dd  eb81                JMP main.main(SB)
  :-1        0x10515df  cc                  INT $0x3
//}

また、@<code>{runtime.deferproc}関数や@<code>{runtime.deferprocStack}関数はランタイムで提供されている関数であるため、次のようにコンパイラによって生成されるバイナリにも含まれています。

//cmd{
# Go1.12のコンパイラが生成するバイナリを逆アセンブルする
$ go1.12.17 build -o main.o /tmp/main.go
$ go1.12.17 tool objdump -s "runtime\.defer" main.o | grep -A 4 TEXT
TEXT runtime.deferproc(SB) ~/sdk/go1.12.17/src/runtime/panic.go
  panic.go:92   0x1021c20  4883ec28            SUBQ $0x28, SP
  panic.go:92   0x1021c24  48896c2420          MOVQ BP, 0x20(SP)
  panic.go:92   0x1021c29  488d6c2420          LEAQ 0x20(SP), BP
  panic.go:93   0x1021c2e  65488b042530000000  MOVQ GS:0x30, AX
--
TEXT runtime.deferreturn(SB) ~/sdk/go1.12.17/src/runtime/panic.go
  panic.go:345  0x10224b0  4883ec40            SUBQ $0x40, SP
  panic.go:345  0x10224b4  48896c2438          MOVQ BP, 0x38(SP)
  panic.go:345  0x10224b9  488d6c2438          LEAQ 0x38(SP), BP
  panic.go:346  0x10224be  65488b0c2530000000  MOVQ GS:0x30, CX

# Go1.13のコンパイラが生成するバイナリを逆アセンブルする
$ go1.13.8 build -o main.o /tmp/main.go
$ go1.13.8 tool objdump -s "runtime\.defer" main.o | grep -A 4 TEXT
TEXT runtime.deferprocStack(SB) ~/sdk/go1.13.8/src/runtime/panic.go
  panic.go:255  0x1023590  4883ec18            SUBQ $0x18, SP
  panic.go:255  0x1023594  48896c2410          MOVQ BP, 0x10(SP)
  panic.go:255  0x1023599  488d6c2410          LEAQ 0x10(SP), BP
  panic.go:256  0x102359e  65488b042530000000  MOVQ GS:0x30, AX
--
TEXT runtime.deferreturn(SB) ~/sdk/go1.13.8/src/runtime/panic.go
  panic.go:502  0x1023b90  4883ec40            SUBQ $0x40, SP
  panic.go:502  0x1023b94  48896c2438          MOVQ BP, 0x38(SP)
  panic.go:502  0x1023b99  488d6c2438          LEAQ 0x38(SP), BP
  panic.go:503  0x1023b9e  65488b0c2530000000  MOVQ GS:0x30, CX
//}

なお、@<list>{simple}をGo1.13でビルドすると@<code>{runtime.deferproc}関数が使用されないため、
生成されるバイナリには含まれません。

== インライン展開

Go1.13で導入された@<code>{runtime.deferprocStack}関数を用いた最適化では、
多くの場合30%ほど効率化されるようになりました。@<fn>{go113}

//footnote[go113][@<href>{https://golang.org/doc/go1.13#runtime}]

しかし、@<code>{runtime.deferproc}関数や@<code>{runtime.deferprocStack}関数、
@<code>{runtime.deferreturn}関数はあくまでランタイムで提供される関数です。
そのため、コンパイラによってさらに最適化されることは期待できません。

そこで、2020年2月に公開されたGo1.14では、@<code>{defer}文をインライン展開することに
よりさらに効率的なオブジェクトコードへ最適化を行います。
@<fn>{go114-defer-proposal}@<fn>{go114-defer-cl}@<fn>{go114}

//footnote[go114-defer-proposal][@<href>{https://github.com/golang/proposal/blob/master/design/34481-opencoded-defers.md}]
//footnote[go114-defer-cl][@<href>{https://go-review.googlesource.com/c/go/+/190098}]
//footnote[go114][@<href>{https://golang.org/doc/go1.14#runtime}]

たとえば、@<list>{go114-defer}のようなコードがあった場合を考えます。
@<code>{defer f1(a)}は無条件で@<code>{defer f2(b)}は
@<code>{cond}が@<code>{true}になる場合に実行されます。

//list[go114-defer][コンパイル前のdefer文を使ったコード][go]{
defer f1(a)

if cond {
  defer f2(b)
}

// ...(略)...
//}

@<list>{go114-defer}をGo1.14のコンパイラでコンパイルすると、
@<list>{go114-defer-compiled}のようにインライン展開されます。

//list[go114-defer-compiled][インライン展開後の擬似コード][go]{
// ※このコードは擬似コード

deferBits |= 1<<0
tmpF1 = f1
tmpA  = a
if cond {
  deferBits |= 1<<1
  tmpF2 = f2
  tmpB  = b
}

// ...(略)...

exit:
if deferBits & 1<<1 != 0 {
  deferBits &^= 1<<1
  tmpF2(tmpB)
}

if deferBits & 1<<0 != 0 {
  deferBits &^= 1<<0
  tmpF1(tmpA)
}
//}

@<code>{defer}文を見つけると@<code>{deferBits}変数のビットを立てていき、
関数と引数を一時的な変数に退避させておきます。

そして、関数終了時に@<code>{deferBits}変数のビットが立っている
@<code>{defer}文で予約された関数だけ退避させておいた引数の値で実行します。

このとき、@<code>{defer f2(b)}で予約した関数呼び出しの方が先に
実行されるようにインライン展開が行われることに注意してください。

@<list>{go114-defer-compiled}で示したコードは擬似コードです。
そのため、実際にこのようなコードがコンパイラによって生成される訳ではありません。

そこで、@<list>{defer-8}のコードをコンパイルしたバイナリで
@<code>{runtime.deferprocStack}関数の呼び出しがなくなっているか
@<code>{go tool objdump}コマンドを用いて確かめてみましょう。

//list[defer-8][8つのdefer文がある関数と9つのdefer文がある関数][go]{
package main

func main() {
  // 8つのdefer文がある
  defer func() {}()
  defer func() {}()
  defer func() {}()
  defer func() {}()
  defer func() {}()
  defer func() {}()
  defer func() {}()
  defer func() {}()
}

// 呼び出しておこないと最適化によって消される
var _ = f()

func f() bool {
  // 9つのdefer文がある
  defer func() {}()
  defer func() {}()
  defer func() {}()
  defer func() {}()
  defer func() {}()
  defer func() {}()
  defer func() {}()
  defer func() {}()
  defer func() {}()

  return true
}
//}

@<code>{main}関数を逆アセンブルし、@<code>{grep}コマンドで
その結果に@<code>{deferproc}という文字列が
含まれているか調べてみると次のような結果になります。

//cmd{
$ go1.14 build -o main.o /tmp/main.go
$ go1.14 tool objdump -s "main\.main$" main.o | grep "deferproc"
//}

@<code>{grep}コマンドによって何も表示されないため、@<code>{main}関数に記述された
@<code>{defer}文はすべてインライン展開されていることが分かります。

一方、関数@<code>{f}を逆アセンブルし、@<code>{grep}コマンドで
その結果に@<code>{deferproc}という文字列が
含まれているか調べてみると次のような結果になります。

//cmd{
$ go1.14 tool objdump -s "main\.f$" main.o | grep "deferproc"
  main.go:17  0x1057200  e8bb00fdff  CALL runtime.deferprocStack(SB)
  main.go:18  0x1057233  e88800fdff  CALL runtime.deferprocStack(SB)
  main.go:19  0x1057266  e85500fdff  CALL runtime.deferprocStack(SB)
  main.go:20  0x1057299  e82200fdff  CALL runtime.deferprocStack(SB)
  main.go:21  0x10572cc  e8effffcff  CALL runtime.deferprocStack(SB)
  main.go:22  0x10572ff  e8bcfffcff  CALL runtime.deferprocStack(SB)
  main.go:23  0x1057332  e889fffcff  CALL runtime.deferprocStack(SB)
  main.go:24  0x105735c  e85ffffcff  CALL runtime.deferprocStack(SB)
  main.go:25  0x1057382  e839fffcff  CALL runtime.deferprocStack(SB)
//}

@<code>{grep}コマンドによってヒットした行が9つあります。
つまり、関数@<code>{f}に記述されている9つの@<code>{defer}文は
インライン展開されていないことが分かります。

Go1.14の@<code>{defer}文のインライン展開は、
1つの関数で9つ以上@<code>{defer}文があるとインライン展開しないことが分かります。

また、@<code>{defer}文が@<code>{for}文の繰り返し処理の中で
記述されていてもインライン展開されません。

@<list>{defer-for-simple}のようなコードをGo1.14のコンパイラで
コンパイルし、生成されたバイナリを@<code>{go tool objdump}コマンドで
逆アセンブルして確かめてみましょう。

//list[defer-for-simple][繰り返し処理でdefer文を記述した場合][go]{
package main

func main() {
  for i := 0; i < 3; i++ {
    defer func() {}()
  }
}
//}

逆アセンブルした結果は次のようになります。

//cmd{
$ go1.14 build -o main.o /tmp/main.go
$ go1.14 tool objdump -s "main\.main$" main.o | grep "deferproc"
  main.go:5  0x10570d8  e8e301fdff  CALL runtime.deferproc(SB)
//}

@<code>{for}文の繰り返し処理で記述されている@<code>{defer}文は
インライン展開されないことが分かります。
このようなパターンはそもそもアンチパターンであることは前述した通りであるため、
多くの場合はインライン展開の恩恵が受けられます。

インライン展開された@<code>{defer}文は、@<code>{runtime.deferproc}関数や
@<code>{runtime.deferprocStack}関数、@<code>{runtime.deferreturn}関数などの
ランタイム関数を用いているわけではないため、さらなるコンパイラによる最適化が期待できます。

== ベンチマークで比較する

さて、@<code>{defer}文のインライン展開によってどの程度パフォーマンスがよくなったのでしょうか。
ここでは、簡単なベンチマークをとって比較してみます。

ベンチマークを行うのは@<list>{bench}に示した3つの関数です。

//list[bench][ベンチマーク対象のコード][go]{
package go114defer

import "sync"

func DeferUnlock() {
  var m sync.Mutex
  m.Lock()
  defer m.Unlock()
}

func NoDeferUnlock() {
  var m sync.Mutex
  m.Lock()
  m.Unlock()
}

func ForDeferUnlock() {
  for {
    var m sync.Mutex
    m.Lock()
    defer m.Unlock()
    break
  }
}
//}

@<code>{DeferUnlock}関数は@<code>{*sync.Mutex}型の
@<code>{Lock}メソッドを呼び出してロックを取った後に
@<code>{defer}文で@<code>{Unlock}メソッドを呼び出してアンロックしています。

@<code>{NoDeferUnlock}関数は、@<code>{defer}文を使わずに
@<code>{Unlcok}メソッドを呼び出しています。

@<code>{ForDeferUnlock}関数は、@<code>{for}文の繰り返し処理の中で@<code>{defer}文を
用いて@<code>{Unlock}メソッドを呼び出しています。

これらの関数のベンチマークを行うテストコードを@<list>{bench-test}に示します。

//list[bench-test][ベンチマークを行うテストコード][go]{
package go114defer

import "testing"

func BenchmarkDeferUnlock(b *testing.B) {
  b.ResetTimer()
  for i := 0; i < b.N; i++ {
    DeferUnlock()
  }
}

func BenchmarkNoDeferUnlock(b *testing.B) {
  b.ResetTimer()
  for i := 0; i < b.N; i++ {
    NoDeferUnlock()
  }
}

func BenchmarkForDeferUnlock(b *testing.B) {
  b.ResetTimer()
  for i := 0; i < b.N; i++ {
    ForDeferUnlock()
  }
}
//}

ベンチマークは次のように@<code>{go test}コマンドに
@<code>{-bench}オプションをつけて実行します。

//cmd{
$ go test -bench .
//}

@<img>{tenntenn/chart}にGoのバージョンごとのベンチマーク結果をグラフにしたものを示します。
なお、ベンチマーク結果は筆者のMacBook Airで行ったものです。

//image[tenntenn/chart][バージョンごとのベンチマーク結果の比較]

Go1.14では@<code>{defer}文がインライン展開されるため、
@<code>{defer}文を使わない場合と大きな違いがなくなっています。
また、@<code>{ForDeferUnlock}関数は@<code>{for}文の繰り返し処理の中で
@<code>{defer}文を使用しているため、インライン展開されずGo1.13までと
あまり変わらない結果になっています。

== 静的単一代入形式で比較する

インライン展開が行われたかどうかは、バイナリを逆アセンブルしても分かりますが、
ここでは静的単一代入（SSA; Static Single Assignment）形式を用いて調べてみましょう。

静的単一代入形式とは、コンパイラの最適化で用いられる形式で、
変数への代入を1度だけになるように制限した形式になります。
静的単一代入形式にすることで最適化がしやすくなるため、
Goのコンパイラでも1.7から導入されました。@<fn>{ssa}

//footnote[ssa][@<href>{https://golang.org/doc/go1.7#compiler}]

Goのコンパイラでは、コードを一度、静的単一代入形式で表現し、
そこに順に最適化処理をかけていき、最終的なオブジェクトコード（バイナリ）を生成します。

コンパイル時に生成された静的単一代入形式をダンプしたい場合には、
次のように@<code>{go tool compile}コマンドの@<code>{-d}オプションに
@<code>{ssa/build/dump}を指定すると行えます。

//cmd{
$ go1.13.8 tool compile -d 'ssa/build/dump=main' /tmp/main.go
$ grep defer main_01__build.dump
  v17 = StaticCall <mem> {runtime.deferprocStack} [16] v16
  v21 = StaticCall <mem> {runtime.deferreturn} v20
  v19 = StaticCall <mem> {runtime.deferreturn} v18
//}

このように実行すると、@<code>{main}関数を静的単一代入形式で表現されたものが
@<code>{main_01__build.dump}というファイルにダンプされます。
@<code>{grep}コマンドで@<code>{defer}という文字列が含まれている部分を絞ると、
@<code>{runtime.deferproc}関数や@<code>{runtime.deferprocStack}関数、
@<code>{runtime.deferreturn}関数などが呼ばれているか調べてることができます。

同様のコマンドをGo1.14のコンパイラで実行すると
@<code>{defer}文がインライン展開されるため@<code>{grep}コマンドで
何もヒットしなくなります。

//cmd{
$ go1.14 tool compile -d 'ssa/build/dump=main' /tmp/main.go
$ grep defer main_01__build.dump
//}

最適化の過程の静的単一代入形式を取得したい場合は、コンパイルする際に
次のように@<code>{GOSSAFUNC}環境変数を指定すると@<code>{ssa.html}ファイルが生成され、
最適化の過程を見ることができます。

//cmd{
$ GOSSAFUNC=main go1.13.8 build /tmp/main.go
//}

@<code>{ssa.html}ファイルをブラウザで開くと@<img>{tenntenn/ssahtml}のような画面が開きます。

sourcesの部分にソースコードが表示されており、最適化のフェーズによって生成される静的単一代入形式のコードが各アコーディオンペインに表示されます。

ソースコードの文をクリックすると@<img>{tenntenn/ssahtml}のように、
対応する静的単一代入形式の命令が同じ色でハイライトされます。

//image[tenntenn/ssahtml][静的単一代入形式]

静的単一代入形式レベルでもGo1.13では@<code>{runtime.deferprocStack}関数が呼ばれていることが分かります。

== おわりに

本章では、@<code>{defer}の仕組みやインライン展開について解説を行いました。
また、逆アセンブルや静的単一代入形式のダンプの方法を解説し、
バージョンごとに最適化が加えられていることが分かりました。

ベンチマークを取ることにより、客観的な視点でパフォーマンスが
改善しているということを知ることがでました。

Goのコンパイラにはこのような形で最適化や改善が日々なされています。
読者のみなさんも興味のあるアップデートをみつけて、
コードリーディングやベンチマークなどを通して深く理解してみると
楽しいのではないでしょうか。
