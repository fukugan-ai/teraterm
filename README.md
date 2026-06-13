# Tera Term (fukugan-ai fork)

これは [TeraTermProject/teraterm](https://github.com/TeraTermProject/teraterm) のフォークです。
Claude Code を快適に利用するための独自修正を加えています。

## このフォークの主な変更点

- **マウストラッキング中の選択・右クリックペースト対応**
  Claude Code がマウストラッキング（DECSET 1000/1002/1006）を有効化している間でも、
  修飾キー無しで通常のドラッグ選択ハイライトと右クリックペーストが機能するようにしました。
  新設定 `SelectByMouseTracking`（既定 ON）で切り替えできます。

  **なぜこうしたか:** マウストラッキングが有効なとき、TeraTerm は左ボタン・移動・右ボタンといった
  マウスイベントを「アプリに報告すべきもの」として横取りし、画面側のローカル処理（選択・ペースト）を
  スキップします。これは本来 `vim` などがマウス操作を受け取るための正しい挙動ですが、Claude Code は
  起動中ずっとマウストラッキングを有効にするため、**ドラッグしても選択が反転表示されず、右クリックでも
  ペーストできない**状態になってしまいます。従来は `Ctrl` 併用で回避できましたが、それを知らないと
  「マウスが効かない」と見えてしまいます。

  そこで設定 `SelectByMouseTracking`（既定 ON）を新設し、有効時は左ドラッグ選択と右クリックペーストを
  **アプリへ報告せずローカル処理**するよう、`vtwin.cpp` の `ButtonDown` / `OnLButtonDblClk` /
  `OnLButtonUp` / `OnMouseMove` / `OnRButtonUp` の5箇所でマウス報告を抑制しました。
  ホイールスクロールと中ボタンは従来どおりアプリへ報告するため、ページャ等のスクロール操作は維持されます。
  `off` にすれば従来挙動（`Ctrl` 併用）に戻せます。
- **既定端末タイプの 256色化**
  既定の端末タイプを `xterm-256color` に変更しました。さらに、既存の `teraterm.ini` に
  古い `TermType=xterm` が保存されている場合も、読込時に `xterm-256color` へ強制上書きします。

  **なぜこうしたか:** TeraTerm は接続先に対し、この端末タイプを `TERM` 環境変数として通知します。
  `TERM=xterm` のままだと、サーバー側のアプリは端末を **8色端末** とみなし、256色 / トゥルーカラーの
  ANSI エスケープシーケンス（`SGR 38;5;n` / `38;2;r;g;b`）を送ってきません。その結果、Claude Code の
  配色が崩れ、たとえば**オレンジが赤に化ける**といった現象が起きます。

  本来は `teraterm.ini` を手で編集したり、ログイン後に `TERM` を設定すれば回避できますが、
  ユーザーに環境設定を強いずに済むよう **コード側で救済**する方針としました。既定値を
  `xterm-256color` にしつつ、過去設定で `xterm` が残っているケースだけを上書きで救うことで、
  新規設定・既存設定のどちらでも `TERM=xterm-256color` となり、色が正しく表示されます。
- **TTXSamples のビルド整理**
  公式配布に含まれないデモ群をビルド対象から除外しました。

upstream への追従やマージは行っていません。本家リポジトリには一切変更を加えていません。

---

# Tera Term

| CI | Workflow | Status |
|----|----------|--------|
| GitHub Actions | Build Installer | [![Build Status](https://github.com/TeraTermProject/teraterm/actions/workflows/msbuild.yml/badge.svg)](https://github.com/TeraTermProject/teraterm/actions/workflows/msbuild.yml) |
| GitHub Actions | Bulid with cmake (experimental) |[![Bulid with cmake (experimental)](https://github.com/TeraTermProject/teraterm/actions/workflows/build_cmake.yml/badge.svg)](https://github.com/TeraTermProject/teraterm/actions/workflows/build_cmake.yml) |

Tera Term is a free software terminal emulator

[URLs](https://github.com/TeraTermProject/teraterm/wiki/Urls)

## About Signed Binaries

We thank [SignPath.io](https://signpath.io) for providing a free code signing service, and the [SignPath Foundation](https://signpath.org/) for providing a certificate for free code signing.

