= Goによるコンテナランタイム自作入門

こんにちは、@_moricho_@<fn>{_moricho_}です。
みなさんコンテナ使ってますか。最近ではDockerやKubernetesなど、業務の中でもすっかりコンテナ技術が浸透してきています。
しかしどうやってそのコンテナ型仮想化が実現されているかという内部のことまでは、なかなか触れる機会がありません。
Dockerなどコンテナ技術の中核を成すのが「コンテナランタイム」という部分です。
本章ではGoを使って実際に簡易的なオリジナルのコンテナランタイムを作成しながら、コンテナ型仮想化の仕組みを見ていきます。本章がコンテナへの理解や探究の一助になれば幸いです。
（本章では開発環境としてLinuxを想定しています。）

//footnote[_moricho_][@<href>{https://twitter.com/_moricho_}]

== コンテナランタイムとは
コンテナランタイムはコンテナの実行管理をするものです。コンテナランタイムは大きく分けて「ハイレベルコンテナランタイム」と「ローレベルランタイム」に分割されます。
たとえばDockerなどのコンテナを扱う際は、基本的に「ハイレベルランタイム => ローレベルランタイム」の順で処理が流れていきます。

//image[morito/runtime][コンテナランタイム][scale=0.9]

=== ハイレベルランタイム
ハイレベルランタイムでは直接コンテナを操作するわけではなく、その次のローレベルランタイムへ命令を渡す役割を果たします。
またここではイメージの管理（@<tt>{pull/push/rm…}）も担います。

=== ローレベルランタイム
ハイレベルランタイムから受け取った指示でコンテナの起動や停止をキックするなど、コンテナの直接的な操作を担当する部分です。
今回は、コンテナ型仮想化のコアとなるこの部分を作っていきます。


== コンテナ型仮想化の概要

本節では、コンテナをコンテナたらしめている仕組みをざっくり説明します。
実際にコンテナランタイムを実装していく上で見通しをよくするためにも必要になる知識ですので、ざっと理解しておきましょう。

=== コンテナとは
コンテナを一言でいうとなんでしょうか。コンテナとは「ホストOSのリソースを隔離・制限したプロセス」といえます。
プロセスであるため、通常のアプリケーションと遜色ないレベルですばやく実行できます。また、ホストOSのリソースの一部を使用しているのがポイントです。

=== なぜ異なるOSベースのイメージが動くのか
普段何気なく使っているコンテナ技術ですが、そもそもなんでホストと異なるOSのイメージが動くのか不思議に思ったことがある方も多いのではないのでしょうか。
まず前提として、あるホストの上ですべてのOSイメージが動くわけではありません。ホストがあるLinuxディストリビューションであると仮定すると、実行可能なイメージはあくまでも次のようなものです。

 * 異なるLinuxディストリビューション
 * 同じLinuxディストリビューションの異なるバージョン

そのためmacOSやWindowsの上でUbuntuイメージは直接動きません。

たとえば私たちが普段から馴染みのあるDockerについて考えてみましょう。鋭い方は、なぜ私たちのmacOSの上でCentOSやUbuntuのDockerイメージが動くのか疑問に思ったのではないでしょうか。実は私たちがMacで使っているDocker、正確には@<tt>{Docker for Mac}は、@<tt>{LinuxKit}@<fn>{linuxkit}という軽量のLinuxVMとして動作しています。つまり、macOS上にハイパーバイザ型のミニマムなLinuxを立ち上げ、そのうえでコンテナが動いているのです。より正確には、macOSとLinuxKitの間に@<tt>{HyperKit}@<fn>{hyperkit}というmacOSの仮想化システムが入ります。（参考：@<tt>{LinuxKit with HyperKit（macOS）}@<fn>{disc}）

//footnote[linuxkit][@<href>{https://github.com/linuxkit/linuxkit}]
//footnote[hyperkit][@<href>{https://github.com/moby/hyperkit}]
//footnote[disc][@<href>{https://github.com/linuxkit/linuxkit/blob/master/docs/platform-hyperkit.md}]

=== 異なるOSイメージを動かすためのキー
コンテナはOS機能すべてを再現するものではなく、あくまでも特定のディストリビューション上のアプリケーションの動きを再現するものです。そしてそのキーとなるものが「Linuxカーネル」と「ファイルシステム」です。

