= Go本体にコントリビュートする方法

本稿はGo本体およびGo周辺ツールへのコントリビュートをする方法のまとめです。
主にGit拡張の@<tt>{codechange}や、コードレビューツールであるGerritの使い方について解説します。
Go本体のソースコードの解説はしません。むしろ筆者が知りたい。

筆者は本体および周辺ツールに、いくつか変更をコミットした経験があります。
コミット方法はドキュメント化されている@<fn>{contribute}ものの、慣習的な部分について暗黙知が多い分野に思われます。
本稿が実際にコントリビュートする際の手助けになれば幸いです。

//footnote[contribute][@<href>{https://golang.org/doc/contribute.html}]

== リポジトリ

GoはGoogle SourceのGitリポジトリで管理されています@<fn>{repo}。
Go本体ではなく、@<tt>{golang.org/x}配下のツールもGoogle Sourceリポジトリで管理されています。
コントリビュート方法はいずれも同じです。

//footnote[repo][@<href>{https://go-review.googlesource.com/admin/repos}]

代表的なリポジトリのURLは次のとおりです。

//table[typical_repos][代表的なリポジトリ]{
パッケージ名	リポジトリURL
------------
（Go本体）	@<href>{https://go.googlesource.com/go}
@<tt>{golang.org/x/image}	@<href>{https://go.googlesource.com/image}
@<tt>{golang.org/x/mobile}	@<href>{https://go.googlesource.com/mobile}
@<tt>{golang.org/x/tools}	@<href>{https://go.googlesource.com/tools}
//}

コントリビュートはこれらのGoogle Sourceリポジトリに対して行います。
GitHub上にあるリポジトリはミラーで、これらに対してコミットはできません。
コントリビュートする際には、GitHub上のリポジトリをcloneしないようにしましょう。

なおGtiHub上にPull Requestを作ると、Gerrit上に自動でアップロードされるようになっています。
そのためPull Requestを使ったコントリビュートも可能です。
この記事ではそれについては解説しません。

なおIssueは、本体と@<tt>{golang.org/x}配下含めてすべて、GitHubの@<tt>{golang/go}@<fn>{issues}で管理されています。

//footnote[issues][@<href>{https://github.com/golang/go/issues}]

== Git codereview

Gitプラグインである@<tt>{git-codereview}を導入しましょう。
詳しいインストール手順については公式ドキュメント@<fn>{contribute}を参照してください。
本稿では、概要をかいつまんで説明します。

Goおよび周辺ツールはGitHub Pull Request上でコードレビューを行いません。
代わりにGerritと呼ばれるツールを使います。
@<tt>{git-codereview}プラグインはGerritを使うのに便利なツールです。

たとえば@<tt>{codereview}の@<tt>{mail}を実行する場合は@<tt>{git codereview mail}というコマンドで実行できます。
GitのAlias@<fn>{alias}を設定すれば、@<tt>{git mail}でも実行可能です。

//footnote[alias][@<href>{https://golang.org/doc/contribute.html#git-config}]

=== GerritのChange List

@<tt>{git-codereview}ツールの説明の前に、GerritのCL（Change List）の概念について説明します。
Gerrit上でのレビューはCLという単位で行われます。
このCLはGitHubのPull Requestとはだいぶ異なる概念です。

CLは名前のとおりリストであり、複数のパッチ（変更差分）が含まれます。
CLに対してパッチは追加はできますが、削除はできません。
CLにパッチを追加する場合は、基本的に@<tt>{git commit --amend}と同等の操作を行い、
最後のGitコミットを上書き更新します。
手元には1つのGitコミットしか残りませんが、Gerrit上には過去のパッチが残り続けます。
最後にCLをサブミットしてマージする場合は、最後のパッチが採用されます。

Gerritは、GitHubのPull Requestにたとえると常時Squash & Rebaseしてマージするような動作をします。
最終成果物はGitの1コミットになるわけです。
GerritはGitの1コミットを洗練させて完成させるための仕組みです。
GitHubのように複数のコミットを積み重ねて1つの成果物を作るのとは異なります。

@<tt>{git-codereview}はGerritに対しての操作を簡単に行うためのツールです@<fn>{cl}。

//footnote[cl][余談ですが、プロジェクトによってはGerritを操作するのに別のツールを使ったりします。たとえばChromiumプロジェクトは@<tt>{depot_tools}にある@<tt>{git-cl}というのを使います。]

=== @<tt>{help}

@<tt>{git-codereview}の詳しい説明を表示します。

=== @<tt>{change}

現在の変更のコメントを編集します。
ほとんどの場合において@<tt>{git commit}または@<tt>{git commit --amend}と同じです。
また、必要ならばブランチも作成されます。

実際のところ、筆者はこのコマンドをあまり使わずに@<tt>{git commit --amend}してしまっています。

=== @<tt>{mail}

変更をGerritに送信し、CLを作成または更新します。
またレビュアーにメールが送信されます。
実行後はCLのURLが表示されます。

注意すべきなのは、GitコミットメッセージがそのままCL上のコミットメッセージとして反映されることです。
Gerrit上でもコミットメッセージの更新は可能なのですが、@<tt>{mail}を実行するたびに上書き更新されてしまいます。
そのためGitのコミットメッセージを常に一次情報としておくとよいでしょう。

複数コミットがある場合はそれぞれ別のCLを作成または更新します。
この場合Relation chainというものがCL上に作られます。
パッチどうしに依存関係があるものを取り扱う場合に便利です。
具体例としては筆者のパッチ@<fn>{chain_patch}があります。
これは2つのCLが連なっていて、上のCLが下のCLに依存しています。
レビュアーはそのCLの差分のみをみてレビューするわけです。

//image[hajimehoshi/relation_chain][Relation chain]

複数コミットを送信する場合は、次のようにブランチを明示的に指定しなければなりません。

//cmd{
git codereview mail HEAD
//}

//footnote[chain_patch][@<href>{https://go-review.googlesource.com/c/mobile/+/210477}]

=== @<tt>{gofmt}

@<tt>{gofmt}をかけます。
@<tt>{change}や@<tt>{mail}前にフォーマットされているかどうかのチェックが自動でかかります。
フォーマットが必要な場合、@<tt>{git gofmt}を実行せよというメッセージが表示されるはずです。

@<tt>{git-codereview}は他にもさまざまなコマンドがありますが、実際のところ頻繁に使うのはこれくらいです。
そのほかは同等のGitコマンドまたはGerrit上の操作でなんとかなってしまいます。

== Gerrit

先述のとおり、GoはGoogle製のコードレビューツールであるGerritを使用します。
Gerritは最終成果物の差分を1つのGitコミットとしてマージします。
GitHubのPull Requestにたとえるなら常時Squash & Rebaseする運用です。
Gerritはそれに特化しているため、コメントが見やすいというメリットがあります。

GerritのURLはたとえば次のようなものです。

//cmd{
https://go-review.googlesource.com/c/mobile/+/214899
//}

最後の数字、214899はCLのIDです。
なお省略用のURLとして@<tt>{golang.org/cl/NNNN}というのがあります。
これは上のURLにリダイレクトするだけで、まったく同じものを指します。
CL内でほかのCLを参照する際には、短いバージョンのURLが好まれます。

//cmd{
https://golang.org/cl/214899
//}

//image[hajimehoshi/gerrit][Gerrit]

この図を参考に、UIの各部分について説明します。

=== タイトル

図でいうところの、「@<tt>{cmd/gomobile:}」で始まる一行だけの部分です。
コミットメッセージの一行目がそのまま採用されます。

慣習として、変更に関連するディレクトリ名から始まり、CLの概要が続きます。
概要は常に小文字で始まります。

=== コミットメッセージ

真ん中の白いテキスト欄です。
マージする際にそのままコミットメッセージとして採用されます。
先述のとおり、@<tt>{git codereview mail}コマンドを使った際、ローカルマシンにあるGitコミットメッセージがそのまま反映されます。
よって、Gerrit UI上でコミットメッセージを直接編集するのはお勧めしません。

内容としては、次のようなことを含むのが一般的です。

  * このCLが導入される前の問題は何なのか。
  * このCLはその問題をどう問題を解決するのか。
  * このCLは何をするのか。
  * 関連するIssue。

文脈をまったく知らない人がこのCLを見たときに、情報を把握するために必要です。
多少冗長でも明確な説明が好まれます。

関連するIssueは@<tt>{Updates}または@<tt>{Fixes}と書き、その後にIssue番号をIssue番号は@<tt>{golang/go#32963}のように書きます。
@<tt>{Updates}は関連するIssueを更新するがまだ閉じない場合に使います。
@<tt>{Fixes}は関連するIssueを解決して閉じる場合に使います。
@<tt>{Fixes}の場合は、CLがマージされたあとに関連Issueが自動的にCloseします。
@<tt>{golang/go}と書くのは、Goの場合は周辺ツール含めてすべてのIssueがGitHubの@<tt>{golang/go}で管理されているためです。

コミットメッセージの@<tt>{Change-Id}以降は自動的に追記されるため、一切気にする必要はありません。

=== Reply

ReplyボタンはCLにコメントを残したり、レビュアーを追加できます。

//image[hajimehoshi/gerrit_reply][Replyボタンを押したとき表示されるダイアログ。]

CLをレビューしてもらうためにはレビュアーをアサインする必要があります。
Issue上で議論があった上でのCLならば、そこに関係する人をアサインするのがよいでしょう。
そうでない場合、過去のCLを参考に、一番詳しそうな人に頼むのがよいでしょう。
CLは誰でもApproveできるわけではなく、権限を持った人のみであることに注意してください。

=== Code-Review

レビュアーのCLについての評価です。
サブミットするためには最低1つの+2が必要です。

//table[gerrit_score][GoにおけるGerritレビュアースコアの意味]{
スコア	意味
------------
-2	サブミットしてはならない。議論しなおし。
-1	このままサブミットは望ましくない。修正次第では続行可能。
0	なし
+1	Approved。ほかの人の+2を待て。
+2	Approved。サブミットしてよし。
//}

今回の例でいうとHana氏が+2評価をしてくれたおかげでサブミットできています。

なおこの表はGoにおける数字の意味であって、他プロジェクトでは異なる意味付けをしている可能性があります。

=== TryBot-Result

TryBot（テストを実行するbot）の実行結果です。
すべてのテストが通れば+1、何か失敗すれば-1がつきます。

TryBotの結果に関係なくCLのサブミットはできてしまいますが、極力通してからのほうがよいでしょう。

TryBotは権限を持っている人しか起動できません。
権限がない場合、権限を持っている人に依頼する必要があります。

=== コメント

//image[hajimehoshi/gerrit_comment][Gerritにおけるコメントの例。]

レビュアーがCLの修正を求める場合、該当箇所にコメントをつけることがあります。
適切にパッチを修正したうえで、コメントの返信を送信します。

なお、GitHub Pull Requestとちがい、コメントを即座に送信する機能はありません。
すべてのコマンドは即座に送信されず、Replyボタンを押したときに初めて全部送信されます。
よってパッチ修正とコメント返信を同時並行にやりつつ、あとでまとめて返信できます。

ACKボタンは「Ack.」コメントを残します。コメントの内容は了解したが、パッチに反映させるものがないことを意味します。
DONEボタンは「Done.」コメントを残します。はコメントの内容を了解し、パッチも修正したことを意味します。
このUIのおかげで、単に修正したという場合は「Done.」と一言書いて閉じるだけでよい、という文化になっています。

== 用語

IssuesやGitHub Issuesでは、よく次のフレーズが使われます。

//table[phrases][よく使われるフレーズ]{
フレーズ	意味
------------
PTAL	Please take a look / Please take another look
WIP	Work in progress（作業中）
SGTM	Seems good to me
LGTM	Looks good to me
ping	「応答願います」（24時間程度経っても返答がない場合などに使う）
ooo	Out of office（不在のためしばらくの間応答不可）
//}

PTALは、CL作成または更新が完了したので見てほしいときに使います。
CLを作ったときに最初のコメントとして書いておくとよいでしょう。

そのほか用語などについて、公式Wiki@<fn>{wiki}などが参考になると思われます。

//footnote[wiki][@<href>{https://github.com/golang/go/wiki/CodeReview}]

== コミュニティ

コントリビュートに関して何かわからないことがある場合、質問できるコミュニティはいくつかあります。

  * @<tt>{go-nuts}メーリングリスト@<fn>{ml}
  * Gophers Slack@<fn>{slack}の@<tt>{#contributing}チャンネル

//footnote[ml][@<href>{https://groups.google.com/forum/#!forum/golang-nuts}]
//footnote[slack][@<href>{https://invite.slack.golangbridge.org/}]

いずれも言語は英語です。
