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
 * dark_mode.cpp
 *
 * ウィンドウ枠(タイトルバー)・スクロールバー・メニューバーの暗色化。
 *
 * 仕組み:
 *  - タイトルバー : DWM の DWMWA_USE_IMMERSIVE_DARK_MODE を設定。
 *  - スクロールバー: SetWindowTheme(hWnd, L"DarkMode_Explorer") で
 *                    非クライアントスクロールバーを暗色テーマに切替。
 *  - メニューバー  : OS は上部メニュー帯(File/Edit...)を自動でダーク化
 *                    しないため、未公開の WM_UAHDRAWMENU / WM_UAHDRAWMENUITEM
 *                    メッセージを受けて自前でオーナードロー描画する。
 *                    メニュー下端の明色 1px ラインも WM_NCPAINT/WM_NCACTIVATE
 *                    の後で塗り潰す。
 *
 * いずれも Windows 10 1809 (build 17763) 以降でのみ動作する未公開 API に
 * 依存するため、全て動的ロードし、未対応環境では何もしない(安全に無効化)。
 *
 * 未公開 uxtheme API の調査は ysc3839/win32-darkmode 等の公開知見に基づく。
 */

#include <windows.h>
#include <uxtheme.h>

#include "dark_mode.h"

/* ---- 暗色パレット (near-black, Claude のターミナル配色に合わせる) ---- */
static const COLORREF DM_CLR_BG       = RGB(32, 32, 32);    // メニュー背景
static const COLORREF DM_CLR_HOT      = RGB(60, 60, 60);    // ホット/選択中の背景
static const COLORREF DM_CLR_TEXT     = RGB(225, 225, 225); // 通常文字
static const COLORREF DM_CLR_TEXT_DIS = RGB(120, 120, 120); // 無効文字

/* ---- DWM 属性 (SDK が古い場合に備えて自前定義) ---- */
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
/* build 18362(1903) より前は 19 が使われていた */
#define DM_DWMWA_USE_IMMERSIVE_DARK_MODE_OLD 19

/* 古い SDK では未定義の場合がある */
#ifndef LOAD_LIBRARY_SEARCH_SYSTEM32
#define LOAD_LIBRARY_SEARCH_SYSTEM32 0x00000800
#endif

/* ---- メニューテーマのパーツ/状態 (vssym32.h 相当を自前定義) ---- */
#ifndef MENU_BARBACKGROUND
#define MENU_BARBACKGROUND 7
#endif
#ifndef MENU_BARITEM
#define MENU_BARITEM 8
#endif
#ifndef MBI_NORMAL
#define MBI_NORMAL         1
#define MBI_HOT            2
#define MBI_PUSHED         3
#define MBI_DISABLED       4
#define MBI_DISABLEDHOT    5
#define MBI_DISABLEDPUSHED 6
#endif

/* ---- 未公開メニュー描画メッセージ ---- */
#define DM_WM_UAHDRAWMENU     0x0091  // lParam: UAHMENU*
#define DM_WM_UAHDRAWMENUITEM 0x0092  // lParam: UAHDRAWMENUITEM*

typedef union tagDM_UAHMENUITEMMETRICS {
	struct { DWORD cx; DWORD cy; } rgsizeBar[2];
	struct { DWORD cx; DWORD cy; } rgsizePopup[4];
} DM_UAHMENUITEMMETRICS;

typedef struct tagDM_UAHMENUPOPUPMETRICS {
	DWORD rgcx[4];
	DWORD fUpdateMaxWidths : 2;
} DM_UAHMENUPOPUPMETRICS;

typedef struct tagDM_UAHMENU {
	HMENU hmenu;
	HDC   hdc;
	DWORD dwFlags;
} DM_UAHMENU;

typedef struct tagDM_UAHMENUITEM {
	int                    iPosition;
	DM_UAHMENUITEMMETRICS  umim;
	DM_UAHMENUPOPUPMETRICS umpm;
} DM_UAHMENUITEM;

typedef struct tagDM_UAHDRAWMENUITEM {
	DRAWITEMSTRUCT dis;
	DM_UAHMENU     um;
	DM_UAHMENUITEM umi;
} DM_UAHDRAWMENUITEM;

/* ---- 未公開 uxtheme API の型 (ordinal でロード) ---- */
enum DM_PreferredAppMode { DM_APPMODE_DEFAULT, DM_APPMODE_ALLOWDARK, DM_APPMODE_FORCEDARK, DM_APPMODE_FORCELIGHT, DM_APPMODE_MAX };