==== 1. Linuxカーネル
LinuxディストリビューションはいずれもLinuxカーネルを使って動作します。またOS上で動くアプリケーションは、システムコールを使ってカーネルに対して要求を出したりします。肝は、システムコールにおけるアプリケーションとLinuxカーネル間のインタフェースである「ABI（Application Binary Intetface）」です。このインタフェースは互換性を考慮して作られており、Linuxカーネルのバージョンの多少の違いによってシステムコールが大きく変わることはありません。そのため、コンテナとホストのOSのバージョンが多少違っても問題ありません。しかし、このままではコンテナプロセスがホストOSのリソースを使いたい放題です。これでは困るため、次の２つを行います。

 * カーネルリソースの隔離
 * ハードウェアリソースの制限

==== 2. ファイルシステム
OSによってファイルシステムは異なります。コンテナでは、あるOSと同じ状態のファイルシステムをプロセスに対して見せることで、そのOSさながらの環境を実現しています。そのためにも、プロセスに対してファイルシステムのルートを勘違いさせます。また、あるコンテナがホストやほかのコンテナのファイルを見れてしまうと分離度が下がってしまうため、次のことを行います。

 * ファイルシステムのルート変更/ファイルシステムの隔離

=== 次節からの実装に向けて
 （ローレベル）コンテナランタイムを作るにあたり私たちが実装するべきものは、大きく次の３つです。

  * カーネルリソースの隔離（Namespace）
  * ファイルシステムの隔離（pivot_root）
  * ハードウェアリソースの制限（cgroups）

次節からこれらを詳しく見ていきます。その前に必要な準備をしましょう。ご自身のLinux環境で試してみてください。

//list[prepare][準備][]{
$ git clone https://github.com/moricho/shoten8-container && cd shoten8-container
$ mkdir -p /tmp/shoten/rootfs
$ tar -C /tmp/shoten/rootfs -xf busybox.tar
//}

@<code>{busybox.tar}を@<code>{/tmp/shoten/rootfs}へ展開する意味については、のちほど説明します。それではコンテナを作り始めましょう。

== カーネルリソースの隔離
Linuxには、プロセスごとにリソースを分離して提供する@<tt>{Namespaces}という機能があります。
分離できるリソースには次のようなものがあります。

 * PID：プロセスID
 * User：ユーザーID/グループID
 * Mount：ファイルシステムツリー
 * UTS：hostname、domainname
 * Network：ネットワークデバイスやIPアドレス
 * IPC：プロセス間通信のリソース

たとえばPID名前空間を分離するとしましょう。そうすると、それぞれのPID名前空間で独立にプロセスIDがふられます。つまり、同一ホスト上で同一のPIDを持ったプロセスが同居しているような状態が作れるのです。こうして名前空間を分離することで「コンテナAがコンテナBの重要なファイルシステムをアンマウントする」、「コンテナCがコンテナDのネットワークI/Fを削除する」といったこともできなくなります。

注意として、@<tt>{Namespaces}はあくまでもプロセス間のカーネルリソースを隔離しているのであって、ホストのハードウェアリソース（CPUやメモリなど）へのアクセスを制限しているわけではありません。ハードウェアリソースの制限は、後に紹介する@<tt>{cgroups}という機能によって実現されます。それでは実際に、各@<tt>{Namespace}を分離した新たな子プロセスを生成してみましょう。

//list[namespace1][Namespaceの分離：main.go][go]{
func main() {
  cmd := exec.Command("/bin/sh")
	cmd.SysProcAttr = &syscall.SysProcAttr{
		Cloneflags: syscall.CLONE_NEWUSER |
			syscall.CLONE_NEWNET |
			syscall.CLONE_NEWPID |
			syscall.CLONE_NEWIPC |
			syscall.CLONE_NEWUTS |
			syscall.CLONE_NEWNS,
		UidMappings: []syscall.SysProcIDMap{
			{
				ContainerID: 0,
				HostID:      os.Getuid(),
				Size:        1,
			},
		},
		GidMappings: []syscall.SysProcIDMap{
			{
				ContainerID: 0,
				HostID:      os.Getgid(),
				Size:        1,
			},
		},
	}

	cmd.Stdin = os.Stdin
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
  cmd.Env = []string{"PS1=-[shoten]- # "}

  if err := cmd.Run(); err != nil {
		fmt.Printf("Error running the /bin/sh command - %s\n", err)
		os.Exit(1)
	}
}
//}

