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

//image[gorilla/about_tui_tool][筆者が作ったdockerのTUIツール]

== TUIツール
TUIについて説明したところで、どんなTUIツールがあるのかを紹介します。

=== docui
TUIとはの節で軽く紹介しましたが、改めて紹介します。
docui@<fn>{about_docui}はdockerの操作をシンプルに、そして初心者でも使いやすいTUIツールです。

主な機能は次です。

 * イメージの検索、取得、削除、インポート、セーブ、ロード
 * コンテナの作成、起動、停止、削除、アタッチ、エクスポート
 * ネットワーク、ボリュームの削除

docuiでは各リソース（イメージやコンテナ）をパネルごとに表示しています。
各パネルでのキーを使って操作します。たとえばイメージのパネルで@<code>{f}で入力画面が表示され、キーワードを入力してEnterを押すと@<img>{gorilla/docui-search}のようにイメージの検索結果一覧を見れます。
また、@<code>{c}でコンテナの作成画面が表示され、必要な項目を入力してコンテナを作成できます。

//image[gorilla/docui-create-container][コンテナ作成]
//image[gorilla/docui-search][イメージの検索]

//footnote[about_docui][https://github.com/skanehira/docui]

=== lazygit
lazygit@<fn>{about_lazygit}はGitのTUIツールです。gitコマンドをTUIツールでラップしてより使いやすくなっています。

lazygitはファイルの差分確認やステージ追加、コミットの差分確認などを行うときに便利です。
@<img>{gorilla/lazygit-diff}はコミットの差分を表示する画面です。
@<img>{gorilla/lazygit-staging}はファイルの行単位の差分をステージに追加している画面です。

//image[gorilla/lazygit-diff][lazygit]
//image[gorilla/lazygit-staging][lazygit]
//footnote[about_lazygit][https://github.com/jesseduffield/lazygit]
