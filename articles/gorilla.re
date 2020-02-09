= TUIツールを作ろう

初めまして、ゴリラ@<fn>{about_gorilla}です。
本章はGoを使ったTUI（Text-based User Interface）ツールの作り方について解説していきます。

//footnote[about_gorilla][https://twitter.com/gorilla0513]

== TUIとは
TUIは簡潔に説明しますと端末上で動作するGUIのようなインタフェースのことです。
GUI（Graphical User Interface）と同様、マウスとキーボードを使用できます。

TUIツールとはそのようなインタフェースを持つツールのことを指します。
よくCLI（Command Line Interface）と混合されますが、CLIは名前のとおりコマンドを使って操作するインタフェースです。
たとえば@<code>{ls}などもCLIの一種です。

TUIがCLIと混同されがちな理由は、おそらくどちらもコマンドを使って実行するからなのではないかと考えられます。

=== TUIのメリットとデメリット
TUIツールを使う一番のメリットは、直感的でシンプルに操作できるところです。

たとえば、@<img>{gorilla/about_tui_tool}は筆者が作成したdockerを操作できるdocuiというTUIツールですが、
@<code>{u}でコンテナを起動、@<code>{d}コンテナを削除できます。
これをCLIで行うと次のコマンドを入力して実行する必要があります。

//cmd{
# コンテナの停止
$ docker stop {container name or id}

# コンテナの削除
# docker rm {container name or id}
//}

このように、CLIでは都度コマンドを入力して実行という手順を踏む必要があって面倒です。
そのような面倒を解消するために筆者はTUIツールをよく作っています。

しかし、TUIにもデメリットがあります。TUIはキーボードまたはマウスの入力待ちが発生するため自動化はかなり困難です。
対してCLIは一般的に標準入出力を使うだけですので、自動化に向いています。

このように、TUIとCLIはそれぞれ活かせる場面がありますので、その使い分けもまた大事と筆者は考えています。

//image[gorilla/about_tui_tool][筆者が作ったdockerのTUIツール][scale=0.9]

== TUIツールの紹介
TUIについて説明したところで、どんなTUIツールがあるのかを紹介します。

=== docui
TUIとはの節で軽く紹介しましたが、あらためて紹介します。
docui@<fn>{about_docui}はdockerの操作をシンプルに、そして初心者でも使いやすいTUIツールです。

主な機能は次です。

 * イメージの検索、取得、削除、インポート、セーブ、ロード
 * コンテナの作成、起動、停止、削除、アタッチ、エクスポート
 * ネットワーク、ボリュームの削除

docuiでは各リソース（イメージやコンテナ）をパネルごとに表示しています。
各パネルでのキーを使って操作します。

たとえばイメージのパネルでは、@<code>{c}でコンテナの@<img>{gorilla/docui-create-container}のように作成画面が表示され、必要な項目を入力してコンテナを作成できます。
また、@<code>{f}で入力画面が表示され、キーワードを入力してEnterを押すと@<img>{gorilla/docui-search}のようにイメージの検索結果一覧を見れます。

//image[gorilla/docui-create-container][コンテナ作成][scale=0.9]
//image[gorilla/docui-search][イメージの検索][scale=0.9]

//footnote[about_docui][https://github.com/skanehira/docui]

=== lazygit
lazygit@<fn>{about_lazygit}はGitのTUIツールです。Gitコマンドをラップしてより便利になっています。

lazygitはファイルの差分確認やステージ追加、コミットの差分確認などを行うときに便利です。
@<img>{gorilla/lazygit-diff}はコミットの差分を表示する画面です。
@<img>{gorilla/lazygit-staging}はファイルの行単位の差分をステージに追加している画面です。

//image[gorilla/lazygit-diff][lazygit][scale=0.9]
//image[gorilla/lazygit-staging][lazygit][scale=0.9]

//footnote[about_lazygit][https://github.com/jesseduffield/lazygit]

=== JSON
JSONといえばjq@<fn>{about_jq}というCLIツールが有名です。
しかし、jqはインタラクティブに値を絞り込みできないので、筆者はJSONをインタラクティブに操作できるtson@<fn>{about_tson}というTUIツールを作りました。

tsonの主な機能は次になります。

 * JSONをツリー状で表示し、キーと値の絞り込みができる
 * ファイルまたは標準入力からJSON文字列を受け取ることができる
 * 編集できる

@<img>{gorilla/tson-edit}はJSONを編集している様子です。

//image[gorilla/tson-edit][tsonによる編集][scale=0.9]