まずは@<list>{namespace1}を実行してみてください。

//list[namespace2][実行結果][]{
$ go build -o main
$ ./main
-[shoten]- # whoami
root
-[shoten]- # id
uid=0(root) gid=0(root) groups=0(root)
//}

新しくプロセスが開始され、rootユーザーとして認識されているのがわかります。ではコードの方を見ていきましょう。

//list[namespace3][プロセスのclone][go]{
cmd.SysProcAttr = &syscall.SysProcAttr{
    Cloneflags: syscall.CLONE_NEWUSER |
        syscall.CLONE_NEWNET |
        syscall.CLONE_NEWPID |
        syscall.CLONE_NEWIPC |
        syscall.CLONE_NEWUTS |
        syscall.CLONE_NEWNS,
}
//}

まずプロセスの起動に際して@<code>{Cloneflags}というものを渡しています。これはLinuxカーネルのシステムコールである@<code>{clone（2）}@<fn>{clone}コマンドに渡せるflagと同じです。@<code>{clone（2）}とは、Linuxが子プロセスの作成をするときに呼ばれるシステムコールです。ここでは上であげた６つの名前空間すべてを新しく分離しています。

//footnote[clone][@<href>{https://linuxjm.osdn.jp/html/LDP_man-pages/man2/clone.2.html}]

次にその下を見ましょう。

//list[namespace4][プロセスのclone][go]{
cmd.SysProcAttr = &syscall.SysProcAttr{
  UidMappings: []syscall.SysProcIDMap{
    {
      ContainerID: 0,
      HostID:      os.Getuid(),
      Size:        1,
    },
  },
  GidMappings: []syscall.SysProcIDMap{
    {
      ContainerID: 0,
      HostID:      os.Getgid(),
      Size:        1,
    },
  },
}
//}

@<list>{namespace4}では、ホストのユーザー名前空間と新たに分離したユーザー名前空間におけるUID/GIDのマッピングを行っています。
なぜこうするのかというと、単にユーザー名前空間を分離しただけでは起動後のプロセス内でユーザー/グループが@<code>{nobody/nogroup}となってしまうからです。新しいユーザー名前空間で実行されるプロセスのUID/GIDを設定するためには、@<code>{/proc/[pid]/uid_map}と@<code>{/proc/[pid]/gid_map}に対して書き込みを行います。Goでは@<code>{syscall.SysProcAttr}に@<code>{UidMappings}フィールドと@<code>{GidMappings}フィールドを設定することでこれをやってくれます。@<list>{namespace4}はrootユーザーとして新たなプロセスを実行しています。

== ファイルシステムの隔離
前節までは、マウント名前空間（@<code>{CLONE_NEWNS}フラグで指定したもの）含む各名前空間を分離したプロセスを起動するところまでやりました。前節のスクリプトを実行して起動したプロセスに入り、プロセス内で何がマウントされているか見てましょう。

//list[mount1][実行結果][]{
$ go build -o main
$ ./main
-[shoten]- # cat /proc/mounts
/dev/xvda1 / ext4 rw,relatime,discard,data=ordered 0 0
udev /dev devtmpfs rw,nosuid,relatime,size=491524k,\
  nr_inodes=122881,mode=755 0 0
devpts /dev/pts devpts rw,nosuid,noexec,relatime,gid=5,\
  mode=620,ptmxmode=000 0 0
...
//}

マウント空間を分離したはずなのに、ホストでマウントされている多くのマウントの情報を見ることができてしまいます。@<code>{mount_namespaces}@<fn>{mount_namespaces}のmanページを見ると、それがなぜだかわかります。@<code>{CLONE_NEWNS}フラグ付きで@<code>{clone（2）}が呼ばれた場合、呼び出し元のマウントポイントのリストが新たなプロセスへコピーされる仕様になっているのです。これでは、コンテナからホストの情報が見えてしまっているためよくありません。そこで登場するのが@<b>{pivot_root（2）}@<fn>{pivot_root}です。@<code>{pivot_root}について説明するにあたって、まずはファイルシステムについておさらいしましょう。

