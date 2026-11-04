/* Darshan File Manager 2.0 - by Darshan Corporation
   Plain Win32 (Unicode). Build:
   x86_64-w64-mingw32-windres app.rc -O coff -o app.res
   x86_64-w64-mingw32-gcc -municode -mwindows -static -O2 -s main.c core.c lang.c app.res -o DarshanFileManager.exe -lcomctl32 -lshell32 -lole32 -luuid -lgdi32 -ladvapi32
*/
#include "core.h"
#include "lang.h"
#include <shellapi.h>
#include <shlobj.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "resource.h"

#define APP_NAME   L"Darshan File Manager"
#define APP_TITLE  L"Darshan File Manager 2.0"
#define APP_CLASS  L"DarshanFileManagerWnd"
#define RUN_VALUE  L"DarshanFileManager"
#define WM_TRAY    (WM_APP + 1)
#define WM_WAKEUP  (WM_APP + 2)
#define WM_STARTRUN (WM_APP + 3)
#define TIMER_AUTO 1
#define MUTEX_NAME L"DarshanFileManager_SingleInstance"

enum {
  IDC_PATH = 101, IDC_BROWSE, IDC_LBL_FOLDER, IDC_STATUS, IDC_LOG, IDC_LBL_ACT,
  IDC_TAB0 = 120,                                  /* 120..126 */
  IDC_DATE = 140, IDC_EXT, IDC_TYPE, IDC_SDATE, IDC_UNDO, IDC_OPEN, IDC_ADMIN,
  IDC_AUTO, IDC_INTERVAL, IDC_R_EXT, IDC_R_TYPE, IDC_TRAYCHK, IDC_STARTUP,
  IDC_A_RULES, IDC_A_SORT, IDC_A_DATE, IDC_A_OLD, IDC_A_DUP,
  IDC_DUP_SUB = 170, IDC_DUP_SCAN, IDC_DUP_MOVE, IDC_DUP_UNDO, IDC_DUP_SUM, IDC_DUP_LIST,
  IDC_OLD_DAYS = 180, IDC_OLD_CUSTOM, IDC_OLD_PREV, IDC_OLD_MOVE, IDC_OLD_UNDO, IDC_OLD_SUM, IDC_OLD_LIST,
  IDC_REN_CONTAINS = 190, IDC_REN_EXT, IDC_REN_PREFIX, IDC_REN_SUFFIX, IDC_REN_FIND, IDC_REN_REPL,
  IDC_REN_NUM, IDC_REN_BASE, IDC_REN_START, IDC_REN_DIGITS, IDC_REN_PREV, IDC_REN_APPLY, IDC_REN_UNDO, IDC_REN_SUM, IDC_REN_LIST,
  IDC_RULE_USE = 210, IDC_RULE_LIST, IDC_RULE_TYPE, IDC_RULE_PAT, IDC_RULE_FOLDER, IDC_RULE_ADD, IDC_RULE_UPD, IDC_RULE_DEL,
  IDC_STO_SUB = 220, IDC_STO_GO, IDC_STO_SUM, IDC_CHART,
  IDC_THEME = 230, IDC_COLOR, IDC_LANG,
  IDC_STATIC0 = 300,
  IDM_OPEN = 500, IDM_EXIT
};
#define NPAGES 7
enum { PG_ALL = -1, PG_SORT = 0, PG_DUP, PG_OLD, PG_REN, PG_RULES, PG_STO, PG_SET };
enum { K_STATIC, K_BTN, K_PRIMARY, K_CHECK, K_RADIO, K_EDIT, K_COMBO, K_LIST, K_SECTION, K_HINT, K_TAB, K_OTHER };

typedef struct { HWND h; int page; int sid; const wchar_t *icon; int kind; int fx, fy; } Ctl;   /* fx/fy: painted frame around the control */
static Ctl g_ctl[200]; static int g_nctl = 0;

static HINSTANCE g_inst;
static HWND g_wnd, g_path, g_log, g_status, g_auto, g_interval, g_rext, g_rtype, g_tray, g_startup, g_adminBtn;
static HWND g_aRules, g_aSort, g_aDate, g_aOld, g_aDup;
static HWND g_dupSub, g_dupSum, g_dupList, g_oldDays, g_oldCustom, g_oldSum, g_oldList;
static HWND g_ren[6], g_renNum, g_renBase, g_renStart, g_renDigits, g_renSum, g_renList;
static HWND g_ruleUse, g_ruleList, g_ruleType, g_rulePat, g_ruleFolder;
static HWND g_stoSub, g_stoSum, g_chart, g_theme, g_color, g_langCombo;
static HFONT g_font, g_fontBold, g_fontHead, g_fontSub, g_fontEmoji, g_fontSection, g_fontGu;
static HBRUSH g_brBg, g_brField;
static int g_dpi = 96, g_page = 0, g_inTray = 0, g_trayTipShown = 0, g_elevated = 0, g_deniedNoticeShown = 0;
static int g_filling = 0, g_cliMode = 0, g_startPage = -1, g_startRun = 0;
static wchar_t g_cliFolder[PBUF];
static const int g_intervals[] = { 1, 5, 15, 30 };
static const int g_dayOpts[] = { 30, 60, 90, 180, 365 };
static HANDLE g_mutex;
static DupResult g_dup; static FList g_old; static RenamePlan g_plan;
static CatStat g_stats[NCATS_MAX]; static int g_statsFiles = -1; static ULONGLONG g_statsTotal = 0;

#define S(x) MulDiv((x), g_dpi, 96)
#define CW 680
#define CH 664

/* ------------------------------------------------------------------ */
/* Theme                                                               */
/* ------------------------------------------------------------------ */
typedef struct { COLORREF bg, text, sub, field, fieldText, btn, btnPress, btnBorder, btnText, line, head, track; } Pal;
typedef struct { int sid; COLORREF base, press, on, textL, textD, line, hsub; } Accent;
static const Pal PAL_LIGHT = { RGB(255,255,255), RGB(30,30,36), RGB(95,95,104), RGB(255,255,255), RGB(30,30,36),
                               RGB(242,242,245), RGB(222,222,227), RGB(200,200,206), RGB(40,40,47), RGB(227,227,232), RGB(243,243,246), RGB(236,236,240) };
static const Pal PAL_DARK  = { RGB(31,31,35), RGB(236,236,240), RGB(164,164,174), RGB(43,43,48), RGB(236,236,240),
                               RGB(52,52,59), RGB(69,69,78), RGB(82,82,92), RGB(238,238,242), RGB(58,58,66), RGB(46,46,52), RGB(48,48,55) };
static const Accent ACCENTS[] = {
  { S_C_MAROON, RGB(123,30,58),  RGB(90,16,41),  RGB(255,255,255), RGB(123,30,58), RGB(232,135,159), RGB(212,166,62),  RGB(240,205,120) },
  { S_C_GOLD,   RGB(247,181,0),  RGB(212,155,0), RGB(43,29,0),     RGB(138,100,0),  RGB(247,181,0),   RGB(123,30,58),   RGB(74,46,0) },
  { S_C_BLUE,   RGB(30,99,201),  RGB(22,76,156), RGB(255,255,255), RGB(26,91,191),  RGB(127,178,255), RGB(142,197,255), RGB(210,230,255) },
  { S_C_GREEN,  RGB(30,126,70),  RGB(21,94,52),  RGB(255,255,255), RGB(27,115,65),  RGB(111,214,155), RGB(168,230,184), RGB(212,245,222) },
  { S_C_PURPLE, RGB(106,27,154), RGB(78,18,115), RGB(255,255,255), RGB(106,27,154), RGB(199,146,234), RGB(224,184,255), RGB(235,217,250) },
  { S_C_ORANGE, RGB(194,65,12),  RGB(154,52,9),  RGB(255,255,255), RGB(178,60,11),  RGB(255,158,94),  RGB(255,208,138), RGB(255,227,194) },
};
#define NACCENTS 6
static int g_themeSel = 2, g_accentSel = 0, g_dark = 0;
static Pal P; static Accent A;
static COLORREF accent_text(void) { return g_dark ? A.textD : A.textL; }

static int system_is_dark(void) {
  HKEY k; DWORD v = 1, sz = sizeof v, type = 0;
  if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", 0, KEY_READ, &k) == ERROR_SUCCESS) {
    if (RegQueryValueExW(k, L"AppsUseLightTheme", NULL, &type, (BYTE *)&v, &sz) != ERROR_SUCCESS || type != REG_DWORD) v = 1;
    RegCloseKey(k);
  }
  return v == 0;
}
static void set_window_theme(HWND h, const wchar_t *name) {
  typedef HRESULT (WINAPI *SWT)(HWND, LPCWSTR, LPCWSTR);
  static SWT f = NULL; static int tried = 0;
  if (!tried) { HMODULE m = LoadLibraryW(L"uxtheme.dll"); if (m) f = (SWT)(void *)GetProcAddress(m, "SetWindowTheme"); tried = 1; }
  if (f && h) f(h, name, NULL);
}
static void dark_title_bar(HWND h, int dark) {
  typedef HRESULT (WINAPI *DSWA)(HWND, DWORD, LPCVOID, DWORD);
  static DSWA f = NULL; static int tried = 0;
  if (!tried) { HMODULE m = LoadLibraryW(L"dwmapi.dll"); if (m) f = (DSWA)(void *)GetProcAddress(m, "DwmSetWindowAttribute"); tried = 1; }
  if (!f) return;
  BOOL v = dark ? TRUE : FALSE;
  if (FAILED(f(h, 20, &v, sizeof v))) f(h, 19, &v, sizeof v);   /* DWMWA_USE_IMMERSIVE_DARK_MODE (20H1+), older builds 19 */
}

/* ------------------------------------------------------------------ */
/* Settings                                                            */
/* ------------------------------------------------------------------ */
static int ini_int(const wchar_t *k, int def) { return (int)GetPrivateProfileIntW(L"Settings", k, def, g_ini); }
static void ini_set_int(const wchar_t *k, int v) { wchar_t b[24]; _snwprintf(b, 24, L"%d", v); b[23] = 0; WritePrivateProfileStringW(L"Settings", k, b, g_ini); }
static void ini_str(const wchar_t *k, const wchar_t *def, wchar_t *out, int n) { GetPrivateProfileStringW(L"Settings", k, def, out, n, g_ini); }
static void ini_set_str(const wchar_t *k, const wchar_t *v) { WritePrivateProfileStringW(L"Settings", k, v, g_ini); }

static int chk(HWND h) { return h ? (int)GetWindowLongPtrW(h, GWLP_USERDATA) : 0; }
static void chk_set(HWND h, int v) { if (h) { SetWindowLongPtrW(h, GWLP_USERDATA, v ? 1 : 0); InvalidateRect(h, NULL, FALSE); } }
static void get_text(HWND h, wchar_t *out, int n) { GetWindowTextW(h, out, n); out[n-1] = 0; }

static int old_days(void) {
  if (!g_oldDays) return ini_int(L"OldDays", 90);
  int sel = (int)SendMessageW(g_oldDays, CB_GETCURSEL, 0, 0);
  if (sel >= 0 && sel < 5) return g_dayOpts[sel];
  wchar_t b[16]; GetWindowTextW(g_oldCustom, b, 16); int d = _wtoi(b);
  return d < 1 ? 1 : (d > 36500 ? 36500 : d);
}

static void save_settings(void) {
  if (!g_path || !g_renDigits) return;
  wchar_t f[PBUF]; GetWindowTextW(g_path, f, PBUF);
  ini_set_str(L"Folder", f);
  ini_set_int(L"AutoMode", chk(g_auto));
  int sel = (int)SendMessageW(g_interval, CB_GETCURSEL, 0, 0); if (sel < 0) sel = 1;
  ini_set_int(L"IntervalMinutes", g_intervals[sel]);
  ini_set_int(L"AutoSort", chk(g_rext) ? 0 : 1);
  ini_set_int(L"MinimizeToTray", chk(g_tray));
  ini_set_int(L"AutoRules", chk(g_aRules)); ini_set_int(L"AutoDoSort", chk(g_aSort));
  ini_set_int(L"AutoDate", chk(g_aDate)); ini_set_int(L"AutoOld", chk(g_aOld)); ini_set_int(L"AutoDupes", chk(g_aDup));
  ini_set_int(L"OldDays", old_days());
  ini_set_int(L"DupSub", chk(g_dupSub)); ini_set_int(L"StoSub", chk(g_stoSub)); ini_set_int(L"UseRules", chk(g_ruleUse));
  ini_set_int(L"Theme", g_themeSel); ini_set_int(L"Accent", g_accentSel); ini_set_int(L"Language", g_lang);
  static const wchar_t *rk[6] = { L"RenContains", L"RenExt", L"RenPrefix", L"RenSuffix", L"RenFind", L"RenReplace" };
  for (int i = 0; i < 6; i++) { wchar_t b[200]; get_text(g_ren[i], b, 200); ini_set_str(rk[i], b); }
  { wchar_t b[200]; get_text(g_renBase, b, 200); ini_set_str(L"RenBase", b);
    get_text(g_renStart, b, 200); ini_set_int(L"RenStart", _wtoi(b));
    get_text(g_renDigits, b, 200); ini_set_int(L"RenDigits", _wtoi(b)); }
  ini_set_int(L"RenNumbering", chk(g_renNum));
}

/* ------------------------------------------------------------------ */
/* Start with Windows / elevation                                      */
/* ------------------------------------------------------------------ */
static int startup_is_on(void) {
  HKEY k; int on = 0;
  if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_READ, &k) == ERROR_SUCCESS) {
    on = RegQueryValueExW(k, RUN_VALUE, NULL, NULL, NULL, NULL) == ERROR_SUCCESS; RegCloseKey(k);
  }
  return on;
}
static int startup_set(int on) {
  HKEY k; LONG r;
  if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, NULL, 0, KEY_SET_VALUE, NULL, &k, NULL) != ERROR_SUCCESS) return 0;
  if (on) {
    wchar_t v[PBUF + 16]; _snwprintf(v, PBUF + 16, L"\"%s\" --tray", g_self); v[PBUF+15] = 0;
    r = RegSetValueExW(k, RUN_VALUE, 0, REG_SZ, (const BYTE *)v, (DWORD)((wcslen(v) + 1) * sizeof(wchar_t)));
  } else {
    r = RegDeleteValueW(k, RUN_VALUE); if (r == ERROR_FILE_NOT_FOUND) r = ERROR_SUCCESS;
  }
  RegCloseKey(k); return r == ERROR_SUCCESS;
}
static int is_elevated(void) {
#ifdef FORCE_NOT_ELEVATED
  return 0;
#endif
  HANDLE tok; TOKEN_ELEVATION e; DWORD sz = 0; int r = 0;
  if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &tok)) {
    if (GetTokenInformation(tok, TokenElevation, &e, sizeof e, &sz)) r = e.TokenIsElevated != 0;
    CloseHandle(tok);
  }
  return r;
}