//footnote[about_jq][https://stedolan.github.io/jq/]
//footnote[about_tson][https://github.com/skanehira/tson]

=== ファイラ
端末で作業しているとディレクトリの移動やファイルのコピーや削除、中身を確認したりすることが多々あります。
これらもすべてコマンドで操作する必要があり、ちょっと面倒です。

そこで、筆者はff@<fn>{about_ff}というTUIツールを作りました。
ffは手軽にディレクトリの移動やファイル、ディレクトリの作成、削除やプレビューを行えることが特徴です。

@<img>{gorilla/about-ff}はffを使ってファイルをプレビューしている様子です。

//image[gorilla/about-ff][ファイラー][scale=0.9]
//footnote[about_ff][https://github.com/skanehira/ff]

== TUIツールを作る
TUIツールをいくつか紹介したところで、実際に簡単なTUIツールを作ってみましょう。
今回のTUIツールはファイルのビューアです。ビューアの仕様の次になります。

 * カレントディレクトリのファイル一覧画面
 * 選択したファイルの中身をプレビュー画面に表示

とてもシンプルなTUIですので、みなさんもぜひ一緒に手を動かしながら読み進めましょう。

=== ライブラリ
今回使用するライブラリはtview@<fn>{about_tview}というTUIライブラリです。筆者は普段このライブラリを使用してTUIツールを作っています。
1点注意ですが、このライブラリはWindowsでは正しく描画されないです。Windowsの方はWSLなどを使って実装してください。

//footnote[about_tview][https://github.com/rivo/tview]

=== 実装
実装は大まかに3ステップになります。

 1. ファイル一覧を取得
 2. ファイル一覧を表示する画面を作成
 3. ファイルのプレビュー画面を作成し中身を表示する

読者が理解しやすいように、すべてmainパッケージに実装します。なお、ファイル分けはします。
では、やっていきましょう。

==== 1. ファイル一覧を取得
まず、file.goファイルを作成して、@<code>{ioutil}パッケージの@<code>{ReadDir()}関数を使用して、
@<list>{get_files}のカレントディレクトリ配下のファイル情報のみを取得する関数を作成します。

//listnum[get_files][ファイル一覧を取得][go]{
package main

import (
	"io/ioutil"
	"os"
)

func Files(dir string) ([]os.FileInfo, error) {
	fileInfo, err := ioutil.ReadDir(dir)
	if err != nil {
		return nil, err
	}

	var files []os.FileInfo
	for _, f := range fileInfo {
		if !f.IsDir() {
			files = append(files, f)
		}
	}

	return files, nil
}
//}

続いて、file_test.goファイルを作成して、@<list>{test_get_files}のテストを書きます。

//listnum[test_get_files][ファイル一覧取得関数のテスト][go]{
package main

import (
	"io/ioutil"
	"os"
	"path/filepath"
	"testing"
)

func mkfile(name string) error {
	f, err := os.Create(name)
	if err != nil {
		return err
	}
	defer f.Close()
	return nil
}

func TestFiles(t *testing.T) {
	testdir, err := ioutil.TempDir("", "")
	if err != nil {
		t.Fatalf("cannot create testdir: %s", err)
	}
	defer os.RemoveAll(testdir)

	exceptedFiles := map[string]string{
		"a.go": "f",
		"tmp":  "d",
	}

	for f, typ := range exceptedFiles {
		tmp := filepath.Join(testdir, f)
		// if file
		if typ == "f" {
			err := mkfile(tmp)
			if err != nil {
				t.Fatalf("create error: %s", err)
			}
			// if dir
		} else if typ == "d" {
			err := os.Mkdir(tmp, 0666)
			if err != nil {
				t.Fatalf("create error: %s", err)
			}
		}
	}

	files, err := Files(testdir)
	if err != nil {
		t.Fatalf("cannot get files: %s", err)
	}

	fileName := files[0].Name()
	if fileName != "a.go" {
		t.Fatalf("want: a.go, got: %s", fileName)
	}
}

func TestFilesFail(t *testing.T) {
	if _, err := Files("xxx"); err == nil {
		t.Fatalf("failed test: err is nil")
	}
}
//}

テスト書き終えたら、実行してみましょう。PASSしたらOKです。

//cmd{
$ go test -v
=== RUN   TestFiles
=== RUN   TestFiles/success
=== RUN   TestFiles/failed
--- PASS: TestFiles (0.00s)
    --- PASS: TestFiles/success (0.00s)
    --- PASS: TestFiles/failed (0.00s)