//footnote[mount_namespaces][@<href>{http://man7.org/linux/man-pages/man7/mount_namespaces.7.html}]
//footnote[pivot_root][@<href>{https://linuxjm.osdn.jp/html/LDP_man-pages/man2/pivot_root.2.html}]

=== ファイルシステムとは
ファイルシステムとは、@<b>{ブロックデバイスのデータを構造的に扱う仕組み}のことです。ブロックデバイスはHDDやSSDなどを指します。ブロックデバイスのデータは人が直接扱うには複雑ですが、ファイルシステムがそれらを@<b>{ファイル}や@<b>{ディレクトリ}という形で扱うことを可能にしています。またブロックデバイスのデータを、システム上のディレクトリツリーに対応させることを@<b>{マウント}といいます。

=== pivot_root
ファイルシステムについて理解したところで、続いて@<code>{pivot_root}について説明していきます。@<code>{pivot_root}とは、プロセスのルートファイルシステムを変更するLinuxの機能です。@<code>{pivot_root}は引数として@<code>{new_root}と@<code>{put_old}を取ります。呼び出し元のプロセスのルートファイルシステムを@<code>{put_old}ディレクトリに移動させ、@<code>{new_root}を呼び出し元のプロセスの新しいルートファイルシステムにします。また@<code>{pivot_root}には、new_rootとput_oldに関して制約があります。

 * ディレクトリでなければならない
 * 現在のrootと同じファイルシステムにあってはならない
 * @<code>{put_old}は@<code>{new_root}の下になければならない
 * ほかのファイルシステムがput_oldにマウントされていてはならない

の４つです。
これらを考慮して@<code>{pivot_root}を実装しましょう。

//list[mount2][pivot_rootの実装][go]{
func pivotRoot(newroot string) error {
	putold := filepath.Join(newroot, "/oldrootfs")

	// pivot_rootの条件を満たすために、新たなrootで自分自身をバインドマウント
  if err := syscall.Mount(
		newroot,
		newroot,
		"",
		syscall.MS_BIND|syscall.MS_REC,
		"",
	); err != nil {
		return err
	}

	if err := os.MkdirAll(putold, 0700); err != nil {
		return err
	}

	if err := syscall.PivotRoot(newroot, putold); err != nil {
		return err
	}

	if err := os.Chdir("/"); err != nil {
		return err
	}

	putold = "/oldrootfs"
	if err := syscall.Unmount(putold, syscall.MNT_DETACH); err != nil {
		return err
	}

	if err := os.RemoveAll(putold); err != nil {
		return err
	}

	return nil
}
//}

流れは次のようになっています。

 1. @<code>{new_root}で@<code>{new_root}自身をバインドマウント
 2. @<code>{pivot_root}を実行
 3. 不要になった以前のルートファイルシステムをアンマウント、そしてディレクトリを削除

@<code>{pivot_root}の制約の１つに「@<code>{new_root}と@<code>{put_old}は現在のrootと同じファイルシステムにあってはならない」がありました。まず@<code>{new_root}を新たなマウントポイントとして@<code>{new_root}自身でバインドマウントすることにより、これを満たすようにしています。またマウントは通常、ディレクトリツリーをブロックデバイスの領域へ紐付けるために行われます。それに対し「バインドマウント」は、ディレクトリをディレクトリにマウントします。今回は@<code>{new_root}を@<code>{new_root}でマウントすることにより、@<code>{new_root}以下の階層の内容はそのままに、@<code>{new_root}を新たなマウントポイントとしたファイルシステムとして認識させています。

そしてこの後に@<code>{pivot_root}を実行することで、新たに@<code>{new_root}がファイルシステムのルートになり、その上位階層（ホストのディレクトリ）は見ることができなくなります。また@<code>{pivot_root}では元のファイルシステムが@<code>{put_old}をマウントポイントとしてマウントされるため、コンテナにホストの情報が残ったままになってしまいます。これを回避するために、続いて@<code>{put_old}をアンマウントしてから削除しています。