/* ------------------------------------------------------------------ */
/* Log / status (window) or stdout (command line)                      */
/* ------------------------------------------------------------------ */
static void now_str(wchar_t *out, int n) { SYSTEMTIME st; GetLocalTime(&st); _snwprintf(out, n, L"%02d:%02d", st.wHour, st.wMinute); out[n-1] = 0; }
static void out_utf8(const wchar_t *s) {
  HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
  if (!h || h == INVALID_HANDLE_VALUE) return;
  int n = WideCharToMultiByte(CP_UTF8, 0, s, -1, NULL, 0, NULL, NULL);
  char *b = (char *)malloc(n + 2); if (!b) return;
  WideCharToMultiByte(CP_UTF8, 0, s, -1, b, n, NULL, NULL);
  DWORD w; WriteFile(h, b, (DWORD)strlen(b), &w, NULL); WriteFile(h, "\n", 1, &w, NULL);
  free(b);
}
static void outf(const wchar_t *fmt, ...) {
  wchar_t buf[PBUF * 2]; va_list ap; va_start(ap, fmt); _vsnwprintf(buf, PBUF * 2, fmt, ap); va_end(ap); buf[PBUF*2-1] = 0;
  out_utf8(buf);
}
void log_line(const wchar_t *fmt, ...) {
  wchar_t buf[PBUF], line[PBUF + 16], t[16];
  va_list ap; va_start(ap, fmt); _vsnwprintf(buf, PBUF, fmt, ap); va_end(ap); buf[PBUF-1] = 0;
  if (g_cliMode) { out_utf8(buf); return; }
  if (!g_log) return;
  now_str(t, 16);
  _snwprintf(line, PBUF + 16, L"%s   %s", t, buf); line[PBUF+15] = 0;
  int c = (int)SendMessageW(g_log, LB_ADDSTRING, 0, (LPARAM)line);
  while (SendMessageW(g_log, LB_GETCOUNT, 0, 0) > 1500) { SendMessageW(g_log, LB_DELETESTRING, 0, 0); c--; }
  SendMessageW(g_log, LB_SETTOPINDEX, (WPARAM)(c > 0 ? c : 0), 0);
}
static void set_status(const wchar_t *fmt, ...) {
  wchar_t buf[PBUF]; va_list ap; va_start(ap, fmt); _vsnwprintf(buf, PBUF, fmt, ap); va_end(ap); buf[PBUF-1] = 0;
  if (g_status) SetWindowTextW(g_status, buf);
}

