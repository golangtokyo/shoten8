= Goによるコンテナランタイム自作入門

== はじめに
こんにちは、@_moricho_@<fn>{_moricho_}です。
みなさんコンテナ使ってますか。最近ではDockerやKubernetesなど、業務の中でもすっかりコンテナ技術が浸透してきています。またAWSのLambdaなども実は裏でコンテナ技術が使われています。
しかしどうやってそのコンテナ型仮想化が実現されているかという内部のことまでは、なかなか触れる機会がありません。
DockerしかりLambdaしかり、コンテナ技術の中核を成すのが「コンテナランタイム」という部分です。
ここではGoを使って実際に簡易的なオリジナルのコンテナランタイムを作成しながら、コンテナ型仮想化の仕組みを見ていきます。本章がコンテナへの理解や探究の一助になれば幸いです。

//footnote[_moricho_][@<href>{https://twitter.com/_moricho_}]

== コンテナランタイムとは
コンテナランタイムはコンテナの実行管理をするものです。コンテナランタイムは大きく分けて「ハイレベルコンテナランタイム」と「ローレベルランタイム」に分割されます。
たとえばDockerなどのコンテナを扱う際は、基本的に「ハイレベルランタイム => ローレベルランタイム」の順で処理が流れていきます。

1. ハイレベルランタイム
ハイレベルランタイムでは直接コンテナを操作するわけではなく、その次のローレベルランタイムへ命令を渡す役割を果たします。
またここではイメージの管理（pull/push/rm…）も担います。

2.  ローレベルランタイム
ハイレベルランタイムから受け取った指示でコンテナの起動や停止をキックするなど、コンテナの直接的な操作を担当する部分です。
今回は、コンテナのコアとなるこの部分を作っていきます。


== コンテナ型仮想化の概要

ここでは、コンテナをコンテナたらしめている仕組みをざっくり説明します。
実際にコンテナランタイムを実装していく上で見通しをよくするためにも必要になる知識ですので、ざっと理解しておきましょう。

=== コンテナとは
コンテナを一言でいうとなんでしょうか。コンテナとは「ホストOSのリソースを隔離・制限したプロセス」といえます。
プロセスであるため、通常のアプリケーションと遜色ないレベルですばやく実行できます。また、ホストOSのリソースの一部を使用しているのがポイントです。

=== なぜ異なるOSベースのイメージが動くのか
普段何気なく使っているコンテナ技術ですが、そもそもなんでホストと異なるOSのイメージが動くのか不思議に思ったことがある方も多いのではないのでしょうか。
まず前提として、あるホストの上ですべてのOSイメージが動くわけではありません。ホストがあるLinuxディストリビューションであると仮定すると、対象となるイメージはあくまでも
・異なるLinuxディストリビューション
・同じLinuxディストリビューションの異なるバージョン
であり、たとえばMacやWindowsの上でUbuntuイメージは（直接は）動きません。

鋭い方は、ではなぜ普段私たちのMacの上でCentOSやUbuntuのDockerイメージが動くのか疑問に思ったのではないでしょうか。
実は私たちがMacで使っているDocker、正確にはDockeer for Macは、「LinuxKit」という軽量のLinuxVMとして動作しています。つまり、Mac上にハイパーバイザ型のミニマムなLinuxを立ち上げ、そのうえでコンテナが動いているのです。（より正確には、MacとLinuxKitの間に「HyperKit」というmacOSの仮想化システムが入ります。）
どうりでDocker for Macの立ち上げが遅いわけですよね。

=== 異なるOSイメージを動かすためのキー
コンテナはOS機能すべてを再現するものではなく、あくまでも特定のディストリビューション上のアプリケーションの動きを再現するものです。そしてそのキーとなるものが「Linuxカーネル」と「ファイルシステム」です。

1. Linuxカーネル
LinuxディストリビューションはいずれもLinuxカーネルを使って動作します。またOS上で動くアプリケーションは、システムコールを使ってカーネルに対して要求を出したりします。
肝は、システムコールにおけるアプリケーションとLinuxカーネル間のインタフェースである「ABI（Application Binary Intetface）」です。このインタフェースは互換性を考慮して作られており、Linuxカーネルのバージョンの多少の違いによってシステムコールが大きく変わることはありません。そのため、コンテナとホストのOSのバージョンが多少違っても問題ありません。
しかし、このままではコンテナプロセスがホストOSのリソースを使いたい放題です。これでは困るため、
・カーネルリソースの隔離
・ハードウェアリソースの制限
を行います。

2. ファイルシステム
OSによってファイルシステムは異なります。コンテナでは、あるOSと同じ状態のファイルシステムをプロセスに対して見せることで、そのOSさながらの環境を実現しています。
そのためにも、プロセスに対してファイルシステムのルートを勘違いさせます。また、あるコンテナがホストやほかのコンテナのファイルを見れてしまうと分離度が下がってしまうため、
・ファイルシステムのルート変更/ファイルシステムの隔離
を行います。


== 実装

（ローレベル）コンテナランタイムを作るにあたり私たちが実装するべきものは、
・カーネルリソースの隔離
・ファイルシステムの隔離
・ハードウェアリソースの制限
の大きく3つです。
それぞれ詳しく見ていきましょう。


=== 1.Namespaces
カーネルリソースの隔離

=== 2.pivot_root
ファイルシステムの隔離

=== 3.cgroup
ハードウェアリソースの制限


== GoでCLI作成

== 自作コンテナをさらに拡張する

== 終わりに