では@<list>{mount2}の@<code>{pivotRoot}関数はどのタイミングで実行すればよいでしょうか。
もちろん名前空間が分離された後ですが、分離度の観点から、プロセスが起動し@<code>{/bin/sh}が実行されるよりも前がいいです。
しかし@<list>{namespace1}で見たように、一度@<code>{cmd.Run}関数が呼ばれたら名前空間が分離され、そしてプロセスが実行されてしまいます。ここをうまく解決してくれるのが@<code>{reexec}パッケージです。

=== reexecパッケージ

@<code>{reexec}パッケージ@<fn>{reexec}は、OSS版Dockerの開発を進めるMobyプロジェクト@<fn>{moby}から提供されています。
さっそく、@<code>{reexec}を使って@<list>{namespace1}をアップデートしたコードを見てましょう。

//list[mount3][reexecを使ったコード１：main.go][go]{
func init() {
	reexec.Register("InitContainer", InitContainer)
	if reexec.Init() {
		os.Exit(0)
	}
}

func InitContainer() {
	newrootPath := os.Args[1]
	if err := pivotRoot(newrootPath); err != nil {
		fmt.Printf("Error running pivot_root - %s\n", err)
		os.Exit(1)
	}

	Run()
}

func Run() {
	cmd := exec.Command("/bin/sh")

	cmd.Stdin = os.Stdin
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr

	cmd.Env = []string{"PS1=-[shoten]- # "}

	if err := cmd.Run(); err != nil {
		fmt.Printf("Error running the /bin/sh command - %s\n", err)
		os.Exit(1)
	}
}

func main() {
  ...
}
//}

@<list>{mount3}について説明していきます。まず@<code>{init}関数での初期化処理が追加されているのがわかります。またその初期化処理の中で@<code>{reexec.Register("InitContainer", InitContainer)}が呼ばれています。ここでは、後述する@<code>{InitContainer}関数を@<b>{InitContainer}という名前でreexecに登録しました。こうして登録しておくことで、 @<code>{main}関数内で@<tt>{InitContainer}というコマンドとして実行できます。

続いて@<code>{InitContainer}関数です。この関数内の@<code>{newrootPath}は、@<tt>{InitContainer}コマンドの引数として渡ってくる予定のものです。まずこの@<code>{newrootPath}を、@<list>{mount2}で実装した@<code>{pivotRoot}関数に渡し、@<tt>{pivot_root}を行っています。そして@<code>{InitContainer}関数の最後には、@<code>{Run}関数で今までどおり@<code>{exec.Cmd}から@<code>{/bin/sh}を実行しています。

では最後に@<code>{main}関数を見ていきましょう。

//list[mount4][reexecを使ったコード２：main.go][go]{
func main() {
	var rootfsPath = "/tmp/shoten/rootfs"

	cmd := reexec.Command("InitContainer", rootfsPath)
	cmd.SysProcAttr = &syscall.SysProcAttr{
		Cloneflags: syscall.CLONE_NEWUSER |
			syscall.CLONE_NEWNET |
			syscall.CLONE_NEWPID |
			syscall.CLONE_NEWIPC |
			syscall.CLONE_NEWUTS |
			syscall.CLONE_NEWNS,
		UidMappings: []syscall.SysProcIDMap{
			{
				ContainerID: 0,
				HostID:      os.Getuid(),
				Size:        1,
			},
		},
		GidMappings: []syscall.SysProcIDMap{
			{
				ContainerID: 0,
				HostID:      os.Getgid(),
				Size:        1,
			},
		},
	}

	cmd.Stdin = os.Stdin
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr

	if err := cmd.Run(); err != nil {
		fmt.Printf("Error running the /bin/sh command - %s\n", err)
		os.Exit(1)
	}
}
//}

新たに@<code>{cmd := reexec.Command("InitContainer", rootfsPath)}として、@<list>{mount3}で登録した@<code>{InitContainer}コマンドを呼んでいるのがわかります。それ以外は@<list>{namespace1}とほとんど変わらず、@<code>{cmd}に対して@<code>{Cloneflags}や@<code>{Uid/GidMappings}の設定をしています。また@<list>{mount3}では@<code>{/tmp/shoten/rootfs}を新たなルートファイルシステムとして@<code>{pivot_root}しています。