/* ------------------------------------------------------------------ */
/* Tray                                                                */
/* ------------------------------------------------------------------ */
static void tray_icon(int add, const wchar_t *balloon) {
  NOTIFYICONDATAW nid; ZeroMemory(&nid, sizeof nid);
  nid.cbSize = sizeof nid; nid.hWnd = g_wnd; nid.uID = 1;
  if (!add) { Shell_NotifyIconW(NIM_DELETE, &nid); return; }
  nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
  nid.uCallbackMessage = WM_TRAY;
  nid.hIcon = (HICON)LoadImageW(g_inst, MAKEINTRESOURCEW(IDI_APP), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
  wcscpy(nid.szTip, L"Darshan File Manager - Auto mode");
  if (balloon) { nid.uFlags |= NIF_INFO; wcsncpy(nid.szInfo, balloon, 255); wcscpy(nid.szInfoTitle, APP_NAME); nid.dwInfoFlags = NIIF_INFO; }
  if (!Shell_NotifyIconW(NIM_MODIFY, &nid)) Shell_NotifyIconW(NIM_ADD, &nid);
}
static void go_to_tray(void) {
  if (g_inTray) return;
  g_inTray = 1; ShowWindow(g_wnd, SW_HIDE);
  tray_icon(1, g_trayTipShown ? NULL : L_(S_TRAY_TIP));
  g_trayTipShown = 1;
}
static void restore_from_tray(void) {
  if (g_inTray) { tray_icon(0, NULL); g_inTray = 0; }
  ShowWindow(g_wnd, SW_SHOW); ShowWindow(g_wnd, SW_RESTORE); SetForegroundWindow(g_wnd);
}

/* ------------------------------------------------------------------ */
/* Fonts                                                               */
/* ------------------------------------------------------------------ */
static HFONT mkfont(int pt, int weight, const wchar_t *face) {
  return CreateFontW(-MulDiv(pt, g_dpi, 72), 0, 0, 0, weight, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, face);
}
static void make_fonts(void) {
  HFONT old[7] = { g_font, g_fontBold, g_fontHead, g_fontSub, g_fontEmoji, g_fontSection, g_fontGu };
  /* Gujarati needs a font with the script: Nirmala UI (Windows 8+); Windows 7 falls back to Shruti */
  const wchar_t *face = g_lang == 1 ? L"Nirmala UI" : L"Segoe UI";
  g_font = mkfont(10, FW_NORMAL, face);
  g_fontBold = mkfont(10, FW_SEMIBOLD, face);
  g_fontHead = mkfont(17, FW_SEMIBOLD, L"Segoe UI");
  g_fontSub = mkfont(9, FW_NORMAL, face);
  g_fontEmoji = mkfont(11, FW_NORMAL, L"Segoe UI Emoji");
  g_fontSection = mkfont(11, FW_SEMIBOLD, face);
  g_fontGu = mkfont(10, FW_NORMAL, L"Nirmala UI");
  for (int i = 0; i < 7; i++) if (old[i]) DeleteObject(old[i]);
}
static int has_gujarati(const wchar_t *s) { for (; *s; s++) if (*s >= 0x0A80 && *s <= 0x0AFF) return 1; return 0; }

/* ------------------------------------------------------------------ */
/* Control creation                                                    */
/* ------------------------------------------------------------------ */
static HFONT font_for_kind(int kind) { return kind == K_SECTION ? g_fontSection : (kind == K_HINT ? g_fontSub : g_font); }
static Ctl *find_ctl(HWND h) { for (int i = 0; i < g_nctl; i++) if (g_ctl[i].h == h) return &g_ctl[i]; return NULL; }
static void ctl_text(Ctl *c) {
  if (!c->sid) return;
  wchar_t b[400];
  if (c->icon) _snwprintf(b, 400, L"%s  %s", c->icon, L_(c->sid)); else wcsncpy(b, L_(c->sid), 399);
  b[399] = 0;
  if (c->h == g_aOld) _snwprintf(b, 400, L_(S_A_OLD), old_days());
  SetWindowTextW(c->h, b);
}
static int g_id_next = IDC_STATIC0;
static HWND mkc(int page, int kind, int sid, const wchar_t *icon, int x, int y, int w, int h, int id) {
  const wchar_t *cls = L"BUTTON"; DWORD st = WS_CHILD | WS_TABSTOP, ex = 0;
  switch (kind) {
  case K_STATIC: case K_SECTION: case K_HINT: cls = L"STATIC"; st = WS_CHILD | SS_LEFTNOWORDWRAP | SS_NOPREFIX | SS_ENDELLIPSIS; break;
  case K_BTN: case K_PRIMARY: case K_TAB: case K_CHECK: case K_RADIO: st |= BS_OWNERDRAW; break;
  case K_EDIT: cls = L"EDIT"; st |= ES_AUTOHSCROLL; x += 7; y += 5; w -= 14; h -= 9; break;   /* frame painted by the window */
  case K_COMBO: cls = L"COMBOBOX"; st |= CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL; h = 300; break;
  }
  if (id == 0) id = g_id_next++;
  HWND c = CreateWindowExW(ex, cls, L"", st | (page == PG_ALL || page == g_page ? WS_VISIBLE : 0), S(x), S(y), S(w), S(h), g_wnd, (HMENU)(INT_PTR)id, g_inst, NULL);
  if (g_nctl < (int)(sizeof g_ctl / sizeof g_ctl[0])) { Ctl *x2 = &g_ctl[g_nctl++]; x2->h = c; x2->page = page; x2->sid = sid; x2->icon = icon; x2->kind = kind;
    x2->fx = kind == K_EDIT ? 7 : 0; x2->fy = kind == K_EDIT ? 5 : 0; ctl_text(x2); }
  SendMessageW(c, WM_SETFONT, (WPARAM)font_for_kind(kind), TRUE);
  return c;
}
static HWND mklist(int page, int x, int y, int w, int h, int id, int checks) {
  HWND c = CreateWindowExW(0, WC_LISTVIEWW, L"", WS_CHILD | WS_TABSTOP | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL |
                           (page == g_page ? WS_VISIBLE : 0), S(x + 1), S(y + 1), S(w - 2), S(h - 2), g_wnd, (HMENU)(INT_PTR)id, g_inst, NULL);
  ListView_SetExtendedListViewStyle(c, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | (checks ? LVS_EX_CHECKBOXES : 0));
  if (g_nctl < (int)(sizeof g_ctl / sizeof g_ctl[0])) { Ctl *x2 = &g_ctl[g_nctl++]; x2->h = c; x2->page = page; x2->sid = 0; x2->icon = NULL; x2->kind = K_LIST; x2->fx = 1; x2->fy = 1; }
  SendMessageW(c, WM_SETFONT, (WPARAM)g_font, TRUE);
  return c;
}
typedef struct { HWND lv; int n; int sid[6]; int w[6]; } LvCols;
static LvCols g_cols[8]; static int g_ncols = 0;
static void lv_set_cols(HWND lv, int n, const int *sid, const int *w) {
  LvCols *c = NULL;
  for (int i = 0; i < g_ncols; i++) if (g_cols[i].lv == lv) c = &g_cols[i];
  int isnew = c == NULL;
  if (!c) { c = &g_cols[g_ncols++]; c->lv = lv; c->n = n; for (int i = 0; i < n; i++) { c->sid[i] = sid[i]; c->w[i] = w[i]; } }
  for (int i = 0; i < c->n; i++) {
    LVCOLUMNW col; ZeroMemory(&col, sizeof col);
    col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT; col.pszText = (wchar_t *)L_(c->sid[i]); col.cx = S(c->w[i]);
    col.fmt = (c->sid[i] == S_COL_SIZE) ? LVCFMT_RIGHT : LVCFMT_LEFT;
    if (isnew) SendMessageW(lv, LVM_INSERTCOLUMNW, i, (LPARAM)&col);
    else { col.mask = LVCF_TEXT; SendMessageW(lv, LVM_SETCOLUMNW, i, (LPARAM)&col); }
  }
}
static int lv_add(HWND lv, const wchar_t *c0) {
  LVITEMW it; ZeroMemory(&it, sizeof it);
  it.mask = LVIF_TEXT; it.iItem = (int)SendMessageW(lv, LVM_GETITEMCOUNT, 0, 0); it.pszText = (wchar_t *)c0;
  return (int)SendMessageW(lv, LVM_INSERTITEMW, 0, (LPARAM)&it);
}
static void lv_set(HWND lv, int row, int col, const wchar_t *t) {
  LVITEMW it; ZeroMemory(&it, sizeof it); it.iSubItem = col; it.pszText = (wchar_t *)t;
  SendMessageW(lv, LVM_SETITEMTEXTW, row, (LPARAM)&it);
}
typedef struct { HWND h; int n; int sid[8]; } ComboItems;
static ComboItems g_combos[8]; static int g_ncombos = 0;
static void combo_fill(HWND h, int n, const int *sid) {   /* localized items; keeps the selection */
  ComboItems *c = NULL;
  for (int i = 0; i < g_ncombos; i++) if (g_combos[i].h == h) c = &g_combos[i];
  if (!c && n) { c = &g_combos[g_ncombos++]; c->h = h; c->n = n; for (int i = 0; i < n; i++) c->sid[i] = sid[i]; }
  if (!c) return;
  int sel = (int)SendMessageW(h, CB_GETCURSEL, 0, 0);
  SendMessageW(h, CB_RESETCONTENT, 0, 0);
  for (int i = 0; i < c->n; i++) SendMessageW(h, CB_ADDSTRING, 0, (LPARAM)L_(c->sid[i]));
  if (sel >= 0) SendMessageW(h, CB_SETCURSEL, sel, 0);
}

/* list view header colours follow the theme (subclass: the header notifies the list view, not us) */
static WNDPROC g_lvOrig = NULL;
static LRESULT CALLBACK lv_proc(HWND h, UINT m, WPARAM w, LPARAM l) {
  if (m == WM_NOTIFY) {
    NMHDR *nh = (NMHDR *)l;
    if (nh->code == NM_CUSTOMDRAW && nh->hwndFrom == ListView_GetHeader(h)) {
      NMCUSTOMDRAW *cd = (NMCUSTOMDRAW *)l;
      if (cd->dwDrawStage == CDDS_PREPAINT) {
        RECT rc; GetClientRect(nh->hwndFrom, &rc);
        HBRUSH b = CreateSolidBrush(P.head); FillRect(cd->hdc, &rc, b); DeleteObject(b);
        return CDRF_NOTIFYITEMDRAW | CDRF_NOTIFYPOSTPAINT;
      }
      if (cd->dwDrawStage == CDDS_POSTPAINT) {   /* the empty part right of the last column */
        RECT rc, last; GetClientRect(nh->hwndFrom, &rc);
        int n = (int)SendMessageW(nh->hwndFrom, HDM_GETITEMCOUNT, 0, 0);
        if (n > 0 && SendMessageW(nh->hwndFrom, HDM_GETITEMRECT, n - 1, (LPARAM)&last)) rc.left = last.right;
        HBRUSH b = CreateSolidBrush(P.head); FillRect(cd->hdc, &rc, b); DeleteObject(b);
        HPEN pen = CreatePen(PS_SOLID, 1, P.line); HGDIOBJ op = SelectObject(cd->hdc, pen);
        MoveToEx(cd->hdc, rc.left, rc.bottom - 1, NULL); LineTo(cd->hdc, rc.right, rc.bottom - 1);
        SelectObject(cd->hdc, op); DeleteObject(pen);
        return CDRF_DODEFAULT;
      }
      if (cd->dwDrawStage == CDDS_ITEMPREPAINT) {
        RECT rc = cd->rc; wchar_t t[128] = L"";
        HDITEMW hi; ZeroMemory(&hi, sizeof hi); hi.mask = HDI_TEXT | HDI_FORMAT; hi.pszText = t; hi.cchTextMax = 128;
        SendMessageW(nh->hwndFrom, HDM_GETITEMW, cd->dwItemSpec, (LPARAM)&hi);
        HBRUSH b = CreateSolidBrush(P.head); FillRect(cd->hdc, &rc, b); DeleteObject(b);
        HPEN pen = CreatePen(PS_SOLID, 1, P.line); HGDIOBJ op = SelectObject(cd->hdc, pen);
        MoveToEx(cd->hdc, rc.right - 1, rc.top + S(4), NULL); LineTo(cd->hdc, rc.right - 1, rc.bottom - S(4));
        MoveToEx(cd->hdc, rc.left, rc.bottom - 1, NULL); LineTo(cd->hdc, rc.right, rc.bottom - 1);
        SelectObject(cd->hdc, op); DeleteObject(pen);
        SetBkMode(cd->hdc, TRANSPARENT); SetTextColor(cd->hdc, P.sub);
        HGDIOBJ of = SelectObject(cd->hdc, g_fontBold);
        rc.left += S(6); rc.right -= S(6);
        DrawTextW(cd->hdc, t, -1, &rc, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX | ((hi.fmt & HDF_RIGHT) ? DT_RIGHT : DT_LEFT));
        SelectObject(cd->hdc, of);
        return CDRF_SKIPDEFAULT;
      }
    }
  }
  return CallWindowProcW(g_lvOrig, h, m, w, l);
}
static void lv_hook(HWND lv) { WNDPROC o = (WNDPROC)SetWindowLongPtrW(lv, GWLP_WNDPROC, (LONG_PTR)lv_proc); if (!g_lvOrig) g_lvOrig = o; }

/* ------------------------------------------------------------------ */
/* Apply theme / language / page                                       */
/* ------------------------------------------------------------------ */
static void apply_theme(void) {
  g_dark = g_themeSel == 2 ? system_is_dark() : g_themeSel == 1;
  P = g_dark ? PAL_DARK : PAL_LIGHT;
  if (g_accentSel < 0 || g_accentSel >= NACCENTS) g_accentSel = 0;
  A = ACCENTS[g_accentSel];
  if (g_brBg) DeleteObject(g_brBg);
  if (g_brField) DeleteObject(g_brField);
  g_brBg = CreateSolidBrush(P.bg); g_brField = CreateSolidBrush(P.field);
  if (!g_wnd) return;
  for (int i = 0; i < g_nctl; i++) {
    Ctl *c = &g_ctl[i];
    if (c->kind == K_LIST) {
      ListView_SetBkColor(c->h, P.field); ListView_SetTextBkColor(c->h, P.field); ListView_SetTextColor(c->h, P.fieldText);
      set_window_theme(c->h, g_dark ? L"DarkMode_Explorer" : L"Explorer");
    } else if (c->kind == K_EDIT || c->kind == K_COMBO) set_window_theme(c->h, g_dark ? L"DarkMode_CFD" : NULL);
  }
  set_window_theme(g_log, g_dark ? L"DarkMode_Explorer" : L"Explorer");
  dark_title_bar(g_wnd, g_dark);
  RedrawWindow(g_wnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_FRAME | RDW_UPDATENOW);
}
static void refresh_rule_list(void);
static void apply_language(void) {
  make_fonts();
  for (int i = 0; i < g_nctl; i++) {
    Ctl *c = &g_ctl[i];
    SendMessageW(c->h, WM_SETFONT, (WPARAM)font_for_kind(c->kind), FALSE);
    ctl_text(c);
  }
  SendMessageW(g_log, WM_SETFONT, (WPARAM)g_font, FALSE);
  SendMessageW(g_status, WM_SETFONT, (WPARAM)g_font, FALSE);
  for (int i = 0; i < g_ncombos; i++) {
    combo_fill(g_combos[i].h, 0, NULL);
    SendMessageW(g_combos[i].h, CB_SETITEMHEIGHT, (WPARAM)-1, S(22));
    SendMessageW(g_combos[i].h, CB_SETITEMHEIGHT, 0, S(22));
  }
  for (int i = 0; i < g_ncols; i++) lv_set_cols(g_cols[i].lv, 0, NULL, NULL);
  refresh_rule_list();
  RedrawWindow(g_wnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_UPDATENOW);
}
static void show_page(int p) {
  if (p < 0 || p >= NPAGES) p = 0;
  g_page = p;
  for (int i = 0; i < g_nctl; i++) if (g_ctl[i].page >= 0) ShowWindow(g_ctl[i].h, g_ctl[i].page == p ? SW_SHOWNA : SW_HIDE);
  for (int i = 0; i < NPAGES; i++) InvalidateRect(GetDlgItem(g_wnd, IDC_TAB0 + i), NULL, FALSE);
  InvalidateRect(g_wnd, NULL, TRUE);
  ini_set_int(L"LastPage", p);
}

/* ------------------------------------------------------------------ */
/* Drawing                                                             */
/* ------------------------------------------------------------------ */
static void round_fill(HDC dc, RECT r, COLORREF fill, COLORREF border, int rad) {
  HBRUSH b = CreateSolidBrush(fill); HPEN p = CreatePen(PS_SOLID, S(1), border);
  HGDIOBJ ob = SelectObject(dc, b), op = SelectObject(dc, p);
  RoundRect(dc, r.left, r.top, r.right, r.bottom, rad, rad);
  SelectObject(dc, ob); SelectObject(dc, op); DeleteObject(b); DeleteObject(p);
}
static void draw_label(HDC dc, RECT r, const wchar_t *txt, HFONT f, COLORREF col, UINT fmt) {
  SetBkMode(dc, TRANSPARENT); SetTextColor(dc, col);
  HGDIOBJ of = SelectObject(dc, f);
  DrawTextW(dc, txt, -1, &r, DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS | fmt);
  SelectObject(dc, of);
}
static void draw_button(const DRAWITEMSTRUCT *di) {
  Ctl *c = find_ctl(di->hwndItem); if (!c) return;
  wchar_t txt[256]; GetWindowTextW(di->hwndItem, txt, 256);
  int pressed = (di->itemState & ODS_SELECTED) != 0, focus = (di->itemState & ODS_FOCUS) != 0;
  int disabled = (di->itemState & ODS_DISABLED) != 0;
  HDC dc = di->hDC; RECT r = di->rcItem;
  FillRect(dc, &r, g_brBg);
  if (c->kind == K_TAB) {
    int sel = (int)(di->CtlID == (UINT)(IDC_TAB0 + g_page));
    RECT tr = r; tr.bottom -= S(3);
    draw_label(dc, tr, txt, sel ? g_fontBold : g_font, sel ? accent_text() : P.sub, DT_CENTER);
    if (sel) { RECT bar = { r.left + S(8), r.bottom - S(3), r.right - S(8), r.bottom }; HBRUSH b = CreateSolidBrush(A.base); FillRect(dc, &bar, b); DeleteObject(b); }
    if (focus && !sel) { RECT fr = r; InflateRect(&fr, -S(6), -S(6)); SetTextColor(dc, P.sub); DrawFocusRect(dc, &fr); }
    return;
  }
  if (c->kind == K_CHECK || c->kind == K_RADIO) {
    int on = chk(di->hwndItem), sz = S(16);
    RECT bx = { r.left + S(1), (r.top + r.bottom - sz) / 2, r.left + S(1) + sz, (r.top + r.bottom - sz) / 2 + sz };
    if (c->kind == K_RADIO) {
      HBRUSH b = CreateSolidBrush(P.field); HPEN p = CreatePen(PS_SOLID, on ? S(2) : S(1), on ? A.base : P.btnBorder);
      HGDIOBJ ob = SelectObject(dc, b), op = SelectObject(dc, p);
      Ellipse(dc, bx.left, bx.top, bx.right, bx.bottom);
      SelectObject(dc, ob); SelectObject(dc, op); DeleteObject(b); DeleteObject(p);
      if (on) { RECT in = bx; InflateRect(&in, -S(4), -S(4)); HBRUSH d = CreateSolidBrush(A.base); HPEN np = CreatePen(PS_SOLID, 1, A.base);
        ob = SelectObject(dc, d); op = SelectObject(dc, np); Ellipse(dc, in.left, in.top, in.right, in.bottom);
        SelectObject(dc, ob); SelectObject(dc, op); DeleteObject(d); DeleteObject(np); }
    } else {
      round_fill(dc, bx, on ? A.base : P.field, on ? A.base : P.btnBorder, S(5));
      if (on) {
        HPEN p = CreatePen(PS_SOLID, S(2), A.on); HGDIOBJ op = SelectObject(dc, p);
        MoveToEx(dc, bx.left + S(4), bx.top + S(8), NULL); LineTo(dc, bx.left + S(7), bx.top + S(11)); LineTo(dc, bx.left + S(12), bx.top + S(5));
        SelectObject(dc, op); DeleteObject(p);
      }
    }
    RECT tr = r; tr.left = bx.right + S(7);
    draw_label(dc, tr, txt, g_font, disabled ? P.sub : P.text, DT_LEFT);
    if (focus) { RECT fr = tr; SIZE ts; HGDIOBJ of = SelectObject(dc, g_font); GetTextExtentPoint32W(dc, txt, (int)wcslen(txt), &ts); SelectObject(dc, of);
      fr.left -= S(2); fr.right = fr.left + ts.cx + S(4); if (fr.right > r.right) fr.right = r.right; fr.top += S(2); fr.bottom -= S(2);
      SetTextColor(dc, P.sub); DrawFocusRect(dc, &fr); }
    return;
  }
  int primary = c->kind == K_PRIMARY;
  COLORREF bg = primary ? (pressed ? A.press : A.base) : (pressed ? P.btnPress : P.btn);
  COLORREF bd = primary ? A.line : P.btnBorder;
  if (primary && g_accentSel == 1) bd = A.press;        /* gold: a darker gold edge, not maroon */
  round_fill(dc, r, bg, bd, S(10));
  COLORREF fg = disabled ? P.sub : (primary ? A.on : P.btnText);
  wchar_t *sep = wcsstr(txt, L"  "), *label = txt, icon[8] = L"";
  if (sep) { int n = (int)(sep - txt); if (n > 7) n = 7; wcsncpy(icon, txt, n); icon[n] = 0; label = sep + 2; }
#ifdef SHOT_NO_EMOJI   /* screenshot builds under Wine, which cannot draw the colour emoji icons */
  if (icon[0] >= 0xD800 && icon[0] <= 0xDBFF) icon[0] = 0;
#endif
  SetBkMode(dc, TRANSPARENT); SetTextColor(dc, fg);
  HFONT lf = primary ? g_fontBold : g_font;
  HGDIOBJ of = SelectObject(dc, lf);
  SIZE ls, is = { 0, 0 }; GetTextExtentPoint32W(dc, label, (int)wcslen(label), &ls);
  if (icon[0]) { SelectObject(dc, g_fontEmoji); GetTextExtentPoint32W(dc, icon, (int)wcslen(icon), &is); is.cx += S(6); }
  int x = (r.left + r.right - (is.cx + ls.cx)) / 2, off = pressed ? S(1) : 0;
  if (x < r.left + S(6)) x = r.left + S(6);
  if (icon[0]) { RECT ir = { x + off, r.top + off, x + is.cx + off, r.bottom + off }; DrawTextW(dc, icon, -1, &ir, DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_NOPREFIX); }
  SelectObject(dc, lf);
  RECT tr = { x + is.cx + off, r.top + off, r.right - S(4), r.bottom + off };
  DrawTextW(dc, label, -1, &tr, DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_NOPREFIX | DT_END_ELLIPSIS);
  SelectObject(dc, of);
  if (focus) { RECT fr = r; InflateRect(&fr, -S(4), -S(4)); SetTextColor(dc, fg); DrawFocusRect(dc, &fr); }
}
static void draw_combo_item(const DRAWITEMSTRUCT *di) {
  HDC dc = di->hDC; RECT r = di->rcItem;
  int sel = (di->itemState & ODS_SELECTED) && !(di->itemState & ODS_COMBOBOXEDIT);
  HBRUSH b = CreateSolidBrush(sel ? A.base : P.field); FillRect(dc, &r, b); DeleteObject(b);
  if ((int)di->itemID < 0) return;
  wchar_t t[200] = L""; SendMessageW(di->hwndItem, CB_GETLBTEXT, di->itemID, (LPARAM)t);
  RECT tr = r; tr.left += S(6);
  if (di->CtlID == IDC_COLOR && di->itemID < NACCENTS) {
    int hgt = r.bottom - r.top - S(8); if (hgt < S(8)) hgt = S(8);
    RECT sw = { r.left + S(6), r.top + S(4), r.left + S(6) + hgt * 2, r.top + S(4) + hgt };
    round_fill(dc, sw, ACCENTS[di->itemID].base, sel ? A.on : P.btnBorder, S(6));
    tr.left = sw.right + S(8);
  }
  draw_label(dc, tr, t, has_gujarati(t) && g_lang == 0 ? g_fontGu : g_font, sel ? A.on : P.fieldText, DT_LEFT);
}
/* the closed combo box is painted here too, so it follows the theme everywhere */
static WNDPROC g_cbOrig = NULL;
static LRESULT CALLBACK cb_proc(HWND h, UINT m, WPARAM w, LPARAM l) {
  if (m == WM_PAINT) {
    PAINTSTRUCT ps; HDC dc = BeginPaint(h, &ps); RECT r; GetClientRect(h, &r);
    FillRect(dc, &r, g_brBg);
    int focus = GetFocus() == h, en = IsWindowEnabled(h);
    round_fill(dc, r, en ? P.field : P.bg, focus ? A.base : P.btnBorder, S(8));
    int sel = (int)SendMessageW(h, CB_GETCURSEL, 0, 0);
    RECT tr = r; tr.left += S(9); tr.right -= S(24);
    if (sel >= 0) {
      wchar_t t[200] = L""; SendMessageW(h, CB_GETLBTEXT, sel, (LPARAM)t);
      if (GetDlgCtrlID(h) == IDC_COLOR && sel < NACCENTS) {
        int hg = r.bottom - r.top - S(12); RECT sw = { tr.left, r.top + S(6), tr.left + hg * 2, r.top + S(6) + hg };
        round_fill(dc, sw, ACCENTS[sel].base, P.btnBorder, S(6)); tr.left = sw.right + S(8);
      }
      draw_label(dc, tr, t, has_gujarati(t) && g_lang == 0 ? g_fontGu : g_font, en ? P.fieldText : P.sub, DT_LEFT);
    }
    int cx = r.right - S(13), cy = (r.top + r.bottom) / 2;
    HPEN p = CreatePen(PS_SOLID, S(2), P.sub); HGDIOBJ op = SelectObject(dc, p);
    MoveToEx(dc, cx - S(4), cy - S(2), NULL); LineTo(dc, cx, cy + S(2)); LineTo(dc, cx + S(5), cy - S(3));
    SelectObject(dc, op); DeleteObject(p);
    EndPaint(h, &ps); return 0;
  }
  if (m == WM_ERASEBKGND) return 1;
  LRESULT res = CallWindowProcW(g_cbOrig, h, m, w, l);
  if (m == WM_SETFOCUS || m == WM_KILLFOCUS || m == CB_SETCURSEL || m == WM_ENABLE) InvalidateRect(h, NULL, FALSE);
  return res;
}
static void draw_chart(const DRAWITEMSTRUCT *di) {
  HDC dc = di->hDC; RECT r = di->rcItem;
  FillRect(dc, &r, g_brBg);
  if (g_statsFiles < 0) { draw_label(dc, r, L_(S_STO_HINT), g_font, P.sub, DT_CENTER); return; }
  int n = cat_count() + 1, order[NCATS_MAX], k = 0;
  for (int i = 0; i < n; i++) if (g_stats[i].count) order[k++] = i;
  for (int i = 0; i < k; i++) for (int j = i + 1; j < k; j++)
    if (g_stats[order[j]].bytes > g_stats[order[i]].bytes) { int t = order[i]; order[i] = order[j]; order[j] = t; }
  if (k == 0) { draw_label(dc, r, L"\x2014", g_font, P.sub, DT_CENTER); return; }
  ULONGLONG mx = g_stats[order[0]].bytes; if (!mx) mx = 1;
  int rowH = S(23), maxRows = (r.bottom - r.top) / rowH; if (k > maxRows) k = maxRows;
  int nameW = S(140), valW = S(190), barL = r.left + nameW, barR = r.right - valW;
  for (int i = 0; i < k; i++) {
    CatStat *cs = &g_stats[order[i]];
    int y = r.top + i * rowH;
    RECT nr = { r.left + S(8), y, barL - S(8), y + rowH };
    draw_label(dc, nr, cat_name(order[i]), g_font, P.text, DT_LEFT);
    RECT track = { barL, y + S(6), barR, y + rowH - S(6) };
    round_fill(dc, track, P.track, P.track, S(8));
    int w = (int)((double)(barR - barL) * (double)cs->bytes / (double)mx); if (w < S(8)) w = S(8);
    RECT bar = { barL, y + S(6), barL + w, y + rowH - S(6) };
    round_fill(dc, bar, A.base, A.base, S(8));
    wchar_t sz[32], v[96]; format_size(cs->bytes, sz, 32);
    _snwprintf(v, 96, L"%s  \x2022  %d %s", sz, cs->count, L_(S_FILES)); v[95] = 0;
    RECT vr = { barR + S(12), y, r.right, y + rowH };
    draw_label(dc, vr, v, g_font, P.sub, DT_LEFT);
  }
}
static void paint_main(HDC dc) {
  RECT r; GetClientRect(g_wnd, &r);
  RECT h = r; h.bottom = S(68);
  HBRUSH b = CreateSolidBrush(A.base); FillRect(dc, &h, b); DeleteObject(b);
  RECT line = h; line.top = h.bottom - S(3);
  b = CreateSolidBrush(A.line); FillRect(dc, &line, b); DeleteObject(b);
  HICON ic = (HICON)LoadImageW(g_inst, MAKEINTRESOURCEW(IDI_APP), IMAGE_ICON, S(40), S(40), 0);
  if (ic) { DrawIconEx(dc, S(16), S(13), ic, S(40), S(40), 0, NULL, DI_NORMAL); DestroyIcon(ic); }
  SetBkMode(dc, TRANSPARENT);
  HFONT of = (HFONT)SelectObject(dc, g_fontHead);
  SetTextColor(dc, A.on);
  TextOutW(dc, S(66), S(8), APP_NAME, (int)wcslen(APP_NAME));
  SIZE ts; GetTextExtentPoint32W(dc, APP_NAME, (int)wcslen(APP_NAME), &ts);
  SelectObject(dc, g_fontSub); SetTextColor(dc, A.hsub);
  const wchar_t *badge = g_elevated ? L"v2.0  (Admin)" : L"v2.0";
  TextOutW(dc, S(66) + ts.cx + S(8), S(17), badge, (int)wcslen(badge));
  const wchar_t *sub = L_(S_APPSUB);
  TextOutW(dc, S(67), S(41), sub, (int)wcslen(sub));
  SelectObject(dc, of);
  RECT tl = { S(12), S(172), r.right - S(12), S(173) };
  b = CreateSolidBrush(P.line); FillRect(dc, &tl, b); DeleteObject(b);
  /* frames around edits, lists and the log (flat, theme coloured, accent when focused) */
  HWND foc = GetFocus();
  for (int i = 0; i < g_nctl; i++) {
    Ctl *c = &g_ctl[i];
    if (!(c->fx || c->fy) || !IsWindowVisible(c->h)) continue;
    RECT fr; GetWindowRect(c->h, &fr); MapWindowPoints(NULL, g_wnd, (POINT *)&fr, 2);
    InflateRect(&fr, S(c->fx), S(c->fy));
    if (c->fx == 1) { InflateRect(&fr, 1, 1); }
    round_fill(dc, fr, c->kind == K_EDIT && !IsWindowEnabled(c->h) ? P.bg : P.field, foc == c->h ? A.base : P.btnBorder, c->kind == K_EDIT ? S(8) : S(6));
  }
}
static void invalidate_frame(HWND h) {
  Ctl *c = find_ctl(h); if (!c) return;
  RECT fr; GetWindowRect(h, &fr); MapWindowPoints(NULL, g_wnd, (POINT *)&fr, 2);
  InflateRect(&fr, S(c->fx) + 2, S(c->fy) + 2);
  InvalidateRect(g_wnd, &fr, TRUE);
}

/* ------------------------------------------------------------------ */
/* Actions                                                             */
/* ------------------------------------------------------------------ */
static int get_folder(wchar_t *out) {
  GetWindowTextW(g_path, out, PBUF);
  size_t l = wcslen(out);
  while (l > 3 && (out[l-1] == L'\\' || out[l-1] == L'/')) out[--l] = 0;
  if (!folder_ok(out)) { set_status(L"%s", L_(S_NOFOLDER)); log_line(L"Folder not found: %s", out); return 0; }
  return 1;
}
static int risky_folder(const wchar_t *f) {
  wchar_t win[MAX_PATH] = L"", up[PBUF] = L"";
  GetWindowsDirectoryW(win, MAX_PATH); GetEnvironmentVariableW(L"USERPROFILE", up, PBUF);
  if (wcslen(f) <= 3) return 1;
  if (win[0] && _wcsnicmp(f, win, wcslen(win)) == 0) return 1;
  if (wcsstr(f, L"\\Program Files") || wcsstr(f, L"\\ProgramData")) return 1;
  if (up[0] && _wcsicmp(f, up) == 0) return 1;
  return 0;
}
static int confirm_risky(const wchar_t *f) {
  return !risky_folder(f) || MessageBoxW(g_wnd, L_(S_RISKY), APP_NAME, MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) == IDYES;
}
static void report_moved(int moved, int errs) {
  if (errs) set_status(L_(S_ST_ERRS), moved, errs);
  else if (moved) set_status(L_(S_ST_MOVED), moved);
  else set_status(L"%s", L_(S_ST_CLEAN));
}
static void denied_notice(int moved, int automatic) {
  if (g_lastDenied <= 0 || (automatic && g_deniedNoticeShown)) return;
  if (!g_elevated) {
    log_line(L"%d shortcut(s) in Public Desktop need admin \x2014 click 'Run as admin' to sort them too", g_lastDenied);
    set_status(L_(S_ADMIN_NEED), moved, g_lastDenied);
    if (g_adminBtn && g_page == PG_SORT) SetFocus(g_adminBtn);
  } else log_line(L"%d Public Desktop item(s) could not be moved (access denied, even as admin)", g_lastDenied);
  g_deniedNoticeShown = 1;
}
static void do_sort(int base) {
  wchar_t f[PBUF]; int errs = 0, br = 0;
  if (!get_folder(f) || !confirm_risky(f)) return;
  int flags = base | (chk(g_ruleUse) && g_nrules ? SF_RULES : 0);
  HCURSOR old = SetCursor(LoadCursor(NULL, IDC_WAIT));
  int moved = run_sort_flags(flags, f, 1, &errs, NULL, &br);
  SetCursor(old);
  if (moved < 0) { log_line(L"Could not read folder %s", f); return; }
  const wchar_t *what = base == SF_EXT ? L_(S_SORTEXT) : base == SF_TYPE ? L_(S_SORTTYPE) : L_(S_SORTDATE);
  if (br) log_line(L"%s: %d file(s) moved, %d by your rules", what, moved, br);
  else log_line(L"%s: %d file(s) moved", what, moved);
  report_moved(moved, errs);
  denied_notice(moved, 0);
}
static void auto_cfg(AutoCfg *c) {
  c->rules = chk(g_aRules) && g_nrules > 0; c->sort = chk(g_aSort); c->sortMode = chk(g_rext) ? SF_EXT : SF_TYPE;
  c->date = chk(g_aDate); c->old = chk(g_aOld); c->oldDays = old_days(); c->dupes = chk(g_aDup);
}
static void summary_text(const AutoSummary *s, wchar_t *out) {
  wchar_t part[128]; out[0] = 0;
  if (s->sorted) { _snwprintf(part, 128, L_(S_SUM_SORTED), s->sorted); part[127] = 0; wcscat(out, part); }
  if (s->dupes) { if (out[0]) wcscat(out, L", "); _snwprintf(part, 128, L_(S_SUM_DUP), s->dupes); part[127] = 0; wcscat(out, part); }
  if (s->old) { if (out[0]) wcscat(out, L", "); _snwprintf(part, 128, L_(S_SUM_OLD), s->old); part[127] = 0; wcscat(out, part); }
}
static void do_auto(void) {
  wchar_t f[PBUF], t[16], sum[400];
  if (!get_folder(f)) return;
  if (risky_folder(f)) { set_status(L"%s", L_(S_AUTO_SKIP)); return; }
  AutoCfg c; AutoSummary s; auto_cfg(&c);
  auto_run(f, &c, 1, &s);
  now_str(t, 16);
  int total = s.sorted + s.dupes + s.old;
  if (total == 0) { if (g_lastDenied == 0 || g_deniedNoticeShown) set_status(L_(S_AUTO_NOTHING), t); denied_notice(0, 1); return; }
  summary_text(&s, sum);
  log_line(L"[%s] %s", L_(S_AUTOMODE), sum);
  set_status(L"%s: %s", L_(S_AUTOMODE), sum);
  if (g_inTray) tray_icon(1, sum);       /* balloon with the same summary */
  denied_notice(s.sorted, 1);
}
static void apply_timer(void) {
  KillTimer(g_wnd, TIMER_AUTO);
  if (chk(g_auto)) {
    int sel = (int)SendMessageW(g_interval, CB_GETCURSEL, 0, 0); if (sel < 0) sel = 1;
    SetTimer(g_wnd, TIMER_AUTO, (UINT)g_intervals[sel] * 60u * 1000u, NULL);
    set_status(L_(S_AUTO_ON), g_intervals[sel]);
  }
}
static void undo_last(void) {
  wchar_t label[128] = L"", q[300];
  if (!exists(g_undoLog)) { set_status(L"%s", L_(S_NOTHING_UNDO)); log_line(L"%s", L_(S_NOTHING_UNDO)); return; }
  { HANDLE h = CreateFileW(g_undoLog, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (h != INVALID_HANDLE_VALUE) { wchar_t b[130]; DWORD rd = 0; ZeroMemory(b, sizeof b); ReadFile(h, b, sizeof b - 4, &rd, NULL); CloseHandle(h);
      wchar_t *s = b[0] == 0xFEFF ? b + 1 : b; wchar_t *e = wcschr(s, L'\n'); if (e) *e = 0; wcsncpy(label, s, 127); label[127] = 0; } }
  _snwprintf(q, 300, L_(S_UNDO_Q), label); q[299] = 0;
  if (MessageBoxW(g_wnd, q, APP_NAME, MB_YESNO | MB_ICONQUESTION) != IDYES) return;
  int errs = 0; int back = run_undo(&errs, label, 128);
  if (back < 0) { set_status(L"%s", L_(S_NOTHING_UNDO)); return; }
  log_line(L"Undo (%s): %d file(s) put back%s", label, back, errs ? L", some were missing" : L"");
  set_status(L_(S_UNDO_DONE), back);
  if (g_lastDenied > 0) log_line(L"%d file(s) could not go back to Public Desktop \x2014 click 'Run as admin', then Undo again", g_lastDenied);
}
static const wchar_t *rel_path(const wchar_t *folder, const wchar_t *p) {
  size_t l = wcslen(folder);
  if (_wcsnicmp(p, folder, l) == 0 && p[l] == L'\\') return p + l + 1;
  return p;
}
static void fmt_time(FILETIME ft, wchar_t *out, int n) {
  FILETIME lt; SYSTEMTIME st; FileTimeToLocalFileTime(&ft, &lt); FileTimeToSystemTime(&lt, &st);
  _snwprintf(out, n, L"%02d-%02d-%04d %02d:%02d", st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute); out[n-1] = 0;
}
static void dup_scan_ui(void) {
  wchar_t f[PBUF], b[64], s[300];
  if (!get_folder(f)) return;
  set_status(L"%s", L_(S_SCANNING)); UpdateWindow(g_status);
  HCURSOR old = SetCursor(LoadCursor(NULL, IDC_WAIT));
  dup_free(&g_dup);
  dup_scan(f, chk(g_dupSub), &g_dup);
  SetCursor(old);
  g_filling = 1;
  SendMessageW(g_dupList, WM_SETREDRAW, FALSE, 0);
  ListView_DeleteAllItems(g_dupList);
  for (int i = 0; i < g_dup.files.n; i++) {
    FItem *it = &g_dup.files.v[i];
    _snwprintf(b, 64, L"%d", it->group); int row = lv_add(g_dupList, b);
    lv_set(g_dupList, row, 1, rel_path(f, it->path));
    format_size(it->size, b, 64); lv_set(g_dupList, row, 2, b);
    fmt_time(it->mt, b, 64); lv_set(g_dupList, row, 3, b);
    lv_set(g_dupList, row, 4, it->keep ? L_(S_KEEP) : L_(S_MOVE));
    ListView_SetCheckState(g_dupList, row, !it->keep);
  }
  SendMessageW(g_dupList, WM_SETREDRAW, TRUE, 0);
  g_filling = 0;
  if (g_dup.dupes) { format_size(g_dup.wasted, b, 64); _snwprintf(s, 300, L_(S_DUP_SUM), g_dup.groups, g_dup.dupes, b); }
  else wcscpy(s, L_(S_DUP_NONE));
  s[299] = 0; SetWindowTextW(g_dupSum, s); set_status(L"%s", s);
  log_line(L"%s: %s", L_(S_DUP_TITLE), s);
  save_settings();
}
static void dup_move_ui(void) {
  wchar_t f[PBUF], q[400]; int n = g_dup.files.n, cnt = 0, errs = 0;
  if (!get_folder(f)) return;
  if (!n) { set_status(L"%s", L_(S_NOSEL)); return; }
  int *sel = (int *)calloc(n, sizeof(int));
  for (int i = 0; i < n; i++) { sel[i] = ListView_GetCheckState(g_dupList, i) ? 1 : 0; cnt += sel[i]; }
  if (!cnt) { free(sel); set_status(L"%s", L_(S_NOSEL)); return; }
  _snwprintf(q, 400, L_(S_CONFIRM_DUP), cnt); q[399] = 0;
  if (MessageBoxW(g_wnd, q, APP_NAME, MB_YESNO | MB_ICONQUESTION) != IDYES) { free(sel); return; }
  int moved = dup_move(f, &g_dup, sel, 1, NULL, &errs);
  free(sel);
  wchar_t s[200]; _snwprintf(s, 200, L_(S_DUP_MOVED), moved); s[199] = 0;
  log_line(L"%s", s); set_status(L"%s", s);
  dup_free(&g_dup); ListView_DeleteAllItems(g_dupList); SetWindowTextW(g_dupSum, s);
}
static void old_label_update(void) { Ctl *c = find_ctl(g_aOld); if (c) ctl_text(c); }
static void old_preview_ui(void) {
  wchar_t f[PBUF], b[64], s[300]; ULONGLONG tot = 0;
  if (!get_folder(f)) return;
  flist_free(&g_old);
  int d = old_days();
  old_scan(f, d, &g_old, &tot);
  SendMessageW(g_oldList, WM_SETREDRAW, FALSE, 0);
  ListView_DeleteAllItems(g_oldList);
  for (int i = 0; i < g_old.n; i++) {
    int row = lv_add(g_oldList, rel_path(f, g_old.v[i].path));
    fmt_time(g_old.v[i].mt, b, 64); lv_set(g_oldList, row, 1, b);
    format_size(g_old.v[i].size, b, 64); lv_set(g_oldList, row, 2, b);
  }
  SendMessageW(g_oldList, WM_SETREDRAW, TRUE, 0);
  format_size(tot, b, 64); _snwprintf(s, 300, L_(S_OLD_SUM), g_old.n, d, b); s[299] = 0;
  SetWindowTextW(g_oldSum, s); set_status(L"%s", s);
  save_settings();
}
static void old_move_ui(void) {
  wchar_t f[PBUF], q[300]; int errs = 0;
  if (!get_folder(f)) return;
  if (!g_old.n) { set_status(L"%s", L_(S_NOSEL)); return; }
  if (!confirm_risky(f)) return;
  _snwprintf(q, 300, L_(S_CONFIRM_OLD), g_old.n); q[299] = 0;
  if (MessageBoxW(g_wnd, q, APP_NAME, MB_YESNO | MB_ICONQUESTION) != IDYES) return;
  int moved = old_move(f, &g_old, 1, NULL, &errs);
  wchar_t s[200]; _snwprintf(s, 200, L_(S_OLD_MOVED), moved); s[199] = 0;
  log_line(L"%s", s); set_status(L"%s", s);
  flist_free(&g_old); ListView_DeleteAllItems(g_oldList); SetWindowTextW(g_oldSum, s);
}
static void ren_opts(RenameOpts *o) {
  ZeroMemory(o, sizeof *o);
  get_text(g_ren[0], o->contains, 128); get_text(g_ren[1], o->ext, 64); get_text(g_ren[2], o->prefix, 128);
  get_text(g_ren[3], o->suffix, 128); get_text(g_ren[4], o->find, 128); get_text(g_ren[5], o->repl, 128);
  get_text(g_renBase, o->base, 128);
  wchar_t b[16]; get_text(g_renStart, b, 16); o->start = _wtoi(b);
  get_text(g_renDigits, b, 16); o->digits = _wtoi(b); if (o->digits < 1) o->digits = 3;
  o->numbering = chk(g_renNum);
}
static void ren_preview_ui(void) {
  wchar_t f[PBUF], s[200]; RenameOpts o;
  if (!get_folder(f)) return;
  ren_opts(&o);
  rename_free(&g_plan);
  rename_plan(f, &o, &g_plan);
  SendMessageW(g_renList, WM_SETREDRAW, FALSE, 0);
  ListView_DeleteAllItems(g_renList);
  for (int i = 0; i < g_plan.n; i++) { int row = lv_add(g_renList, g_plan.from[i]); lv_set(g_renList, row, 1, g_plan.to[i]); }
  SendMessageW(g_renList, WM_SETREDRAW, TRUE, 0);
  _snwprintf(s, 200, L_(S_REN_SUM), g_plan.n); s[199] = 0;
  SetWindowTextW(g_renSum, s); set_status(L"%s", s);
  save_settings();
}
static void ren_apply_ui(void) {
  wchar_t f[PBUF], q[300]; int errs = 0;
  if (!get_folder(f)) return;
  if (!g_plan.n) { set_status(L"%s", L_(S_NOSEL)); return; }
  _snwprintf(q, 300, L_(S_CONFIRM_REN), g_plan.n); q[299] = 0;
  if (MessageBoxW(g_wnd, q, APP_NAME, MB_YESNO | MB_ICONQUESTION) != IDYES) return;
  int done = rename_apply(f, &g_plan, 1, NULL, &errs);
  wchar_t s[200]; _snwprintf(s, 200, L_(S_REN_DONE), done); s[199] = 0;
  log_line(L"%s", s); set_status(L"%s", s);
  rename_free(&g_plan); ListView_DeleteAllItems(g_renList); SetWindowTextW(g_renSum, s);
}
static void refresh_rule_list(void) {
  if (!g_ruleList) return;
  int keep = ListView_GetNextItem(g_ruleList, -1, LVNI_SELECTED);
  ListView_DeleteAllItems(g_ruleList);
  for (int i = 0; i < g_nrules; i++) {
    wchar_t b[16]; _snwprintf(b, 16, L"%d", i + 1);
    int row = lv_add(g_ruleList, b);
    lv_set(g_ruleList, row, 1, L_(g_rules[i].type == RULE_NAME ? S_R_NAME : S_R_EXT));
    lv_set(g_ruleList, row, 2, g_rules[i].pat);
    lv_set(g_ruleList, row, 3, g_rules[i].folder);
  }
  if (keep >= 0 && keep < g_nrules) ListView_SetItemState(g_ruleList, keep, LVIS_SELECTED, LVIS_SELECTED);
}
static void rule_edit(int mode) {   /* 0 add, 1 update, 2 delete */
  int sel = ListView_GetNextItem(g_ruleList, -1, LVNI_SELECTED);
  if (mode == 2) {
    if (sel < 0 || sel >= g_nrules) { set_status(L"%s", L_(S_NOSEL)); return; }
    for (int i = sel; i < g_nrules - 1; i++) g_rules[i] = g_rules[i + 1];
    g_nrules--;
  } else {
    Rule r; r.type = (int)SendMessageW(g_ruleType, CB_GETCURSEL, 0, 0) == 1 ? RULE_NAME : RULE_EXT;
    get_text(g_rulePat, r.pat, 256); get_text(g_ruleFolder, r.folder, 256);
    if (!rule_valid(r.type, r.pat, r.folder)) { set_status(L"%s", L_(S_RULE_BAD)); MessageBeep(MB_ICONWARNING); return; }
    if (mode == 1 && sel >= 0 && sel < g_nrules) g_rules[sel] = r;
    else { if (g_nrules >= MAX_RULES) { set_status(L"%s", L_(S_RULE_FULL)); return; } g_rules[g_nrules++] = r; }
    SetWindowTextW(g_rulePat, L""); SetWindowTextW(g_ruleFolder, L"");
  }
  rules_save(); refresh_rule_list();
  set_status(L_(S_RULE_SAVED), g_nrules);
  log_line(L_(S_RULE_SAVED), g_nrules);
}
static void stats_ui(void) {
  wchar_t f[PBUF], b[64], s[200];
  if (!get_folder(f)) return;
  HCURSOR old = SetCursor(LoadCursor(NULL, IDC_WAIT));
  g_statsFiles = stats_scan(f, chk(g_stoSub), g_stats, &g_statsTotal);
  SetCursor(old);
  format_size(g_statsTotal, b, 64); _snwprintf(s, 200, L_(S_STO_SUM), g_statsFiles, b); s[199] = 0;
  SetWindowTextW(g_stoSum, s); set_status(L"%s", s);
  InvalidateRect(g_chart, NULL, TRUE);
  save_settings();
}
static int CALLBACK browse_cb(HWND h, UINT msg, LPARAM lp, LPARAM data) {
  (void)lp; if (msg == BFFM_INITIALIZED && data) SendMessageW(h, BFFM_SETSELECTIONW, TRUE, data);
  return 0;
}
static void browse(void) {
  wchar_t cur[PBUF], out[PBUF];
  GetWindowTextW(g_path, cur, PBUF);
  BROWSEINFOW bi; ZeroMemory(&bi, sizeof bi);
  bi.hwndOwner = g_wnd; bi.lpszTitle = L_(S_BROWSE_TITLE);
  bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE; bi.lpfn = browse_cb; bi.lParam = (LPARAM)cur;
  PIDLIST_ABSOLUTE pidl = SHBrowseForFolderW(&bi);
  if (pidl) {
    if (SHGetPathFromIDListW(pidl, out)) { SetWindowTextW(g_path, out); save_settings(); log_line(L"Folder: %s", out); set_status(L"%s", L_(S_FOLDER_SEL)); }
    CoTaskMemFree(pidl);
  }
}
static void run_as_admin(void) {
  wchar_t f[PBUF], args[PBUF + 32];
  GetWindowTextW(g_path, f, PBUF);
  save_settings();
  size_t l = wcslen(f);
  if (l && f[l-1] == L'\\') _snwprintf(args, PBUF + 32, L"--folder \"%s\\\"", f);
  else _snwprintf(args, PBUF + 32, L"--folder \"%s\"", f);
  args[PBUF+31] = 0;
  if (g_mutex) { ReleaseMutex(g_mutex); CloseHandle(g_mutex); g_mutex = NULL; }
  SHELLEXECUTEINFOW sei; ZeroMemory(&sei, sizeof sei);
  sei.cbSize = sizeof sei; sei.hwnd = g_wnd; sei.lpVerb = L"runas"; sei.lpFile = g_self;
  sei.lpParameters = args; sei.nShow = SW_SHOWNORMAL; sei.fMask = SEE_MASK_NOASYNC;
  if (ShellExecuteExW(&sei)) { DestroyWindow(g_wnd); return; }
  DWORD e = GetLastError();
  g_mutex = CreateMutexW(NULL, TRUE, MUTEX_NAME);
  if (e == ERROR_CANCELLED) { log_line(L"Admin permission was not given (UAC: No)."); set_status(L"Run as admin cancelled."); }
  else { log_line(L"Could not restart as admin (error %lu).", e); set_status(L"Could not restart as admin."); }
}

/* ------------------------------------------------------------------ */
/* Build the window                                                    */
/* ------------------------------------------------------------------ */
static HWND lbl(int page, int sid, int x, int y, int w) { return mkc(page, K_STATIC, sid, NULL, x, y, w, 22, 0); }
static void build_ui(void) {
  static const int tabs[NPAGES] = { S_TAB_SORT, S_TAB_DUP, S_TAB_OLD, S_TAB_REN, S_TAB_RULES, S_TAB_STO, S_TAB_SET };
  wchar_t f[PBUF], b[256];
  g_page = ini_int(L"LastPage", 0); if (g_startPage >= 0) g_page = g_startPage;
  if (g_page < 0 || g_page >= NPAGES) g_page = 0;

  lbl(PG_ALL, S_FOLDER, 20, 77, 400);
  g_path = mkc(PG_ALL, K_EDIT, 0, NULL, 20, 99, 520, 28, IDC_PATH);
  mkc(PG_ALL, K_BTN, S_BROWSE, NULL, 548, 98, 112, 30, IDC_BROWSE);
  int tw = (CW - 24) / NPAGES;
  for (int i = 0; i < NPAGES; i++) mkc(PG_ALL, K_TAB, tabs[i], NULL, 12 + i * tw, 138, tw, 34, IDC_TAB0 + i);

  /* ---- Sort ---- */
  mkc(PG_SORT, K_SECTION, S_ACTIONS, NULL, 20, 180, 400, 24, 0);
  mkc(PG_SORT, K_PRIMARY, S_DATEFOLDER, L"\U0001F4C1", 20, 208, 150, 38, IDC_DATE);
  mkc(PG_SORT, K_PRIMARY, S_SORTEXT, L"\U0001F524", 178, 208, 160, 38, IDC_EXT);
  mkc(PG_SORT, K_PRIMARY, S_SORTTYPE, L"\U0001F5C2", 346, 208, 150, 38, IDC_TYPE);
  mkc(PG_SORT, K_PRIMARY, S_SORTDATE, L"\U0001F4C5", 504, 208, 156, 38, IDC_SDATE);
  mkc(PG_SORT, K_BTN, S_UNDO, L"\x21A9", 20, 254, 150, 30, IDC_UNDO);
  mkc(PG_SORT, K_BTN, S_OPENFOLDER, L"\U0001F4C2", 178, 254, 160, 30, IDC_OPEN);
  if (!g_elevated) g_adminBtn = mkc(PG_SORT, K_BTN, S_RUNADMIN, L"\U0001F6E1", 346, 254, 150, 30, IDC_ADMIN);
  mkc(PG_SORT, K_SECTION, S_AUTOMODE, NULL, 20, 296, 400, 24, 0);
  g_auto = mkc(PG_SORT, K_CHECK, S_AUTOMODE, NULL, 20, 324, 150, 24, IDC_AUTO);
  lbl(PG_SORT, S_EVERY, 176, 326, 40);
  g_interval = mkc(PG_SORT, K_COMBO, 0, NULL, 218, 322, 70, 0, IDC_INTERVAL);
  for (int i = 0; i < 4; i++) { _snwprintf(b, 8, L"%d", g_intervals[i]); SendMessageW(g_interval, CB_ADDSTRING, 0, (LPARAM)b); }
  lbl(PG_SORT, S_MINUTES, 296, 326, 160);
  mkc(PG_SORT, K_HINT, S_AUTORUNS, NULL, 20, 356, 400, 20, 0);
  g_aRules = mkc(PG_SORT, K_CHECK, S_A_RULES, NULL, 20, 378, 204, 24, IDC_A_RULES);
  g_aSort  = mkc(PG_SORT, K_CHECK, S_A_SORT, NULL, 232, 378, 96, 24, IDC_A_SORT);
  g_rtype  = mkc(PG_SORT, K_RADIO, S_TYPE, NULL, 332, 378, 100, 24, IDC_R_TYPE);
  g_rext   = mkc(PG_SORT, K_RADIO, S_EXTENSION, NULL, 436, 378, 150, 24, IDC_R_EXT);
  g_aDate  = mkc(PG_SORT, K_CHECK, S_A_DATE, NULL, 20, 406, 204, 24, IDC_A_DATE);
  g_aOld   = mkc(PG_SORT, K_CHECK, S_A_OLD, NULL, 232, 406, 200, 24, IDC_A_OLD);
  g_aDup   = mkc(PG_SORT, K_CHECK, S_A_DUP, NULL, 436, 406, 200, 24, IDC_A_DUP);
  g_tray   = mkc(PG_SORT, K_CHECK, S_TRAY, NULL, 20, 444, 204, 24, IDC_TRAYCHK);
  g_startup = mkc(PG_SORT, K_CHECK, S_STARTUP, NULL, 232, 444, 260, 24, IDC_STARTUP);

  /* ---- Duplicates ---- */
  mkc(PG_DUP, K_SECTION, S_DUP_TITLE, NULL, 20, 180, 400, 24, 0);
  mkc(PG_DUP, K_HINT, S_DUP_HINT, NULL, 20, 205, 640, 20, 0);
  g_dupSub = mkc(PG_DUP, K_CHECK, S_SUBFOLDERS, NULL, 20, 230, 200, 24, IDC_DUP_SUB);
  g_dupSum = mkc(PG_DUP, K_STATIC, 0, NULL, 236, 232, 424, 22, IDC_DUP_SUM);
  mkc(PG_DUP, K_PRIMARY, S_SCAN, L"\U0001F50D", 20, 260, 130, 32, IDC_DUP_SCAN);
  mkc(PG_DUP, K_BTN, S_MOVEDUP, NULL, 156, 260, 200, 32, IDC_DUP_MOVE);
  mkc(PG_DUP, K_BTN, S_UNDO_SHORT, L"\x21A9", 362, 260, 100, 32, IDC_DUP_UNDO);
  g_dupList = mklist(PG_DUP, 12, 300, 656, 186, IDC_DUP_LIST, 1);
  { int s[5] = { S_COL_GROUP, S_COL_FILE, S_COL_SIZE, S_COL_MOD, S_COL_ACTION }, w[5] = { 56, 290, 80, 124, 80 }; lv_set_cols(g_dupList, 5, s, w); }

  /* ---- Old files ---- */
  mkc(PG_OLD, K_SECTION, S_OLD_TITLE, NULL, 20, 180, 400, 24, 0);
  lbl(PG_OLD, S_OLD_NOTMOD, 20, 214, 200);
  g_oldDays = mkc(PG_OLD, K_COMBO, 0, NULL, 224, 210, 96, 0, IDC_OLD_DAYS);
  for (int i = 0; i < 5; i++) { _snwprintf(b, 8, L"%d", g_dayOpts[i]); SendMessageW(g_oldDays, CB_ADDSTRING, 0, (LPARAM)b); }
  SendMessageW(g_oldDays, CB_ADDSTRING, 0, (LPARAM)L_(S_CUSTOM));
  g_oldCustom = mkc(PG_OLD, K_EDIT, 0, NULL, 326, 210, 64, 27, IDC_OLD_CUSTOM);
  SetWindowLongPtrW(g_oldCustom, GWL_STYLE, GetWindowLongPtrW(g_oldCustom, GWL_STYLE) | ES_NUMBER);
  lbl(PG_OLD, S_DAYS, 398, 214, 100);
  g_oldSum = mkc(PG_OLD, K_HINT, S_OLD_HINT, NULL, 20, 242, 640, 20, IDC_OLD_SUM);
  mkc(PG_OLD, K_PRIMARY, S_PREVIEW, L"\U0001F441", 20, 266, 130, 32, IDC_OLD_PREV);
  mkc(PG_OLD, K_BTN, S_MOVEOLD, NULL, 156, 266, 200, 32, IDC_OLD_MOVE);
  mkc(PG_OLD, K_BTN, S_UNDO_SHORT, L"\x21A9", 362, 266, 100, 32, IDC_OLD_UNDO);
  g_oldList = mklist(PG_OLD, 12, 306, 656, 180, IDC_OLD_LIST, 0);
  { int s[3] = { S_COL_FILE, S_COL_MOD, S_COL_SIZE }, w[3] = { 370, 150, 110 }; lv_set_cols(g_oldList, 3, s, w); }

  /* ---- Rename ---- */
  mkc(PG_REN, K_SECTION, S_REN_TITLE, NULL, 20, 180, 400, 24, 0);
  { static const int L1[3] = { S_REN_CONTAINS, S_REN_PREFIX, S_REN_FIND }, L2[3] = { S_REN_EXT, S_REN_SUFFIX, S_REN_REPLACE };
    for (int i = 0; i < 3; i++) {
      int y = 208 + i * 32;
      lbl(PG_REN, L1[i], 20, y + 3, 100);  g_ren[i * 2] = mkc(PG_REN, K_EDIT, 0, NULL, 122, y, 200, 27, IDC_REN_CONTAINS + i * 2);
      lbl(PG_REN, L2[i], 336, y + 3, 100); g_ren[i * 2 + 1] = mkc(PG_REN, K_EDIT, 0, NULL, 438, y, 222, 27, IDC_REN_CONTAINS + i * 2 + 1);
    } }
  /* g_ren order: 0 contains, 1 ext, 2 prefix, 3 suffix, 4 find, 5 replace */
  g_renNum = mkc(PG_REN, K_CHECK, S_REN_NUMBER, NULL, 20, 305, 100, 24, IDC_REN_NUM);
  g_renBase = mkc(PG_REN, K_EDIT, 0, NULL, 122, 304, 200, 27, IDC_REN_BASE);
  lbl(PG_REN, S_REN_START, 336, 307, 56);  g_renStart = mkc(PG_REN, K_EDIT, 0, NULL, 394, 304, 56, 27, IDC_REN_START);
  lbl(PG_REN, S_REN_DIGITS, 462, 307, 56); g_renDigits = mkc(PG_REN, K_EDIT, 0, NULL, 520, 304, 44, 27, IDC_REN_DIGITS);
  mkc(PG_REN, K_PRIMARY, S_PREVIEW, L"\U0001F441", 20, 340, 130, 32, IDC_REN_PREV);
  mkc(PG_REN, K_BTN, S_APPLY, L"\x270F", 156, 340, 150, 32, IDC_REN_APPLY);
  mkc(PG_REN, K_BTN, S_UNDO_SHORT, L"\x21A9", 312, 340, 100, 32, IDC_REN_UNDO);
  g_renSum = mkc(PG_REN, K_HINT, S_REN_HINT, NULL, 420, 347, 250, 22, IDC_REN_SUM);
  g_renList = mklist(PG_REN, 12, 380, 656, 106, IDC_REN_LIST, 0);
  { int s[2] = { S_COL_OLDNAME, S_COL_NEWNAME }, w[2] = { 310, 320 }; lv_set_cols(g_renList, 2, s, w); }

  /* ---- Rules ---- */
  mkc(PG_RULES, K_SECTION, S_RULES_TITLE, NULL, 20, 180, 260, 24, 0);
  g_ruleUse = mkc(PG_RULES, K_CHECK, S_USE_RULES, NULL, 300, 180, 360, 24, IDC_RULE_USE);
  g_ruleList = mklist(PG_RULES, 12, 210, 656, 142, IDC_RULE_LIST, 0);
  { int s[4] = { S_COL_NUM, S_COL_IF, S_COL_VALUE, S_COL_FOLDER }, w[4] = { 40, 160, 220, 210 }; lv_set_cols(g_ruleList, 4, s, w); }
  mkc(PG_RULES, K_HINT, S_RULE_LBL, NULL, 20, 360, 170, 20, 0);
  mkc(PG_RULES, K_HINT, S_VALUE_LBL, NULL, 196, 360, 216, 20, 0);
  mkc(PG_RULES, K_HINT, S_FOLDER_LBL, NULL, 420, 360, 240, 20, 0);
  g_ruleType = mkc(PG_RULES, K_COMBO, 0, NULL, 20, 381, 170, 0, IDC_RULE_TYPE);
  { int s[2] = { S_R_EXT, S_R_NAME }; combo_fill(g_ruleType, 2, s); SendMessageW(g_ruleType, CB_SETCURSEL, 0, 0); }
  g_rulePat = mkc(PG_RULES, K_EDIT, 0, NULL, 196, 381, 216, 27, IDC_RULE_PAT);
  g_ruleFolder = mkc(PG_RULES, K_EDIT, 0, NULL, 420, 381, 240, 27, IDC_RULE_FOLDER);
  mkc(PG_RULES, K_PRIMARY, S_ADD, L"\x2795", 20, 418, 150, 32, IDC_RULE_ADD);
  mkc(PG_RULES, K_BTN, S_UPDATE, NULL, 176, 418, 110, 32, IDC_RULE_UPD);
  mkc(PG_RULES, K_BTN, S_DELETE, NULL, 292, 418, 110, 32, IDC_RULE_DEL);
  mkc(PG_RULES, K_HINT, S_RULES_HINT, NULL, 20, 458, 640, 20, 0);

  /* ---- Storage ---- */
  mkc(PG_STO, K_SECTION, S_STO_TITLE, NULL, 20, 180, 230, 24, 0);
  g_stoSub = mkc(PG_STO, K_CHECK, S_SUBFOLDERS, NULL, 260, 181, 220, 24, IDC_STO_SUB);
  mkc(PG_STO, K_PRIMARY, S_ANALYZE, L"\U0001F4CA", 520, 177, 140, 32, IDC_STO_GO);
  g_stoSum = mkc(PG_STO, K_STATIC, 0, NULL, 20, 214, 640, 22, IDC_STO_SUM);
  g_chart = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | SS_OWNERDRAW | (g_page == PG_STO ? WS_VISIBLE : 0), S(12), S(240), S(656), S(246), g_wnd, (HMENU)IDC_CHART, g_inst, NULL);
  if (g_nctl < 200) { Ctl *c = &g_ctl[g_nctl++]; c->h = g_chart; c->page = PG_STO; c->sid = 0; c->icon = NULL; c->kind = K_OTHER; c->fx = c->fy = 0; }

  /* ---- Settings ---- */
  mkc(PG_SET, K_SECTION, S_APPEARANCE, NULL, 20, 180, 400, 24, 0);
  lbl(PG_SET, S_THEME, 20, 215, 145);
  g_theme = mkc(PG_SET, K_COMBO, 0, NULL, 170, 211, 260, 0, IDC_THEME);
  { int s[3] = { S_LIGHT, S_DARK, S_SYSTEM }; combo_fill(g_theme, 3, s); }
  lbl(PG_SET, S_COLOR, 20, 251, 145);
  g_color = mkc(PG_SET, K_COMBO, 0, NULL, 170, 247, 260, 0, IDC_COLOR);
  { int s[NACCENTS]; for (int i = 0; i < NACCENTS; i++) s[i] = ACCENTS[i].sid; combo_fill(g_color, NACCENTS, s); }
  lbl(PG_SET, S_LANGUAGE, 20, 287, 145);
  g_langCombo = mkc(PG_SET, K_COMBO, 0, NULL, 170, 283, 260, 0, IDC_LANG);
  SendMessageW(g_langCombo, CB_ADDSTRING, 0, (LPARAM)L"English");
  SendMessageW(g_langCombo, CB_ADDSTRING, 0, (LPARAM)L"ગુજરાતી (Gujarati)");
  mkc(PG_SET, K_SECTION, S_ABOUT, NULL, 20, 334, 400, 24, 0);
  lbl(PG_SET, S_ABOUT1, 20, 362, 640);
  mkc(PG_SET, K_HINT, S_ABOUT2, NULL, 20, 386, 640, 20, 0);

  /* ---- shared: activity + status ---- */
  lbl(PG_ALL, S_ACTIVITY, 20, 494, 300);
  g_log = CreateWindowExW(0, L"LISTBOX", L"", WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOINTEGRALHEIGHT | LBS_NOSEL,
                          S(17), S(520), S(646), S(98), g_wnd, (HMENU)IDC_LOG, g_inst, NULL);
  SendMessageW(g_log, WM_SETFONT, (WPARAM)g_font, TRUE);
  if (g_nctl < 200) { Ctl *c = &g_ctl[g_nctl++]; c->h = g_log; c->page = PG_ALL; c->sid = 0; c->icon = NULL; c->kind = K_OTHER; c->fx = 5; c->fy = 4; }
  g_status = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_LEFTNOWORDWRAP | SS_ENDELLIPSIS | SS_NOPREFIX,
                             S(14), S(630), S(652), S(24), g_wnd, (HMENU)IDC_STATUS, g_inst, NULL);
  SendMessageW(g_status, WM_SETFONT, (WPARAM)g_font, TRUE);
  SetWindowTextW(g_status, L_(S_READY));
  for (int i = 0; i < g_nctl; i++) if (g_ctl[i].kind == K_LIST) lv_hook(g_ctl[i].h);
  for (int i = 0; i < g_nctl; i++) if (g_ctl[i].kind == K_COMBO) { WNDPROC o = (WNDPROC)SetWindowLongPtrW(g_ctl[i].h, GWLP_WNDPROC, (LONG_PTR)cb_proc); if (!g_cbOrig) g_cbOrig = o; }
  for (int i = 0; i < g_nctl; i++) if (g_ctl[i].kind == K_COMBO) { SendMessageW(g_ctl[i].h, CB_SETITEMHEIGHT, (WPARAM)-1, S(22)); SendMessageW(g_ctl[i].h, CB_SETITEMHEIGHT, 0, S(22)); }

  /* ---- load settings ---- */
  wchar_t def[PBUF]; PWSTR dl = NULL;
  if (SUCCEEDED(SHGetKnownFolderPath(&FOLDERID_Downloads, 0, NULL, &dl)) && dl) { wcsncpy(def, dl, PBUF - 1); def[PBUF-1] = 0; CoTaskMemFree(dl); }
  else if (GetEnvironmentVariableW(L"USERPROFILE", def, PBUF)) wcscat(def, L"\\Downloads"); else wcscpy(def, L"C:\\");
  ini_str(L"Folder", def, f, PBUF);
  if (g_cliFolder[0] && folder_ok(g_cliFolder)) wcscpy(f, g_cliFolder);
  if (!folder_ok(f)) wcscpy(f, def);
  SetWindowTextW(g_path, f);
  int iv = ini_int(L"IntervalMinutes", 5), sel = 1;
  for (int i = 0; i < 4; i++) if (g_intervals[i] == iv) sel = i;
  SendMessageW(g_interval, CB_SETCURSEL, sel, 0);
  int mode = ini_int(L"AutoSort", 1);      /* 0 = extension, 1 = type (v1.x key) */
  chk_set(mode == 0 ? g_rext : g_rtype, 1);
  chk_set(g_tray, ini_int(L"MinimizeToTray", 1));
  chk_set(g_auto, ini_int(L"AutoMode", 0));
  chk_set(g_aRules, ini_int(L"AutoRules", 1)); chk_set(g_aSort, ini_int(L"AutoDoSort", 1));
  chk_set(g_aDate, ini_int(L"AutoDate", 0)); chk_set(g_aOld, ini_int(L"AutoOld", 0)); chk_set(g_aDup, ini_int(L"AutoDupes", 0));
  int od = ini_int(L"OldDays", 90), osel = 5;
  for (int i = 0; i < 5; i++) if (g_dayOpts[i] == od) osel = i;
  SendMessageW(g_oldDays, CB_SETCURSEL, osel, 0);
  _snwprintf(b, 16, L"%d", od); SetWindowTextW(g_oldCustom, b); EnableWindow(g_oldCustom, osel == 5);
  old_label_update();
  chk_set(g_dupSub, ini_int(L"DupSub", 1)); chk_set(g_stoSub, ini_int(L"StoSub", 1)); chk_set(g_ruleUse, ini_int(L"UseRules", 1));
  static const wchar_t *rk[6] = { L"RenContains", L"RenExt", L"RenPrefix", L"RenSuffix", L"RenFind", L"RenReplace" };
  for (int i = 0; i < 6; i++) { ini_str(rk[i], L"", b, 200); SetWindowTextW(g_ren[i], b); }
  ini_str(L"RenBase", L"Photo_", b, 200); SetWindowTextW(g_renBase, b);
  _snwprintf(b, 16, L"%d", ini_int(L"RenStart", 1)); SetWindowTextW(g_renStart, b);
  _snwprintf(b, 16, L"%d", ini_int(L"RenDigits", 3)); SetWindowTextW(g_renDigits, b);
  chk_set(g_renNum, ini_int(L"RenNumbering", 0));
  SendMessageW(g_theme, CB_SETCURSEL, g_themeSel, 0);
  SendMessageW(g_color, CB_SETCURSEL, g_accentSel, 0);
  SendMessageW(g_langCombo, CB_SETCURSEL, g_lang, 0);
  refresh_rule_list();
  int st = startup_is_on();
  chk_set(g_startup, st);
  if (st) startup_set(1);   /* refresh path in case the exe was moved */
  log_line(L"%s", L_(S_WELCOME));
  if (g_elevated) log_line(L"Running as Admin: Public Desktop shortcuts can be sorted too.");
  apply_timer();
}

