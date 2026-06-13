# Tera Term (fukugan-ai fork)

これは [TeraTermProject/teraterm](https://github.com/TeraTermProject/teraterm) のフォークです。
Claude Code を快適に利用するための独自修正を加えています。

フォーク日時: **2026-06-13 19:43:22 (JST)** / 2026-06-13 10:43:22 (UTC)

## このフォークの主な変更点

- **マウストラッキング中の選択・右クリックペースト対応**
  Claude Code がマウストラッキング（DECSET 1000/1002/1006）を有効化している間でも、
  修飾キー無しで通常のドラッグ選択ハイライトと右クリックペーストが機能するようにしました。
  新設定 `SelectByMouseTracking`（既定 ON）で切り替えできます。

  **なぜこうしたか:** マウストラッキングが有効なとき、TeraTerm は左ボタン・移動・右ボタンといった
  マウスイベントを「アプリに報告すべきもの」として横取りし、画面側のローカル処理（選択・ペースト）を
  スキップします。これは本来 `vim` などがマウス操作を受け取るための正しい挙動ですが、Claude Code は
  起動中ずっとマウストラッキングを有効にするため、**ドラッグしても選択が反転表示されず、右クリックでも
  ペーストできない**状態になってしまいます。`Ctrl` 併用や `Alt+V` でも回避はできますが、毎回それを
  押すのが**単純に面倒**だったため、修飾キー無しでそのまま選択・ペーストできるようにしました。

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

  **もうひとつの理由（トゥルーカラー）:** `xterm-256color` を名乗ると、アプリは 256色だけでなく
  24bit トゥルーカラー（`38;2;r;g;b`）も送ってくるようになります。これにより、Claude Code の
  "Sublimating…" のようなスピナー演出の**グラデーション（ぴろぴろ動くやつ）が色飛びせず滑らかに
  綺麗に表示**されます。`xterm`（8色）や 256色どまりだと中間色が間引かれて縞模様になり、見栄えが
  悪くなってしまいます。

  本来は `teraterm.ini` を手で編集したり、ログイン後に `TERM` を設定すれば回避できますが、
  ユーザーに環境設定を強いずに済むよう **コード側で救済**する方針としました。既定値を
  `xterm-256color` にしつつ、過去設定で `xterm` が残っているケースだけを上書きで救うことで、
  新規設定・既存設定のどちらでも `TERM=xterm-256color` となり、色が正しく表示されます。
- **TTXSamples のビルド整理**
  公式配布に含まれないデモ群をビルド対象から除外しました。

upstream への追従やマージは行っていません。本家リポジトリには一切変更を加えていません。

### Powered by Claude Code

このフォークの調査・実装・ドキュメントは [**Claude Code**](https://claude.com/claude-code)（Anthropic）を使い、
**Claude Opus 4.8** で行いました。Claude Code は、ターミナル上で動作する Anthropic 公式の AI コーディング
エージェントです。コードベースの調査、バグ修正、リファクタリング、Git 操作までを自然言語で任せられます。
本フォークの「マウストラッキング中の選択対応」や「256色化」も、TeraTerm の C/C++ コードを Claude Code に
調査させて原因を特定し、修正まで仕上げたものです。

- 公式サイト: https://claude.com/claude-code
- ターミナル / VS Code / JetBrains などで利用できます。

ライセンスは本家と同じ BSD 系ライセンス（`LICENSE.md` 参照）に従います。各ソースの著作権表記・ライセンス条文はそのまま保持しています。

---

# Tera Term

## 🚨🚨 重要：下のインストーラーにフォークの変更は入っていません 🚨🚨

> [!CAUTION]
> **以下の CI バッジおよび「Build Installer」が生成するインストーラーは、本家
> [TeraTermProject/teraterm](https://github.com/TeraTermProject/teraterm) のものです。**
>
> ## ⚠️ このフォークの変更（マウストラッキング対応・256色化など）は一切反映されていません ⚠️
>
> これらの修正を使うには、**このリポジトリをソースからビルド**してください。

| CI | Workflow | Status |
|----|----------|--------|
| GitHub Actions | Build Installer | [![Build Status](https://github.com/TeraTermProject/teraterm/actions/workflows/msbuild.yml/badge.svg)](https://github.com/TeraTermProject/teraterm/actions/workflows/msbuild.yml) |
| GitHub Actions | Bulid with cmake (experimental) |[![Bulid with cmake (experimental)](https://github.com/TeraTermProject/teraterm/actions/workflows/build_cmake.yml/badge.svg)](https://github.com/TeraTermProject/teraterm/actions/workflows/build_cmake.yml) |

Tera Term is a free software terminal emulator

[URLs](https://github.com/TeraTermProject/teraterm/wiki/Urls)

## About Signed Binaries

We thank [SignPath.io](https://signpath.io) for providing a free code signing service, and the [SignPath Foundation](https://signpath.org/) for providing a certificate for free code signing.