では、@<list>{mount3}、@<list>{mount4}で作成した新たな@<tt>{main.go}を実行してみましょう。

//list[mount5][実行結果][]{
$ go build -o main
$ ./main
-[shoten]- # cat /proc/mounts
/dev/xvda1 / ext4 rw,relatime,discard,data=ordered 0 0
proc /proc proc rw,relatime 0 0
//}

@<list>{mount1}とは違い、限られたマウントポイントしか存在していないことがわかります。

//footnote[reexec][@<href>{https://github.com/moby/moby/tree/master/pkg/reexec}]
//footnote[moby][@<href>{https://mobyproject.org/}]

=== ベースイメージとなるファイルシステムの展開
「7.2.4 異なるOSイメージを動かすためのキー」で、こう説明しました。

//quote{
  「コンテナでは、あるOSと同じ状態のファイルシステムをプロセスに対して見せることで、そのOSさながらの環境を実現しています。」
//}

ではいま作っているコンテナのベースイメージはなんでしょうか。@<list>{prepare}で@<code>{busybox.tar}を@<code>{/tmp/shoten/rootfs}に展開したことを思い出してください。実は@<code>{busybox.tar}を解凍すると、@<code>{busybox}のルートファイルシステムが展開されます。また前項までを通して見てきたように、ホストの@<code>{/tmp/shoten/rootfs}ディレクトリが起動したプロセスのルートになります。つまり、起動したプロセスは@<code>{busybox}をベースイメージとしたコンテナに相当するのです。

== ハードウェアリソースの制限
前節までを通して、ユーザー・グループやファイルシステムなどのリソースが分離されたプロセスを作成できました。
しかし、そのプロセスはまだCPUやメモリなどのハードウェアリソースに対して制限なくアクセスできる状態です。
１つのコンテナがホストのCPUやメモリを食い尽くしてしまい、ほかのホストプロセスやコンテナに影響があると困りますよね。

そこで登場するのが@<code>{cgroups}@<fn>{cgroups}です。
@<tt>{cgroups}は@<tt>{control groups}の略です。プロセスのグループ化や、グループ内のプロセスに対するリソース制御などの仕組みを提供しています。
@<tt>{namespaces}では名前空間の隔離を通してカーネルリソースを制御していたのに対し、@<tt>{cgroups}ではCPUやメモリ、ディスクI/Oといった物理的なリソースを制御します。

//footnote[cgroups][@<href>{http://man7.org/linux/man-pages/man7/cgroups.7.html}]

@<tt>{cgroups}では@<tt>{/sys/fs/cgroup}配下に仮想的なファイルシステムを提供しています。
このファイルシステムに対して読み込み・書き込みの操作を行うことで、グループに対してリソースの利用に制限をかけることが可能になっています。試しに@<tt>{Ubuntu16.04}上で@<code>{ls /sys/fs/cgroup}を実行してみます。

//list[cgroup1][ls /sys/fs/cgroup の実行結果][]{
$ ls /sys/fs/cgroup
blkio  cpu  cpu,cpuacct  cpuacct  cpuset  devices  freezer  hugetlb  memory
net_cls  net_cls,net_prio  net_prio  perf_event  pids  rdma  systemd  unified
//}

@<tt>{cpu}や@<tt>{memory}など、わかりやすくリソースごとにディレクトリが別れています。この@<tt>{cpu}や@<tt>{memory}ディレクトリの配下に新たなディレクトリを作成することで、そのディレクトリ名単位でのcgroupが作られます。それでは@<code>{/sys/fs/cgroup/cpu}配下に@<code>{shoten}というCPUレベルでの@<tt>{cgroup}を作成してみましょう。そして@<code>{/sys/fs/cgroup/cpu/shoten}をのぞいてみます。

//list[cgroup2][/sys/fs/cgroup/cpu/shoten][]{
$ mkdir /sys/fs/cgroup/cpu/shoten
$ ls /sys/fs/cgroup/cpu/shoten
cgroup.clone_children  cpu.cfs_period_us  cpu.shares  cpuacct.stat
cpuacct.usage_all     cpuacct.usage_percpu_sys   cpuacct.usage_sys
notify_on_releasecgroup.procs    cpu.cfs_quota_us   cpu.stat
cpuacct.usage  cpuacct.usage_percpu  cpuacct.usage_percpu_user
cpuacct.usage_user  tasks
//}