/* ------------------------------------------------------------------ */
/* Window procedure                                                    */
/* ------------------------------------------------------------------ */
static void startup_toggle(void) {
  int on = chk(g_startup);
  if (startup_set(on)) { log_line(on ? L"Will start with Windows" : L"Won't start with Windows"); }
  else { chk_set(g_startup, !on); set_status(L"Could not change the startup setting."); }
}
static void on_check(HWND c, int id) {
  Ctl *x = find_ctl(c);
  if (x && x->kind == K_RADIO) { chk_set(g_rtype, id == IDC_R_TYPE); chk_set(g_rext, id == IDC_R_EXT); }
  else chk_set(c, !chk(c));
  switch (id) {
  case IDC_AUTO:
    save_settings(); apply_timer();
    if (chk(g_auto)) { log_line(L"%s: ON", L_(S_AUTOMODE)); do_auto(); }
    else { log_line(L"%s", L_(S_AUTO_OFF)); set_status(L"%s", L_(S_AUTO_OFF)); }
    break;
  case IDC_STARTUP: startup_toggle(); break;
  default: save_settings(); break;
  }
}
static LRESULT CALLBACK wndproc(HWND h, UINT m, WPARAM w, LPARAM l) {
  switch (m) {
  case WM_CREATE: g_wnd = h; build_ui(); apply_theme(); return 0;
  case WM_ERASEBKGND: { RECT r; GetClientRect(h, &r); FillRect((HDC)w, &r, g_brBg); return 1; }
  case WM_PAINT: { PAINTSTRUCT ps; HDC dc = BeginPaint(h, &ps); paint_main(dc); EndPaint(h, &ps); return 0; }
  case WM_MEASUREITEM: {
    MEASUREITEMSTRUCT *mi = (MEASUREITEMSTRUCT *)l;
    if (mi->CtlType == ODT_COMBOBOX) { mi->itemHeight = S(22); return TRUE; }
    break; }
  case WM_DRAWITEM: {
    const DRAWITEMSTRUCT *di = (const DRAWITEMSTRUCT *)l;
    if (di->CtlType == ODT_COMBOBOX) draw_combo_item(di);
    else if (di->CtlType == ODT_STATIC) draw_chart(di);
    else draw_button(di);
    return TRUE; }
  case WM_CTLCOLORSTATIC: {
    HDC dc = (HDC)w; HWND c = (HWND)l; Ctl *x = find_ctl(c);
    SetBkColor(dc, P.bg);
    COLORREF col = P.text;
    if (c == g_status) col = P.sub;
    else if (x && x->kind == K_SECTION) col = accent_text();
    else if (x && x->kind == K_HINT) col = P.sub;
    else if (x && x->kind == K_EDIT) { SetTextColor(dc, P.sub); return (LRESULT)g_brBg; }  /* disabled edit */
    SetTextColor(dc, col);
    return (LRESULT)g_brBg; }
  case WM_CTLCOLOREDIT: case WM_CTLCOLORLISTBOX: {
    HDC dc = (HDC)w; SetBkColor(dc, P.field); SetTextColor(dc, P.fieldText);
    return (LRESULT)g_brField; }
  case WM_CTLCOLORBTN: return (LRESULT)g_brBg;
  case WM_SETTINGCHANGE:
    if (l && g_themeSel == 2 && lstrcmpiW((LPCWSTR)l, L"ImmersiveColorSet") == 0) apply_theme();
    break;
  case WM_NOTIFY: {
    NMHDR *nh = (NMHDR *)l;
    if (nh->idFrom == IDC_DUP_LIST && nh->code == LVN_ITEMCHANGED && !g_filling) {
      NMLISTVIEW *nl = (NMLISTVIEW *)l;
      if ((nl->uChanged & LVIF_STATE) && ((nl->uNewState ^ nl->uOldState) & LVIS_STATEIMAGEMASK))
        lv_set(g_dupList, nl->iItem, 4, ListView_GetCheckState(g_dupList, nl->iItem) ? L_(S_MOVE) : L_(S_KEEP));
    }
    if (nh->idFrom == IDC_RULE_LIST && nh->code == LVN_ITEMCHANGED) {
      NMLISTVIEW *nl = (NMLISTVIEW *)l;
      if ((nl->uNewState & LVIS_SELECTED) && !(nl->uOldState & LVIS_SELECTED) && nl->iItem >= 0 && nl->iItem < g_nrules) {
        Rule *r = &g_rules[nl->iItem];
        SendMessageW(g_ruleType, CB_SETCURSEL, r->type == RULE_NAME ? 1 : 0, 0);
        SetWindowTextW(g_rulePat, r->pat); SetWindowTextW(g_ruleFolder, r->folder);
      }
    }
    break; }
  case WM_COMMAND: {
    int id = LOWORD(w), code = HIWORD(w);
    if (id >= IDC_TAB0 && id < IDC_TAB0 + NPAGES) { show_page(id - IDC_TAB0); return 0; }
    Ctl *x = find_ctl((HWND)l);
    if (x && x->kind == K_EDIT && (code == EN_SETFOCUS || code == EN_KILLFOCUS)) invalidate_frame((HWND)l);
    if (x && (x->kind == K_CHECK || x->kind == K_RADIO)) { if (code == BN_CLICKED || code == BN_DOUBLECLICKED) on_check((HWND)l, id); return 0; }
    if (x && (x->kind == K_BTN || x->kind == K_PRIMARY) && code != BN_CLICKED) return 0;
    switch (id) {
    case IDC_BROWSE: browse(); break;
    case IDC_DATE: {
      wchar_t f[PBUF], name[32]; int already = 0;
      if (!get_folder(f)) break;
      if (make_date_folder(f, name, 32, &already)) {
        if (already) { log_line(L_(S_DATE_EXISTS), name); set_status(L_(S_DATE_EXISTS), name); }
        else { log_line(L_(S_DATE_CREATED), name); set_status(L_(S_DATE_CREATED), name); }
      } else { log_line(L"Could not create folder %s", name); set_status(L"Could not create the folder."); }
      break; }
    case IDC_EXT: do_sort(SF_EXT); break;
    case IDC_TYPE: do_sort(SF_TYPE); break;
    case IDC_SDATE: do_sort(SF_DATE); break;
    case IDC_UNDO: case IDC_DUP_UNDO: case IDC_OLD_UNDO: case IDC_REN_UNDO: undo_last(); break;
    case IDC_ADMIN: run_as_admin(); break;
    case IDC_OPEN: { wchar_t f[PBUF]; if (get_folder(f)) ShellExecuteW(h, L"open", f, NULL, NULL, SW_SHOWNORMAL); break; }
    case IDC_INTERVAL: if (code == CBN_SELCHANGE) { save_settings(); apply_timer(); } break;
    case IDC_DUP_SCAN: dup_scan_ui(); break;
    case IDC_DUP_MOVE: dup_move_ui(); break;
    case IDC_OLD_DAYS: if (code == CBN_SELCHANGE) {
        int s = (int)SendMessageW(g_oldDays, CB_GETCURSEL, 0, 0);
        EnableWindow(g_oldCustom, s == 5); if (s == 5) SetFocus(g_oldCustom);
        old_label_update(); save_settings(); }
      break;
    case IDC_OLD_CUSTOM: if (code == EN_CHANGE) old_label_update(); else if (code == EN_KILLFOCUS) save_settings(); break;
    case IDC_OLD_PREV: old_preview_ui(); break;
    case IDC_OLD_MOVE: old_move_ui(); break;
    case IDC_REN_PREV: ren_preview_ui(); break;
    case IDC_REN_APPLY: ren_apply_ui(); break;
    case IDC_RULE_ADD: rule_edit(0); break;
    case IDC_RULE_UPD: rule_edit(1); break;
    case IDC_RULE_DEL: rule_edit(2); break;
    case IDC_STO_GO: stats_ui(); break;
    case IDC_THEME: case IDC_COLOR: if (code == CBN_SELCHANGE) {
        int t = (int)SendMessageW(g_theme, CB_GETCURSEL, 0, 0), a = (int)SendMessageW(g_color, CB_GETCURSEL, 0, 0);
        if (t >= 0) g_themeSel = t;
        if (a >= 0) g_accentSel = a;
        apply_theme(); save_settings();
        wchar_t tn[64]; GetWindowTextW(g_theme, tn, 64);
        set_status(L_(S_THEME_SET), tn, L_(ACCENTS[g_accentSel].sid));
      }
      break;
    case IDC_LANG: if (code == CBN_SELCHANGE) {
        int s = (int)SendMessageW(g_langCombo, CB_GETCURSEL, 0, 0);
        if (s >= 0 && s != g_lang) { g_lang = s; apply_language(); save_settings(); set_status(L"%s", L_(S_READY)); log_line(g_lang ? L"ભાષા: ગુજરાતી" : L"Language: English"); }
      }
      break;
    case IDC_PATH: if (code == EN_KILLFOCUS) save_settings(); break;
    case IDM_OPEN: restore_from_tray(); break;
    case IDM_EXIT: DestroyWindow(h); break;
    }
    return 0; }
  case WM_STARTRUN:
    switch (g_page) {
    case PG_DUP: dup_scan_ui(); break;
    case PG_OLD: old_preview_ui(); break;
    case PG_REN: ren_preview_ui(); break;
    case PG_STO: stats_ui(); break;
    case PG_SORT: if (g_startRun == 2) do_auto(); break;
    }
    return 0;
  case WM_TIMER:
    if (w == TIMER_AUTO) do_auto();
    return 0;
  case WM_SIZE:
    if (w == SIZE_MINIMIZED && chk(g_auto) && chk(g_tray)) go_to_tray();
    return 0;
  case WM_TRAY:
    if (l == WM_LBUTTONDBLCLK || l == WM_LBUTTONUP) restore_from_tray();
    else if (l == WM_RBUTTONUP || l == WM_CONTEXTMENU) {
      POINT p; GetCursorPos(&p);
      HMENU mnu = CreatePopupMenu();
      AppendMenuW(mnu, MF_STRING, IDM_OPEN, L_(S_TRAY_OPEN));
      AppendMenuW(mnu, MF_SEPARATOR, 0, NULL);
      AppendMenuW(mnu, MF_STRING, IDM_EXIT, L_(S_TRAY_EXIT));
      SetMenuDefaultItem(mnu, IDM_OPEN, FALSE);
      SetForegroundWindow(h);
      TrackPopupMenu(mnu, TPM_RIGHTBUTTON, p.x, p.y, 0, h, NULL);
      PostMessageW(h, WM_NULL, 0, 0);
      DestroyMenu(mnu);
    }
    return 0;
  case WM_WAKEUP: restore_from_tray(); return 0;
  case WM_DESTROY:
    save_settings(); KillTimer(h, TIMER_AUTO); if (g_inTray) tray_icon(0, NULL);
    PostQuitMessage(0); return 0;
  }
  return DefWindowProcW(h, m, w, l);
}