PASS
ok      github.com/skanehira/shoten8-sample-tui 0.007s
//}

==== 2. ファイル一覧を表示する画面を作成
ファイル一覧を取得する関数ができたので、次にいよいよtviewを使って画面を作っていきます。
ファイルを選択して何かを操作するインタフェースはtview.Table@<fn>{about_tview_table}を使います。

//footnote[about_tview_table][https://github.com/rivo/tview/wiki/Table]

まず、file_panel.goファイルを作成して、@<list>{file_panel_struct}の構造体を用意します。

//listnum[file_panel_struct][FilePanel][go]{
package main

import (
	"os"

	"github.com/rivo/tview"
)

type FilePanel struct {
	files []os.FileInfo
	*tview.Table
}

//}

選択したファイルをプレビューできるようにするため、
@<code>{Files()}で取得したファイルをfilesフィールドに持たせます。

次に、@<list>{new_file_panel}のNew関数を用意します。

//listnum[new_file_panel][FilePanelのNew関数][go]{
func NewFilePanel() *FilePanel {
	p := &FilePanel{
		Table: tview.NewTable(),
	}

	p.SetBorder(true).
		SetTitle("files").
		SetTitleAlign(tview.AlignLeft)

	p.SetSelectable(true, false)

	return p
}
//}

@<code>{tview.Table}のメソッドについて解説します。

 * @<code>{SetBorder()}は画面に枠を描画するかどうかの設定、trueの場合は枠が描画される
 * @<code>{SetTitle()}は枠上のタイトルをセットする設定
 * @<code>{SetTitleAlign()}はタイトルの位置を設定
 * @<code>{SetSelectable()}は行と列を選択できるかどうかを設定、1つ目の引数は行、2つ目は列

関数の詳細はGoDocを参照していただくとして、基本的にこういった流れで画面を定義していきます。
次に、いくつか関数を追加していきます。追加する関数は@<list>{file_panel_methods}です。

//listnum[file_panel_methods][FilePanelの関数][go]{
func (f *FilePanel) SetFiles(files []os.FileInfo) {
	f.files = files
}

func (f *FilePanel) SelectedFile() os.FileInfo {
	row, _ := f.GetSelection()
	if row > len(f.files)-1 || row < 0 {
		return nil
	}
	return f.files[row]
}

func (f *FilePanel) Keybinding(g *GUI) {
	f.SetSelectionChangedFunc(func(row, col int) {
		// TODO preview file
	})
}

func (f *FilePanel) UpdateView() {
	table := f.Clear()

	for i, fi := range f.files {
		cell := tview.NewTableCell(fi.Name())
		table.SetCell(i, 0, cell)
	}
}
//}

各関数について説明していきます。

@<code>{SetFiles()}は@<code>{Files}で取得したファイル情報を@<code>{FilePanel.files}にセットします。
@<code>{NewFilePanel()}でfilesにセットしてもよいですが、役割が異なるので別関数として切り出します。
@<code>{SelectedFile()}は現在選択しているファイル情報を取得します。@<code>{f.GetSelection()}は現在テーブルの行と列を取得できるのでそれを利用しています。

@<code>{Keybinding()}は@<code>{FilePanel}のキーバインドを設定します。
@<code>{tview.Table}の@<code>{SetSelectionChangedFunc()}は選択する項目が変わるたびに呼び出されます。関数に行と列のindexを取得できます。
indexを使ってfilesからファイル情報を取得します。なお、プレビュー画面はまだ作成していないのでいったんToDoとします。

@<code>{UpdateView()}は画面描画をします。@<code>{tview.Table}では、セルという単位で描画していきますので、
@<code>{tview.Table}の@<code>{SetCell()}関数を使用します。

@<code>{SetCell()}の第1引数は行、第2引数は列、第3引数セル構造体を渡します。
たとえば、2列2行のテーブルを描画したい場合は@<list>{tview_table_setcell}のようになります。

//listnum[tview_table_setcell][2列2行描画][go]{
table.SetCell(0, 0, tview.NewTableCell("1行1列目"))
table.SetCell(0, 1, tview.NewTableCell("1行2列目"))
table.SetCell(1, 0, tview.NewTableCell("2行1列目"))
table.SetCell(1, 1, tview.NewTableCell("2行2列目"))
//}

ファイル一覧を表示して、選択する画面の実装は以上です。
次に@<code>{gui.go}ファイルを作成して、@<list>{about_gui}のファイル一覧画面とプレビュー画面を管理する役割を持つ構造体GUIとそれを生成するNew関数を用意します。