@<list>{cgroup2}を見ると、CPUに関するさまざまな設定ファイルが作成されているのがわかります。たとえばCPU使用率を制限する場合は@<code>{cpu.cfs_quota_us}に書き込みを行います。@<code>{cpu.cfs_quota_us}では、100000マイクロ秒間あたりにいくらCPUを利用できるかという値を設定します。プロセスのCPU使用率を５％に制限したい場合は、@<tt>{5000}という値を@<code>{cpu.cfs_quota_us}に書き込むだけです。
また、@<code>{/sys/fs/cgroup/cpu/shoten}内でもう１つ重要なファイルが@<code>{tasks}です。このファイル内では、どのプロセスをこのcontrol groupに入れるかを管理しています。複数プロセス入れることもできますが、本項ではコンテナとなるプロセス自身のみを設定しましょう。ではcgroupのコードを見ていきます。

//list[cgroup3][cgroupの実装][go]{
  func cgroup() error {
  	if err := os.MkdirAll("/sys/fs/cgroup/cpu/shoten", 0700); err != nil {
  		return fmt.Errorf("failed to create directory: %w", err)
  	}

  	if err := ioutil.WriteFile(
  		"/sys/fs/cgroup/cpu/shoten/tasks",
  		[]byte(fmt.Sprintf("%d\n", os.Getpid())),
  		0644,
  	); err != nil {
  		return fmt.Errorf("failed to register tasks: %w", err)
  	}

  	if err := ioutil.WriteFile(
  		"/sys/fs/cgroup/cpu/shoten/cpu.cfs_quota_us",
  		[]byte("5000\n"),
  		0644,
  	); err != nil {
  		return fmt.Errorf("failed to limit cpu.cfs_quota_us: %w", err)
  	}

  	return nil
  }
//}

とてもシンプルで、主に次の3つを行っています。

 * @<code>{/sys/fs/cgroup/cpu/shoten}ディレクトリを作成
 * @<code>{tasks}ファイルに自身のPIDを書き込み
 * @<code>{cpu.cfs_quota_us}にCPU使用率が５％になるように5000という数字の書き込み

ではまた@<code>{reexec}パッケージを使って、@<list>{cgroup3}を適用していきましょう。変更箇所は@<code>{InitContainer}関数だけです。

//list[cgroup4][InitContainer()の更新][go]{
func InitContainer() {
  if err := cgroup(); err != nil {
		fmt.Printf("Error running cgroup - %s\n", err)
		os.Exit(1)
	}

	...

  if err := pivotRoot(newrootPath); err != nil {
		fmt.Printf("Error running pivot_root - %s\n", err)
		os.Exit(1)
	}

	Run()
}
//}

前節までの@<code>{pivotRoot}関数に加え、@<code>{cgroup}関数を追加しました。このとき@<code>{cgroup}関数と@<code>{pivotRoot}関数の位置関係に注意してください。@<code>{cgroup}関数ではホストの@<code>{/sys/fs/cgroup/cpu/shoten}配下にあるファイルへ書き込みを行いたいため、@<code>{pivotRoot}関数でルートファイルシステムを変更する前に実行しています。

ではCPU使用率が５％に制限されているか確認してみましょう。まず@<code>{/bin/sh}が実行されたプロセス内で「@<code>{while :; do true ; done}」を実行し、CPUに負荷をかけます。@<code>{cgroups}の機能で制限していない場合、通常ならCPU使用率が100％近くになります。

//list[cgroup5][負荷をかける][]{
$ go build -o main
$ ./main
-[shoten]- # while :; do true ; done

//}

では別のセッションに入り@<code>{top}コマンドでCPU使用率を確認してみます。