/* ------------------------------------------------------------------ */
/* Command line (no window). Results are printed (UTF-8) and the exit  */
/* code is the number of files changed (1000 = error).                 */
/*   --sort ext|type|date|rules "folder" [--rules] [--date]            */
/*   --date "folder"      --undo                                        */
/*   --dupes "folder" [--sub] [--move]                                  */
/*   --old "folder" DAYS [--move]                                       */
/*   --rename "folder" [--contains X] [--ext E] [--prefix P] [--suffix S]*/
/*            [--find F --replace R] [--number BASE START DIGITS] [--apply]*/
/*   --stats "folder" [--sub]                                           */
/*   --rule-add ext|name "pattern" "folder"   --rule-list  --rule-clear */
/*   --auto "folder" [--with rules,type,ext,date,old,dupes] [--days N]  */
/*   --set Key Value      --get Key                                     */
/* ------------------------------------------------------------------ */
static const wchar_t *opt(int argc, wchar_t **argv, const wchar_t *name) {
  for (int i = 1; i < argc - 1; i++) if (_wcsicmp(argv[i], name) == 0) return argv[i + 1];
  return NULL;
}
static int has(int argc, wchar_t **argv, const wchar_t *name) {
  for (int i = 1; i < argc; i++) if (_wcsicmp(argv[i], name) == 0) return 1;
  return 0;
}
static int in_list(const wchar_t *list, const wchar_t *word) {
  wchar_t b[256]; _snwprintf(b, 256, L",%s,", list); b[255] = 0; CharLowerW(b);
  wchar_t k[64]; _snwprintf(k, 64, L",%s,", word); k[63] = 0;
  return wcsstr(b, k) != NULL;
}
static int cli(int argc, wchar_t **argv) {
  const wchar_t *cmd = argv[1];
  g_cliMode = 1;
  rules_load();
  if (argc >= 4 && _wcsicmp(cmd, L"--sort") == 0) {
    int flags = 0, errs = 0, br = 0;
    if (!_wcsicmp(argv[2], L"ext")) flags = SF_EXT;
    else if (!_wcsicmp(argv[2], L"date")) flags = SF_DATE;
    else if (!_wcsicmp(argv[2], L"rules")) flags = SF_RULES;
    else flags = SF_TYPE;
    if (has(argc, argv, L"--rules")) flags |= SF_RULES;
    if (has(argc, argv, L"--date")) flags |= SF_DATE;
    int r = run_sort_flags(flags, argv[3], 1, &errs, NULL, &br);
    outf(L"sorted=%d byRules=%d errors=%d", r, br, errs);
    return r < 0 ? 1000 : r;
  }
  if (argc >= 3 && _wcsicmp(cmd, L"--date") == 0) { wchar_t n[32]; int a; int ok = make_date_folder(argv[2], n, 32, &a); outf(L"date_folder=%s existed=%d", n, a); return ok ? 0 : 1000; }
  if (argc >= 2 && _wcsicmp(cmd, L"--undo") == 0) { int e = 0; wchar_t lab[128] = L""; int r = run_undo(&e, lab, 128); outf(L"undo=%d label=%s errors=%d", r, lab, e); return r < 0 ? 1000 : r; }
  if (argc >= 3 && _wcsicmp(cmd, L"--dupes") == 0) {
    DupResult r; wchar_t sz[32]; int errs = 0;
    dup_scan(argv[2], has(argc, argv, L"--sub"), &r);
    for (int i = 0; i < r.files.n; i++) outf(L"group=%d keep=%d size=%llu %s", r.files.v[i].group, r.files.v[i].keep, r.files.v[i].size, r.files.v[i].path);
    format_size(r.wasted, sz, 32);
    outf(L"groups=%d duplicates=%d wasted=%llu (%s)", r.groups, r.dupes, r.wasted, sz);
    int moved = 0;
    if (has(argc, argv, L"--move")) { moved = dup_move(argv[2], &r, NULL, 1, NULL, &errs); outf(L"moved=%d errors=%d", moved, errs); }
    int ret = has(argc, argv, L"--move") ? moved : r.dupes;
    dup_free(&r); return ret;
  }
  if (argc >= 4 && _wcsicmp(cmd, L"--old") == 0) {
    FList l; ULONGLONG tot; int errs = 0, days = _wtoi(argv[3]);
    old_scan(argv[2], days, &l, &tot);
    for (int i = 0; i < l.n; i++) outf(L"old %s", l.v[i].path);
    outf(L"old_files=%d days=%d bytes=%llu", l.n, days, tot);
    int ret = l.n;
    if (has(argc, argv, L"--move")) { ret = old_move(argv[2], &l, 1, NULL, &errs); outf(L"moved=%d errors=%d", ret, errs); }
    flist_free(&l); return ret;
  }
  if (argc >= 3 && _wcsicmp(cmd, L"--rename") == 0) {
    RenameOpts o; ZeroMemory(&o, sizeof o); RenamePlan p; int errs = 0; const wchar_t *v;
    if ((v = opt(argc, argv, L"--contains"))) wcsncpy(o.contains, v, 127);
    if ((v = opt(argc, argv, L"--ext"))) wcsncpy(o.ext, v, 63);
    if ((v = opt(argc, argv, L"--prefix"))) wcsncpy(o.prefix, v, 127);
    if ((v = opt(argc, argv, L"--suffix"))) wcsncpy(o.suffix, v, 127);
    if ((v = opt(argc, argv, L"--find"))) wcsncpy(o.find, v, 127);
    if ((v = opt(argc, argv, L"--replace"))) wcsncpy(o.repl, v, 127);
    for (int i = 1; i < argc - 3; i++) if (!_wcsicmp(argv[i], L"--number")) {
      o.numbering = 1; wcsncpy(o.base, argv[i + 1], 127); o.start = _wtoi(argv[i + 2]); o.digits = _wtoi(argv[i + 3]);
    }
    rename_plan(argv[2], &o, &p);
    for (int i = 0; i < p.n; i++) outf(L"rename %s -> %s", p.from[i], p.to[i]);
    outf(L"planned=%d", p.n);
    int ret = p.n;
    if (has(argc, argv, L"--apply")) { ret = rename_apply(argv[2], &p, 0, NULL, &errs); outf(L"renamed=%d errors=%d", ret, errs); }
    rename_free(&p); return ret;
  }
  if (argc >= 3 && _wcsicmp(cmd, L"--stats") == 0) {
    CatStat st[NCATS_MAX]; ULONGLONG tot; wchar_t sz[32];
    int n = stats_scan(argv[2], has(argc, argv, L"--sub"), st, &tot);
    for (int i = 0; i <= cat_count(); i++) if (st[i].count) { format_size(st[i].bytes, sz, 32); outf(L"%s count=%d bytes=%llu (%s)", cat_name(i), st[i].count, st[i].bytes, sz); }
    outf(L"files=%d bytes=%llu", n, tot);
    return n;
  }
  if (argc >= 5 && _wcsicmp(cmd, L"--rule-add") == 0) {
    int type = _wcsicmp(argv[2], L"name") == 0 ? RULE_NAME : RULE_EXT;
    if (!rule_valid(type, argv[3], argv[4]) || g_nrules >= MAX_RULES) { outf(L"rule rejected"); return 1000; }
    g_rules[g_nrules].type = type; wcsncpy(g_rules[g_nrules].pat, argv[3], 255); wcsncpy(g_rules[g_nrules].folder, argv[4], 255);
    g_nrules++; rules_save(); outf(L"rules=%d", g_nrules); return 0;
  }
  if (_wcsicmp(cmd, L"--rule-list") == 0) {
    for (int i = 0; i < g_nrules; i++) outf(L"%d %s %s -> %s", i + 1, g_rules[i].type == RULE_NAME ? L"name" : L"ext", g_rules[i].pat, g_rules[i].folder);
    outf(L"rules=%d", g_nrules); return g_nrules;
  }
  if (_wcsicmp(cmd, L"--rule-clear") == 0) { g_nrules = 0; rules_save(); outf(L"rules=0"); return 0; }
  if (argc >= 3 && _wcsicmp(cmd, L"--auto") == 0) {
    AutoCfg c; AutoSummary s; const wchar_t *with = opt(argc, argv, L"--with"), *days = opt(argc, argv, L"--days");
    c.rules = ini_int(L"AutoRules", 1); c.sort = ini_int(L"AutoDoSort", 1); c.sortMode = ini_int(L"AutoSort", 1) ? SF_TYPE : SF_EXT;
    c.date = ini_int(L"AutoDate", 0); c.old = ini_int(L"AutoOld", 0); c.oldDays = ini_int(L"OldDays", 90); c.dupes = ini_int(L"AutoDupes", 0);
    if (with) {
      c.rules = in_list(with, L"rules"); c.date = in_list(with, L"date"); c.old = in_list(with, L"old"); c.dupes = in_list(with, L"dupes");
      c.sort = in_list(with, L"type") || in_list(with, L"ext"); c.sortMode = in_list(with, L"ext") ? SF_EXT : SF_TYPE;
    }
    if (days) c.oldDays = _wtoi(days);
    if (c.rules && !g_nrules) c.rules = 0;
    int r = auto_run(argv[2], &c, 1, &s);
    wchar_t sum[400]; summary_text(&s, sum);
    outf(L"auto sorted=%d byRules=%d dupes=%d old=%d errors=%d", s.sorted, s.byRules, s.dupes, s.old, s.errs);
    outf(L"summary: %s", sum[0] ? sum : L"nothing to do");
    return r < 0 ? 1000 : r;
  }
  if (argc >= 4 && _wcsicmp(cmd, L"--set") == 0) { WritePrivateProfileStringW(L"Settings", argv[2], argv[3], g_ini); return 0; }
  if (argc >= 3 && _wcsicmp(cmd, L"--get") == 0) { wchar_t b[PBUF]; ini_str(argv[2], L"", b, PBUF); outf(L"%s=%s", argv[2], b); return 0; }
  g_cliMode = 0;
  return -1;
}