//listnum[about_gui][画面を管理するためのGUI構造体][go]{
package main

import (
	"os"

	"github.com/rivo/tview"
)

type GUI struct {
	App       *tview.Application
	Pages     *tview.Pages
	FilePanel *FilePanel
}

func NewGUI() *GUI {
	return &GUI{
		App:       tview.NewApplication(),
		Pages:     tview.NewPages(),
		FilePanel: NewFilePanel(),
	}
}

//}

@<code>{tview.Application}はtview全体を制御します。TUIツールを起動するときは@<code>{ApplicationのRun()}関数を実行します。

@<code>{Pages}は各画面を制御します。
今回は画面が2つあって、各画面へのフォーカスなどを@<code>{Pages}が行います。

さらに、@<code>{GUI}に@<list>{gui_methods}の関数を追加します。

//listnum[gui_methods][追加する関数][go]{
func (g *GUI) Run() error {
	cur, err := os.Getwd()
	if err != nil {
		return err
	}
	files, err := Files(cur)
	if err != nil {
		return err
	}

	g.FilePanel.SetFiles(files)
	g.FilePanel.UpdateView()

	g.SetKeybinding()

	grid := tview.NewGrid().SetColumns(0, 0).
		AddItem(g.FilePanel, 0, 0, 1, 1, 0, 0, true)

	g.Pages.AddAndSwitchToPage("main", grid, true)

	return g.App.SetRoot(g.Pages, true).Run()
}

func (g *GUI) SetKeybinding() {
	g.FilePanel.Keybinding(g)
}
//}

@<code>{SetKeybinding()}は@<code>{GUI}が管理している画面のキーバインドを起動時にまとめて設定する役割です。
画面を増やしていくたびに、ここにキーバインドの設定関数を追加していきます。

@<code>{Run()}は一番重要な関数で、TUIを実行する役割です。処理の概要を解説をします。

  1. @<code>{os.Getwd()}で現在のディレクトリを取得して@<code>{Files}に渡す
  2. @<code>{Files()}で取得したファイル一覧を@<code>{FilePanel}にセット
  3. @<code>{FilePanel.UpdateView()}でテーブルを描画
  4. @<code>{SetKeybinding()}で各画面のキーバインド設定
  5. @<code>{tview.NewGrid()}でグリッドを作成
  4. @<code>{SetColumns()}でグリッドのセルを縦2つ作る
  5. @<code>{AddItem()}で@<code>{FilePanel}をグリッドに追加して配置
  6. @<code>{Pages.AddAndSwitchToPage()}でグリッドを@<code>{Pages}の管理下におき、フォーカスする
  7. @<code>{App.SetRoot()}でPagesを@<code>{Application}の管理下におき、@<code>{Run()}でtviewを実行

この処理の中で大事になってくるのはグリッドです。グリッドは画面レイアウトを制御するのに使用します。
今回は画面の左半分にファイル一覧、右半分にプレビュー画面のレイアウトにするので、縦に2つセルを用意します。

@<code>{SetColumns()}は縦に作成するセルの数とセルのサイズを設定します。渡した引数の分だけ縦にセルを作ります。
渡した値がセルのサイズです。0の場合は余った領域のサイズになります。

たとえば、@<code>{SetColumns(5, 0)}の場合は、次図のように縦1つ目のセルのサイズは5で、右半分のサイズは残りの領域をすべて使います。
今回は0を2つ渡したので、均等サイズのセルが2つ作られます。

//cmd{
+-----+----------+
|     |          |
|     |          |
|     |          |
|     |          |
|     |          |
+-----+----------+
//}

@<code>{AddItem()}はまず@<code>{FilePanel}をグリッドに追加します。
そして第2引数は行、第3引数は列で、どのセルに置くかを設定します。@<code>{FilePanel}は1行1列目のセルに配置するので第2、3引数はともに0です。
たとえば1行2列目のセルに@<code>{FilePanel}を配置したい場合は第2は0、第3引数は1となります。

次に第3引数は行の、第4引数は列のセルの大きさを設定します。@<code>{FilePanel}は1セルのみを使うので、行と列を1に設定します。
たとえば、@<code>{FilePanel}を1行2列で配置したい場合は第3引数は1、第4引数は2を設定します。

グリッドに関する説明は文面だけでは理解しづらいので、
ソースコードを書き換えて動作を確認して見てください。そちらの方が理解しやすいです。

