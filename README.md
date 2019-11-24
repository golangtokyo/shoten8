# shoten8
[![CircleCI](https://circleci.com/gh/golangtokyo/shoten8.svg?style=svg)][circleci]

[circleci]:https://circleci.com/gh/golangtokyo/shoten8

このリポジトリはRe:VIEWを使って技術書典7用の文書を書くリポジトリです。Re:VIEWバージョン4.0を使っています。


 * [B5紙面サンプル（PDF）](https://github.com/TechBooster/ReVIEW-Template/tree/master/pdf-sample/TechBooster-Template-B5.pdf)
 * [A5紙面サンプル（PDF）](https://github.com/TechBooster/ReVIEW-Template/tree/master/pdf-sample/TechBooster-Template-A5.pdf)
 * [B5紙面電子書籍サンプル（PDF）](https://github.com/TechBooster/ReVIEW-Template/tree/master/pdf-sample/TechBooster-Template-ebook.pdf)

## 使い方は？
提供されている一般的な使い方は[テンプレートの使い方](./TEMPLATE_README.md)を参照してください。


### このリポジトリだけの使い方
`make`コマンドがあれば以下のコマンドを利用できます。


```bash
lint            Execute textlint
fixlint         Fix textlint error
codepen         Execute codepen
build           Build PDF in Docker
help            Show usages
```

また、エディタにtextlintに対応したプラグインがあればコマンドを実行しなくてもtextlintが表示できます。

- VSCode: [テキスト校正くん - Visual Studio Marketplace](https://marketplace.visualstudio.com/items?itemName=ICS.japanese-proofreading)

PRを作成し、校正チェックが問題なければPDFが生成されます。
PDFのありかはPRにCIからコメントが付きます。


## Lintのルールがおかしい・この用語はLintで怒られないようにしてほしい
校正の補助としてtextlintを使っています。ルールの調整は以下のファイルを編集してください。

- [.textlintrc](https://github.com/golangtokyo/shoten8/blob/master/.textlintrc)

単語や正規表現を用いた許可ルールの追加は以下のファイルを編集してください。


- [allow.yml](https://github.com/golangtokyo/shoten8/blob/master/allow.yml)

どちらかのファイルを編集したさいは、その変更だけ先に`master`ブランチに入れておいてもらえると、他の人も同じルールで執筆ができます。