typedef bool (WINAPI *fnAllowDarkModeForWindow)(HWND hWnd, bool allow);          // ordinal 133
typedef bool (WINAPI *fnAllowDarkModeForApp)(bool allow);                        // ordinal 135 (build < 18362)
typedef DM_PreferredAppMode (WINAPI *fnSetPreferredAppMode)(DM_PreferredAppMode); // ordinal 135 (build >= 18362)
typedef void (WINAPI *fnRefreshImmersiveColorPolicyState)(void);                // ordinal 104
typedef void (WINAPI *fnFlushMenuThemes)(void);                                 // ordinal 136

/* ---- ntdll: 正確なビルド番号取得 ---- */
typedef void (WINAPI *fnRtlGetNtVersionNumbers)(LPDWORD major, LPDWORD minor, LPDWORD build);

/* ---- DWM ---- */
typedef HRESULT (WINAPI *fnDwmSetWindowAttribute)(HWND, DWORD, LPCVOID, DWORD);

/* ---- uxtheme の公開 API (uxtheme.lib をリンクしないよう動的ロード) ---- */
typedef HRESULT (WINAPI *fnSetWindowTheme)(HWND, LPCWSTR, LPCWSTR);
typedef HTHEME  (WINAPI *fnOpenThemeData)(HWND, LPCWSTR);
typedef HRESULT (WINAPI *fnCloseThemeData)(HTHEME);
typedef HRESULT (WINAPI *fnDrawThemeTextEx)(HTHEME, HDC, int, int, LPCWSTR, int, DWORD, LPRECT, const DTTOPTS *);

/* ---- モジュール状態 ---- */
static BOOL g_initialized        = FALSE; // DarkMode_Initialize 済み
static BOOL g_supported          = FALSE; // この Windows でダークモードを使えるか
static BOOL g_active             = FALSE; // 現在適用中か
static DWORD g_buildNumber       = 0;

static fnAllowDarkModeForWindow        p_AllowDarkModeForWindow        = NULL;
static fnAllowDarkModeForApp           p_AllowDarkModeForApp           = NULL;
static fnSetPreferredAppMode           p_SetPreferredAppMode           = NULL;
static fnRefreshImmersiveColorPolicyState p_RefreshImmersiveColorPolicyState = NULL;
static fnFlushMenuThemes               p_FlushMenuThemes               = NULL;
static fnDwmSetWindowAttribute         p_DwmSetWindowAttribute         = NULL;
static fnSetWindowTheme                p_SetWindowTheme                = NULL;
static fnOpenThemeData                 p_OpenThemeData                 = NULL;
static fnCloseThemeData                p_CloseThemeData                = NULL;
static fnDrawThemeTextEx               p_DrawThemeTextEx               = NULL;

static HBRUSH g_brBg    = NULL;
static HBRUSH g_brHot   = NULL;
static HTHEME g_menuTheme = NULL;

/* ビルド番号がダークモード対応 (1809 / 17763 以降) か */
static BOOL DarkModeBuildSupported(DWORD build)
{
	return build >= 17763;
}

