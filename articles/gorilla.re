= TUIツールを作ろう

初めまして、ゴリラ@<fn>{about_gorilla}です。
本章はGoを使ったTUI（Text-based User Interface）ツールの作り方について解説していきます。

//footnote[about_gorilla][https://twitter.com/gorilla0513]

== TUIとは
TUIは簡潔に説明しますと端末上で動作するGUIのようなインタフェースのことです。
GUIと同様、マウスとキーボードを使用できます。

TUIツールとはそのようなインタフェースを持つツールのことを指します。
よくCLI（Command Line Interface）と混合されますが、CLIは名前のとおりコマンドを使って操作するインタフェースです。
たとえば@<code>{ls}などはCLIの一種です。

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

しかし、TUIにもデメリットがあります。TUIはキーボードやマウス、画面などを使用するため自動化に向いていません。
対してCLIは自動化にとても向いています。

このように、TUIとCLIはそれぞれ活かせる場面がありますので、その使い分けもまた大事と筆者は考えています。

//image[gorilla/about_tui_tool][筆者が作ったdockerのTUIツール][scale=0.9]

== TUIツール
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
lazygit@<fn>{about_lazygit}はGitのTUIツールです。GitコマンドをTUIツールでラップしてより使いやすくなっています。

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
	t.Run("success", func(t *testing.T) {
		testdir, err := ioutil.TempDir("", "")
		if err != nil {
			t.Fatalf("cannot create testdir: %s", err)
		}
		defer os.RemoveAll(testdir)

		exceptedFiles := map[string]string{
			"a.go": "f",
			"b.md": "f",
			"tmp":  "d",
		}

		for f, typ := range exceptedFiles {
			tmpf := filepath.Join(testdir, f)
			// if file
			if typ == "f" {
				err := mkfile(tmpf)
				if err != nil {
					t.Fatalf("create error: %s", err)
				}
				// if dir
			} else if typ == "d" {
				err := os.Mkdir(tmpf, 0666)
				if err != nil {
					t.Fatalf("create error: %s", err)
				}
			}
		}

		files, err := Files(testdir)
		if err != nil {
			t.Fatalf("cannot get files: %s", err)
		}

		for _, f := range files {
			if _, ok := exceptedFiles[f.Name()]; !ok {
				msg := "want: a.go or b.md, got: %s"
				t.Fatalf(msg, f.Name())
			}
		}
	})

	t.Run("failed", func(t *testing.T) {
		if _, err := Files("xxx"); err == nil {
			t.Fatalf("failed test: err is nil")
		}
	})
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

まず、file_panel.goファイルを作成して、@<code>{file_panel_struct}の構造体を用意します。

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
 * @<code>{SetSelectable()}は行と列を選択できるかどうかにする設定、1つ目の引数は行、次は列

関数の詳細はGoDocを参照していただくとして、基本的にこういった流れで画面を定義していく流れです。
次に、いくつか関数を追加していきます。追加する関数は@<list>{file_panel_methods}です。

//listnum[file_panel_methods][FilePanelの関数][go]{
func (f *FilePanel) SetFiles(files []os.FileInfo) {
	f.files = files
}

func (f *FilePanel) Keybinding(g *GUI) {
	f.SetSelectionChangedFunc(func(row, col int) {
		if row > len(f.files)-1 || row < 0 {
			return
		}
		// TODO preview file
	})
}

func (f *FilePanel) UpdateView() {
	table := f.Clear()

	for i, fi := range f.files {
		table.SetCell(i, 0, tview.NewTableCell(fi.Name()))
	}
}
//}

関数について説明していきます。

@<code>{SetFiles()}は@<code>{Files}で取得したファイル情報を@<code>{FilePanel.files}にセットします。
@<code>{NewFilePanel()}でfilesにセットしてもよいですが、役割が異なるので別関数として切り出します。

@<code>{Keybinding()}は@<code>{FilePanel}のキーバインドを設定します。
@<code>{tview.Table}の@<code>{SetSelectionChangedFunc()}を使用することで、現在選択している項目のindexを取得できます。
そして、そのindexを使ってfilesからファイル情報を取得します。なお、プレビュー画面はまだ作成していないのでいったんToDoとします。

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

==== 3. ファイルのプレビュー画面を作成し中身を表示する