ここまで実装すると動かすことができます。@<img>{gorilla/tui_files_sample}は動かしたときの様子です。

//image[gorilla/tui_files_sample][ファイル一覧の画面][scale=0.9]

==== 3. ファイルのプレビュー画面を作成し中身を表示する
ファイル一覧の画面を出したところで、次はプレビュー画面を作っていきます。
まず@<code>{preview.go}を作成して、@<list>{about_preview}の構造体と関数を用意します。

//listnum[about_preview][Preview構造体と関数][go]{
package main

import (
	"io/ioutil"

	"github.com/rivo/tview"
)

type PreviewPanel struct {
	*tview.TextView
}

func NewPreviewPanel() *PreviewPanel {
	p := &PreviewPanel{
		TextView: tview.NewTextView(),
	}

	p.SetBorder(true).
		SetTitle("preview").
		SetTitleAlign(tview.AlignLeft)

	return p
}

func (p *PreviewPanel) UpdateView(name string) {
	var content string
	b, err := ioutil.ReadFile(name)
	if err != nil {
		content = err.Error()
	} else {
		content = string(b)
	}

	p.Clear().SetText(content)
}
//}

テキストを画面に出力するには@<code>{tview.TextView}を使います。
@<code>{NewPreviewPanel()}は@<code>{NewFilePanel()}とやっていることはほぼ同じで、タイトルとその位置を設定しています。

@<code>{UpdateView()}は実際ファイルの中身を読み込んで画面に出力します。

プレビュー画面を描画する処理は以上です。次に@<code>{gui.go}に@<list>{add_preview_panel}の追加処理を実装していきます。
+の部分が追記箇所です。

//listnum[add_preview_panel][PreviewPanelの追加と初期化][go]{
 type GUI struct {
 	App          *tview.Application
 	Pages        *tview.Pages
 	FilePanel    *FilePanel
+	PreviewPanel *PreviewPanel
 }

 func NewGUI() *GUI {
 	return &GUI{
 		App:          tview.NewApplication(),
 		Pages:        tview.NewPages(),
 		FilePanel:    NewFilePanel(),
+		PreviewPanel: NewPreviewPanel(),
 	}
 }

 func (g *GUI) Run() error {
 	cur, err := os.Getwd()
 	if err != nil {
 		return err
 	}
 	files, err := Files(cur)
 	if err != nil {
 		return err
 	}

 	g.FilePanel.SetFiles(files)
 	g.FilePanel.UpdateView()

+	file := g.FilePanel.SelectedFile()
+	if file != nil {
+		g.PreviewPanel.UpdateView(file.Name())
+	}

 	g.SetKeybinding()

 	grid := tview.NewGrid().SetColumns(0, 0).
 		AddItem(g.FilePanel, 0, 0, 1, 1, 0, 0, true).
+		AddItem(g.PreviewPanel, 0, 1, 1, 1, 0, 0, true)

 	g.Pages.AddAndSwitchToPage("main", grid, true)

 	return g.App.SetRoot(g.Pages, true).Run()
 }
//}

次に@<list>{add_files_preview}のファイル一覧画面で選択したファイルをプレビューする処理を追記します。

//listnum[add_files_preview][プレビュー処理を追加][go]{
 func (f *FilePanel) Keybinding(g *GUI) {
 	f.SetSelectionChangedFunc(func(row, col int) {
 		if file := f.SelectedFile(); file != nil {
+			g.PreviewPanel.UpdateView(file.Name())
 		}
 	})
 }
//}

以上で、ファイルをプレビューするTUIツールは完成です。
実装が問題なければ@<img>{gorilla/preview_tui_sample}のように起動してプレビューできます。

//image[gorilla/preview_tui_sample][プレビューの様子][scale=0.9]

=== 最後に
簡易のプレビューTUIツールを作りましたが、まだ改善余地はあります。たとえばプレビュー画面をスクロールできるようにするなどです。
そこは読者のみなさんへの課題とします。ぜひ取り組んでみてください。

本章で実装したサンプルコードは筆者のリポジトリ@<fn>{sample_ui_repository}に置いてありますので、全体像をつかんで置きたい方はそちらを参考してください。

本章を読んで、TUIツールを作ってみようかなって気持ちになったら筆者的の目的は達成です。
こういった小さなツールは作業効率を上げる道具になりますので、ぜひチャレンジしてみてください。

//footnote[sample_ui_repository][https://github.com/skanehira/shoten8-sample-tui]