void DarkMode_Initialize(void)
{
	if (g_initialized) {
		return;
	}
	g_initialized = TRUE;

	/* 正確なビルド番号を取得 (GetVersionEx は manifest 依存で当てにならない) */
	HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
	if (hNtdll != NULL) {
		fnRtlGetNtVersionNumbers p = (fnRtlGetNtVersionNumbers)GetProcAddress(hNtdll, "RtlGetNtVersionNumbers");
		if (p != NULL) {
			DWORD major = 0, minor = 0, build = 0;
			p(&major, &minor, &build);
			g_buildNumber = build & 0x0FFFFFFF;
		}
	}
	if (!DarkModeBuildSupported(g_buildNumber)) {
		return; /* 古い Windows: ダークモード無効のまま */
	}

	/* uxtheme.dll の未公開 ordinal をロード */
	HMODULE hUx = LoadLibraryExW(L"uxtheme.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
	if (hUx == NULL) {
		return;
	}
	p_RefreshImmersiveColorPolicyState =
		(fnRefreshImmersiveColorPolicyState)GetProcAddress(hUx, MAKEINTRESOURCEA(104));
	p_AllowDarkModeForWindow =
		(fnAllowDarkModeForWindow)GetProcAddress(hUx, MAKEINTRESOURCEA(133));
	p_FlushMenuThemes =
		(fnFlushMenuThemes)GetProcAddress(hUx, MAKEINTRESOURCEA(136));
	/* ordinal 135 はビルドにより意味が異なる */
	if (g_buildNumber >= 18362) {
		p_SetPreferredAppMode = (fnSetPreferredAppMode)GetProcAddress(hUx, MAKEINTRESOURCEA(135));
	}
	else {
		p_AllowDarkModeForApp = (fnAllowDarkModeForApp)GetProcAddress(hUx, MAKEINTRESOURCEA(135));
	}
	/* 公開 API も名前で動的ロード (uxtheme.lib をリンクしないため) */
	p_SetWindowTheme  = (fnSetWindowTheme)GetProcAddress(hUx, "SetWindowTheme");
	p_OpenThemeData   = (fnOpenThemeData)GetProcAddress(hUx, "OpenThemeData");
	p_CloseThemeData  = (fnCloseThemeData)GetProcAddress(hUx, "CloseThemeData");
	p_DrawThemeTextEx = (fnDrawThemeTextEx)GetProcAddress(hUx, "DrawThemeTextEx");

	/* dwmapi.dll の DwmSetWindowAttribute */
	HMODULE hDwm = LoadLibraryExW(L"dwmapi.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
	if (hDwm != NULL) {
		p_DwmSetWindowAttribute = (fnDwmSetWindowAttribute)GetProcAddress(hDwm, "DwmSetWindowAttribute");
	}

	/* 必須 API が揃っているか */
	if (p_AllowDarkModeForWindow == NULL ||
		(p_SetPreferredAppMode == NULL && p_AllowDarkModeForApp == NULL)) {
		return;
	}

	/* アプリ全体にダークモードを許可 */
	if (p_SetPreferredAppMode != NULL) {
		p_SetPreferredAppMode(DM_APPMODE_ALLOWDARK);
	}
	else if (p_AllowDarkModeForApp != NULL) {
		p_AllowDarkModeForApp(true);
	}
	if (p_RefreshImmersiveColorPolicyState != NULL) {
		p_RefreshImmersiveColorPolicyState();
	}

	g_brBg  = CreateSolidBrush(DM_CLR_BG);
	g_brHot = CreateSolidBrush(DM_CLR_HOT);

	g_supported = TRUE;
}

void DarkMode_ApplyToWindow(HWND hWnd, BOOL enable)
{
	if (!g_initialized) {
		DarkMode_Initialize();
	}
	if (!g_supported || hWnd == NULL) {
		g_active = FALSE;
		return;
	}

	g_active = enable ? TRUE : FALSE;

	/* このウィンドウにダークモードを許可 */
	if (p_AllowDarkModeForWindow != NULL) {
		p_AllowDarkModeForWindow(hWnd, enable ? true : false);
	}

	/* タイトルバー: DWMWA_USE_IMMERSIVE_DARK_MODE */
	if (p_DwmSetWindowAttribute != NULL) {
		BOOL dark = enable ? TRUE : FALSE;
		if (p_DwmSetWindowAttribute(hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark)) != S_OK) {
			/* 1903 より前は属性番号が 19 */
			p_DwmSetWindowAttribute(hWnd, DM_DWMWA_USE_IMMERSIVE_DARK_MODE_OLD, &dark, sizeof(dark));
		}
	}

	/* スクロールバー: 非クライアントスクロールバーをダークテーマに */
	if (p_SetWindowTheme != NULL) {
		p_SetWindowTheme(hWnd, enable ? L"DarkMode_Explorer" : NULL, NULL);
	}

	if (p_FlushMenuThemes != NULL) {
		p_FlushMenuThemes();
	}

	/* メニューテーマハンドルは状態変化で取り直す */
	if (g_menuTheme != NULL && p_CloseThemeData != NULL) {
		p_CloseThemeData(g_menuTheme);
	}
	g_menuTheme = NULL;

	/* タイトルバー/フレームを即時再描画 */
	SetWindowPos(hWnd, NULL, 0, 0, 0, 0,
	             SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
	if (GetMenu(hWnd) != NULL) {
		DrawMenuBar(hWnd);
	}
}

BOOL DarkMode_IsActive(void)
{
	return g_active;
}

/* メニュー帯の下に出る明色 1px ラインを暗色で塗り潰す */
static void DarkMode_PaintMenuBottomLine(HWND hWnd)
{
	if (GetMenu(hWnd) == NULL) {
		return;
	}
	MENUBARINFO mbi;
	mbi.cbSize = sizeof(mbi);
	if (!GetMenuBarInfo(hWnd, OBJID_MENU, 0, &mbi)) {
		return;
	}
	RECT rcClient;
	GetClientRect(hWnd, &rcClient);
	MapWindowPoints(hWnd, NULL, (POINT *)&rcClient, 2);

	RECT rcWindow;
	GetWindowRect(hWnd, &rcWindow);

	/* クライアント上端の 1px (メニューとクライアントの境界) */
	RECT rcLine = rcClient;
	rcLine.bottom = rcLine.top;
	rcLine.top--;
	OffsetRect(&rcLine, -rcWindow.left, -rcWindow.top);

	HDC hdc = GetWindowDC(hWnd);
	if (hdc != NULL) {
		FillRect(hdc, &rcLine, g_brBg);
		ReleaseDC(hWnd, hdc);
	}
}

BOOL DarkMode_HandleWindowMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result)
{
	if (!g_active) {
		return FALSE;
	}

	switch (msg) {
	case DM_WM_UAHDRAWMENU: {
		/* メニューバー全体の背景を塗る */
		DM_UAHMENU *pudm = (DM_UAHMENU *)lParam;
		MENUBARINFO mbi;
		mbi.cbSize = sizeof(mbi);
		if (!GetMenuBarInfo(hWnd, OBJID_MENU, 0, &mbi)) {
			return FALSE;
		}
		RECT rcWindow;
		GetWindowRect(hWnd, &rcWindow);
		RECT rc = mbi.rcBar;
		OffsetRect(&rc, -rcWindow.left, -rcWindow.top);
		FillRect(pudm->hdc, &rc, g_brBg);
		if (result) *result = TRUE;
		return TRUE;
	}

	case DM_WM_UAHDRAWMENUITEM: {
		/* メニューバー各項目を描画 */
		DM_UAHDRAWMENUITEM *pudmi = (DM_UAHDRAWMENUITEM *)lParam;

		wchar_t text[256];
		text[0] = L'\0';
		MENUITEMINFOW mii;
		ZeroMemory(&mii, sizeof(mii));
		mii.cbSize     = sizeof(mii);
		mii.fMask      = MIIM_STRING;
		mii.dwTypeData = text;
		mii.cch        = (UINT)(ARRAYSIZE(text) - 1);
		GetMenuItemInfoW(pudmi->um.hmenu, pudmi->umi.iPosition, TRUE, &mii);

		DWORD dtFlags = DT_CENTER | DT_SINGLELINE | DT_VCENTER;
		int textState = MBI_NORMAL;
		int bgState   = MBI_NORMAL;
		if (pudmi->dis.itemState & ODS_HOTLIGHT) {
			textState = bgState = MBI_HOT;
		}
		if (pudmi->dis.itemState & ODS_SELECTED) {
			textState = bgState = MBI_PUSHED;
		}
		if (pudmi->dis.itemState & (ODS_GRAYED | ODS_DISABLED)) {
			textState = bgState = MBI_DISABLED;
		}
		if (pudmi->dis.itemState & ODS_NOACCEL) {
			dtFlags |= DT_HIDEPREFIX;
		}

		if (g_menuTheme == NULL && p_OpenThemeData != NULL) {
			g_menuTheme = p_OpenThemeData(hWnd, L"Menu");
		}

		/* 背景 */
		if (bgState == MBI_HOT || bgState == MBI_PUSHED) {
			FillRect(pudmi->um.hdc, &pudmi->dis.rcItem, g_brHot);
		}
		else {
			FillRect(pudmi->um.hdc, &pudmi->dis.rcItem, g_brBg);
		}

		/* 文字 */
		DTTOPTS opts;
		ZeroMemory(&opts, sizeof(opts));
		opts.dwSize  = sizeof(opts);
		opts.dwFlags = DTT_TEXTCOLOR;
		opts.crText  = (textState == MBI_DISABLED) ? DM_CLR_TEXT_DIS : DM_CLR_TEXT;

		if (g_menuTheme != NULL && p_DrawThemeTextEx != NULL) {
			p_DrawThemeTextEx(g_menuTheme, pudmi->um.hdc, MENU_BARITEM, textState,
			                  text, mii.cch, dtFlags, &pudmi->dis.rcItem, &opts);
		}
		else {
			/* テーマが取れない場合の素朴なフォールバック */
			SetBkMode(pudmi->um.hdc, TRANSPARENT);
			SetTextColor(pudmi->um.hdc, opts.crText);
			DrawTextW(pudmi->um.hdc, text, (int)mii.cch, &pudmi->dis.rcItem, dtFlags);
		}

		if (result) *result = TRUE;
		return TRUE;
	}

	case WM_NCACTIVATE:
	case WM_NCPAINT: {
		/* 既定の枠描画後にメニュー下端の明色ラインを塗り直す */
		LRESULT lr = DefWindowProcW(hWnd, msg, wParam, lParam);
		DarkMode_PaintMenuBottomLine(hWnd);
		if (result) *result = lr;
		return TRUE;
	}

	case WM_THEMECHANGED:
		/* テーマ再取得 (描画は既定処理に任せる) */
		if (g_menuTheme != NULL && p_CloseThemeData != NULL) {
			p_CloseThemeData(g_menuTheme);
		}
		g_menuTheme = NULL;
		return FALSE;

	default:
		return FALSE;
	}
}
