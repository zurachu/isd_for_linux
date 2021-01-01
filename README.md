# isd_for_linux

Linux, BSD, macOS で P/ECE と通信するコマンドラインツール
http://int.main.jp/prog/lin_piece.html （リンク切れ）で公開されていた 「Linux用isd」の改修

# setup

- [libusb-1.0](http://libusb.info)
- pkg-config

## autoconf の場合

```
$ autoreconf -i
$ ./configure && make
```

## CMake の場合

```
$ mkdir -p build && cd build
$ cmake ../
$ make
```

# usage

```
$ isd [オプション]
```

- `-c` - デバイスがあるかどうかのチェック
- `-d filename` - P/ECE にあるファイルを削除
- `-F` - P/ECE のファイルシステムをフォーマット（全削除）
- `-f` - 空き容量
- `-l` - P/ECE にあるファイルを列挙
- `-r filename` - P/ECE にあるファイルをダウンロード
- `-s filename` - P/ECE にファイルをアップロード
- `-v` - バージョン情報