int WINAPI wWinMain(HINSTANCE inst, HINSTANCE prev, PWSTR cmd, int show) {
  (void)prev; (void)cmd;
  g_inst = inst;
  init_paths();
  g_lang = ini_int(L"Language", 0) == 1 ? 1 : 0;
  int argc = 0; wchar_t **argv = CommandLineToArgvW(GetCommandLineW(), &argc);
  int startTray = 0;
  if (argv && argc >= 2) {
    if (_wcsicmp(argv[1], L"--tray") == 0) startTray = 1;
    else if (_wcsicmp(argv[1], L"--folder") == 0) { if (argc >= 3) norm_path(argv[2], g_cliFolder, PBUF); }
    else if (_wcsicmp(argv[1], L"--page") == 0) {   /* open on a tab; --run also presses its main button */
      if (argc >= 3) g_startPage = _wtoi(argv[2]);
      if (has(argc, argv, L"--run")) g_startRun = 1;
      if (has(argc, argv, L"--run-auto")) g_startRun = 2;
    }
    else { int r = cli(argc, argv); if (r != -1) { LocalFree(argv); return r; } }
  }
  if (argv) LocalFree(argv);

  g_elevated = is_elevated();
  g_mutex = CreateMutexW(NULL, TRUE, MUTEX_NAME);
  DWORD me = GetLastError();
  if (me == ERROR_ALREADY_EXISTS || (!g_mutex && me == ERROR_ACCESS_DENIED)) {
    HWND other = FindWindowW(APP_CLASS, NULL);
    if (other) PostMessageW(other, WM_WAKEUP, 0, 0);
    return 0;
  }
  CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
  INITCOMMONCONTROLSEX icc = { sizeof icc, ICC_STANDARD_CLASSES | ICC_WIN95_CLASSES | ICC_LISTVIEW_CLASSES }; InitCommonControlsEx(&icc);

  HDC sdc = GetDC(NULL); g_dpi = GetDeviceCaps(sdc, LOGPIXELSY); ReleaseDC(NULL, sdc);
  if (g_dpi < 96) g_dpi = 96;
  rules_load();
  g_themeSel = ini_int(L"Theme", 2); if (g_themeSel < 0 || g_themeSel > 2) g_themeSel = 2;
  g_accentSel = ini_int(L"Accent", 0); if (g_accentSel < 0 || g_accentSel >= NACCENTS) g_accentSel = 0;
  make_fonts();
  apply_theme();   /* palette + brushes before any control exists */

  WNDCLASSEXW wc; ZeroMemory(&wc, sizeof wc);
  wc.cbSize = sizeof wc; wc.lpfnWndProc = wndproc; wc.hInstance = inst;
  wc.hIcon = LoadIconW(inst, MAKEINTRESOURCEW(IDI_APP));
  wc.hIconSm = (HICON)LoadImageW(inst, MAKEINTRESOURCEW(IDI_APP), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
  wc.hCursor = LoadCursor(NULL, IDC_ARROW); wc.hbrBackground = NULL; wc.lpszClassName = APP_CLASS;
  RegisterClassExW(&wc);

  DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN;
  RECT r = { 0, 0, S(CW), S(CH) }; AdjustWindowRect(&r, style, FALSE);
  HWND h = CreateWindowExW(0, APP_CLASS, g_elevated ? APP_TITLE L" (Admin)" : APP_TITLE, style, CW_USEDEFAULT, CW_USEDEFAULT, r.right - r.left, r.bottom - r.top, NULL, NULL, inst, NULL);
  if (!h) return 1;
  { typedef BOOL (WINAPI *CWMFEx)(HWND, UINT, DWORD, void *);
    CWMFEx f = (CWMFEx)(void *)GetProcAddress(GetModuleHandleW(L"user32.dll"), "ChangeWindowMessageFilterEx");
    if (f) f(h, WM_WAKEUP, 1 /* MSGFLT_ALLOW */, NULL); }

  if (startTray && chk(g_auto) && chk(g_tray)) { g_trayTipShown = 1; g_inTray = 0; go_to_tray(); }
  else { ShowWindow(h, startTray ? SW_SHOWMINNOACTIVE : show); UpdateWindow(h); }
  if (g_startRun) PostMessageW(h, WM_STARTRUN, 0, 0);

  MSG msg;
  while (GetMessageW(&msg, NULL, 0, 0) > 0) {
    if (!IsDialogMessageW(h, &msg)) { TranslateMessage(&msg); DispatchMessageW(&msg); }
  }
  CoUninitialize();
  if (g_mutex) CloseHandle(g_mutex);
  return 0;
}