//list[cgroup6][別セッションでCPU使用率を確認][]{
PID   USER     PR  NI    VIRT    RES    SHR S %CPU %MEM     TIME+ COMMAND
20493 root     20   0    8100   2100   1988 R  5.0  0.2   0:02.31 sh
20270 root     20   0  404360  18924  12840 S  0.3  1.9   0:02.06 ssm-session-wor
1     root     20   0  225140   9212   7088 S  0.0  0.9   0:07.44 systemd
2     root     20   0       0      0      0 S  0.0  0.0   0:00.00 kthreadd
...
//}

一番上の@<code>{sh}を実行しているプロセスが該当するものです。見事に５％前後でキープされているのがわかります。

== 自作コンテナに機能を追加する
ここまでを通してコンテナの核となる機能を実装できました。お疲れさまです。本節では、ここからさらに機能拡張をする場合に何をすればいいか簡単に紹介します。

==== vethとbridge
「7.3 カーネルリソースの隔離」の@<list>{namespace1}では、@<code>{syscall.SysProcAttr}構造体の@<code>{Cloneflags}フィールドに@<tt>{CLONE_NEWNET}フラグをセットし、ネットワーク名前空間を分離しました。

ホストとコンテナの異なるネットワーク名前空間同士で通信ができるようにするためには、@<code>{veth}@<fn>{veth}というL2の仮想ネットワークインタフェースを設定する必要があります。@<code>{veth}はペアで作成され、一方がホスト側、もう一方がコンテナ側のネットワーク名前空間に割り当てられて通信が可能になります。つまり、@<code>{veth}はホストとコンテナ間をL2でトンネリングしてくれるのです。
またホスト側に仮想的なブリッジを作成してホスト側の@<code>{veth}と接続することで、コンテナはホスト外とも通信が可能になります。Linuxで仮想ブリッジを作る機能が@<code>{bridge}@<fn>{vbridge}です。@<code>{veth}や@<code>{bridge}は@<tt>{netlink}パッケージ@<fn>{netlink}を使うことで手軽に実装できます。

//footnote[netlink][@<href>{https://github.com/vishvananda/netlink}]
//footnote[veth][@<href>{http://man7.org/linux/man-pages/man4/veth.4.html}]
//footnote[vbridge][@<href>{http://man7.org/linux/man-pages/man8/bridge.8.html}]

==== OverlayFS
「7.4 ファイルシステムの隔離」では@<code>{pivot_root}や@<code>{mount}を使い、コンテナに対して新たなファイルシステムを割り当て、コンテナからホストのファイルシステムを見えないようにしました。しかしコンテナのファイルシステムはあくまでもホストのルート下に存在しており、コンテナ内でファイルやディレクトリの作成・上書き・削除などを行うと、それがホストにも影響します。

そこで役に立つのが@<code>{OverlayFS}という機能です。@<code>{OverlayFS}@<fn>{overlayfs}は@<code>{UnionFileSystem}の1つで、同じマウントポイントに複数のブロックデバイスをマウントし、それぞれに含まれるディレクトリ構造の和としてファイルシステムを扱います。ディレクトリツリーが重ね合わされており、上位層のディレクトリ/ファイルに変更を加ても、下位層のそれには影響が出ません。これを使うことで、コンテナで実行しているプロセス内でファイルシステムを操作してもホストへの影響を抑えられます。

//footnote[overlayfs][@<href>{https://www.kernel.org/doc/html/latest/filesystems/overlayfs.html?highlight=overlayfs}]

==== seccomp
@<code>{seccomp（2）}@<fn>{seccomp}とは、プロセスのシステムコールの発行を制限する機能です。これを用いて、コンテナが危険なシステムコールを発行することを防ぐことができます。たとえばDockerでは、@<code>{moby/profiles/seccomp/default.json}@<fn>{seccompdocker}で発行可能/不可能なシステムコールを管理しています。

//footnote[seccomp][@<href>{http://man7.org/linux/man-pages/man2/seccomp.2.html}]
//footnote[seccompdocker][@<href>{https://github.com/moby/moby/blob/master/profiles/seccomp/default.json}]


== おわりに
コンテナ型仮想化の仕組みや、Goからカーネルの機能を使って簡単なコンテナを実装する方法を紹介しました。前節で紹介したようなさらなる機能拡張や、Docker以外のコンテナランタイムなど、興味がある方はコンテナ技術についてさらに調査してみてください。
