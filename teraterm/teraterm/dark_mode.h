/*
 * Copyright (C) 2026- TeraTerm Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * dark_mode.h
 *
 * ウィンドウ枠(タイトルバー)・スクロールバー・メニューバーを暗色化する
 * ダークモード対応。Windows 10 1809 (build 17763) 以降で動作し、
 * 未公開の uxtheme.dll API に依存する。古い Windows では何もしない
 * (安全に無効化される)。
 *
 * タイトルバーは DWMWA_USE_IMMERSIVE_DARK_MODE、スクロールバーは
 * SetWindowTheme(L"DarkMode_Explorer") で暗色化する。メニューバー
 * (File/Edit... の横帯)は OS が自動でダーク化しないため、未公開の
 * WM_UAHDRAWMENU 系メッセージを受けて自前でオーナードロー描画する。
 */

#pragma once

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * アプリ全体のダークモードを初期化する。プロセス内で一度だけ呼べばよい
 * (複数回呼んでも安全)。未対応の Windows では内部で何もしない。
 */
void DarkMode_Initialize(void);

/*
 * 指定ウィンドウにダークモードを適用 / 解除する。
 *   enable=TRUE  タイトルバー・スクロールバーを暗色化し、以降このウィンドウの
 *               メニューバーをダーク描画する (DarkMode_HandleWindowMessage 経由)。
 *   enable=FALSE 標準(明色)に戻す。
 * 適用後はタイトルバー/メニューバーの再描画を促す。
 */
void DarkMode_ApplyToWindow(HWND hWnd, BOOL enable);

/*
 * 現在ダークモードを適用中か (最後に DarkMode_ApplyToWindow(,TRUE) されたか)。
 */
BOOL DarkMode_IsActive(void);

/*
 * メニューバーのオーナードロー描画など、ダークモード固有のウィンドウ
 * メッセージを処理する。ウィンドウプロシージャの未処理メッセージを
 * そのまま委譲すること。処理した場合 TRUE を返し、*result に戻り値を格納
 * する。FALSE のときは呼び出し側が通常処理(DefWindowProc 等)を続ける。
 */
BOOL DarkMode_HandleWindowMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result);

#ifdef __cplusplus
}
#endif
