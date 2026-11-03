
/* Darshan File Manager 1.1 - by Darshan Corporation
   Plain Win32 (Unicode). Build:
   x86_64-w64-mingw32-windres app.rc -O coff -o app.res
   x86_64-w64-mingw32-gcc -municode -mwindows -static -O2 -s main.c app.res -o DarshanFileManager.exe -lcomctl32 -lshell32 -lole32 -luuid -lgdi32 -ladvapi32
*/
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define _WIN32_WINNT 0x0601
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <commctrl.h>
#include <wchar.h>
#include <stdio.h>
#include <stdlib.h>
#include "resource.h"

#define APP_NAME   L"Darshan File Manager"
#define APP_CLASS  L"DarshanFileManagerWnd"
#define RUN_VALUE  L"DarshanFileManager"
#define WM_TRAY    (WM_APP + 1)
#define WM_WAKEUP  (WM_APP + 2)
#define TIMER_AUTO 1
#define PBUF 2048

enum { IDC_PATH = 101, IDC_BROWSE, IDC_DATE, IDC_EXT, IDC_TYPE, IDC_UNDO, IDC_OPEN,
       IDC_AUTO, IDC_INTERVAL, IDC_R_EXT, IDC_R_TYPE, IDC_TRAYCHK, IDC_STARTUP,
       IDC_LOG, IDC_STATUS, IDC_G1, IDC_G2, IDC_LBL1, IDC_LBL2, IDC_LBL3, IDC_LBL4, IDC_ADMIN,
       IDM_OPEN = 201, IDM_EXIT };

enum { MODE_EXT = 0, MODE_TYPE = 1 };

static HINSTANCE g_inst;
static HWND g_wnd, g_path, g_log, g_status, g_auto, g_interval, g_rext, g_rtype, g_tray, g_startup;
static HFONT g_font, g_fontHead, g_fontSub;
static HBRUSH g_white;
static int g_dpi = 96;
static int g_inTray = 0;
static int g_trayTipShown = 0;
static wchar_t g_settingsDir[PBUF], g_ini[PBUF], g_undoLog[PBUF], g_self[PBUF];
static const int g_intervals[] = { 1, 5, 15, 30 };
static HANDLE g_mutex;
static HWND g_adminBtn;
static int g_elevated = 0;
static int g_lastDenied = 0;          /* Public Desktop items refused (access denied) in the last sort */
static int g_deniedNoticeShown = 0;   /* auto mode: say it once per session, not every interval */
static wchar_t g_cliFolder[PBUF];     /* --folder "path" (used by the Run as admin relaunch) */
#define MUTEX_NAME L"DarshanFileManager_SingleInstance"

#define S(x) MulDiv((x), g_dpi, 96)

/* ------------------------------------------------------------------ */
/* Categories                                                          */
/* ------------------------------------------------------------------ */
typedef struct { const wchar_t *folder; const wchar_t *exts; } Cat;
static const Cat CATS[] = {
  { L"Images", L".jpg.jpeg.jpe.jfif.jif.pjpeg.pjp.png.apng.gif.bmp.dib.webp.heic.heif.hif.avif.jxl.svg.svgz.ico.icns.cur.tif.tiff.raw.cr2.cr3.crw.nef.nrw.arw.srf.sr2.dng.orf.rw2.raf.pef.srw.x3f.3fr.erf.kdc.mrw.psd.psb.ai.eps.tga.pcx.ppm.pgm.pbm.pnm.hdr.exr.dds.jp2.j2k.jpf.jpx.wbmp.emf.wmf.xcf.kra.ora." },
  { L"Videos", L".mp4.m4v.mkv.avi.mov.qt.wmv.asf.flv.f4v.swf.webm.3gp.3g2.3gpp.ts.mts.m2ts.m2t.mpg.mpeg.mpe.mpv.m2v.vob.ogv.rm.rmvb.divx.xvid.dv.mxf.yuv.h264.h265.hevc.insv.lrv." },
  { L"Music", L".mp3.wav.wave.aac.flac.ogg.oga.opus.m4a.m4b.m4r.wma.amr.awb.mid.midi.kar.aiff.aif.aifc.alac.ape.wv.mka.3ga.caf.ac3.dts.mp2.mpa.ra.au.snd.voc.gsm.spx.xm.mod.it.s3m.cda." },
  { L"PDF", L".pdf." },
  { L"Documents", L".doc.docx.docm.dot.dotx.dotm.odt.ott.rtf.txt.text.wpd.wps.pages.md.markdown.rst.tex.log.nfo.xls.xlsx.xlsm.xlsb.xlt.xltx.ods.ots.csv.tsv.numbers.ppt.pptx.pptm.pot.potx.pps.ppsx.odp.otp.key.xps.oxps.one.onetoc2.msg.eml.emlx.mht.mhtml.vsd.vsdx.pub.gdoc.gsheet.gslides.hwp." },
  { L"Ebooks", L".epub.mobi.azw.azw3.kf8.kfx.fb2.djvu.djv.cbz.cbr.cb7.lit.lrf.ibooks." },
  { L"Archives", L".zip.zipx.rar.7z.tar.gz.tgz.bz2.tbz.tbz2.xz.txz.lz.lzma.lz4.zst.tzst.z.cab.arj.ace.lzh.lha.iso.img.dmg.vhd.vmdk.wim.sit.sitx.cpio.001.002.pak." },
  { L"Programs", L".apk.xapk.apks.apkm.aab.obb.exe.msi.msix.msixbundle.appx.appxbundle.bat.cmd.com.scr.jar.vbs.ps1.reg.deb.rpm.appimage.run.bin.snap.flatpakref.ipa.pkg.mpkg.gadget." },
  { L"Code", L".py.pyw.pyc.ipynb.c.h.cpp.cc.cxx.hpp.hh.hxx.cs.java.class.kt.kts.dart.js.mjs.cjs.tsx.jsx.vue.svelte.html.htm.xhtml.css.scss.sass.less.php.phtml.rb.go.rs.swift.m.mm.sql.db.sqlite.sqlite3.json.jsonc.xml.xsd.xsl.yml.yaml.toml.ini.cfg.conf.env.properties.gradle.sh.bash.zsh.fish.ino.lua.pl.pm.r.rmd.scala.groovy.asm.s.v.sv.vhdl.f.f90.pas.vb.vba.fs.erl.ex.exs.hs.clj.lisp.ml.jl.nim.zig.cmake.makefile.mk.dockerfile.patch.diff." },
  { L"Fonts", L".ttf.otf.woff.woff2.eot.fon.fnt.pfb.pfm.ttc." },
  { L"Design", L".xd.fig.sketch.cdr.indd.idml.afdesign.afphoto.blend.obj.fbx.stl.3ds.max.dwg.dxf.skp.c4d.ma.mb.dae.gltf.glb.ply.3mf.step.stp.iges.igs.sldprt." },
  { L"Contacts_Calendar", L".vcf.vcard.ics.ical.ifb." },
  { L"Torrents", L".torrent." },
};
#define NCATS (int)(sizeof CATS / sizeof CATS[0])

static void now_str(wchar_t *out, int n) {
  SYSTEMTIME st; GetLocalTime(&st);
  _snwprintf(out, n, L"%02d:%02d", st.wHour, st.wMinute); out[n-1] = 0;
}

static void log_line(const wchar_t *fmt, ...) {
  wchar_t buf[PBUF], line[PBUF + 16], t[16];
  va_list ap; va_start(ap, fmt); _vsnwprintf(buf, PBUF, fmt, ap); va_end(ap); buf[PBUF-1] = 0;
  now_str(t, 16);
  _snwprintf(line, PBUF + 16, L"%s   %s", t, buf); line[PBUF+15] = 0;
  if (!g_log) return;
  int c = (int)SendMessageW(g_log, LB_ADDSTRING, 0, (LPARAM)line);
  while (SendMessageW(g_log, LB_GETCOUNT, 0, 0) > 1500) { SendMessageW(g_log, LB_DELETESTRING, 0, 0); c--; }
  SendMessageW(g_log, LB_SETTOPINDEX, (WPARAM)(c > 0 ? c : 0), 0);
}

static void set_status(const wchar_t *fmt, ...) {
  wchar_t buf[PBUF]; va_list ap; va_start(ap, fmt); _vsnwprintf(buf, PBUF, fmt, ap); va_end(ap); buf[PBUF-1] = 0;
  if (g_status) SetWindowTextW(g_status, buf);
}

/* ------------------------------------------------------------------ */
/* File rules                                                          */
/* ------------------------------------------------------------------ */
static int ends_with_i(const wchar_t *s, const wchar_t *suf) {
  size_t a = wcslen(s), b = wcslen(suf);
  return a >= b && _wcsicmp(s + a - b, suf) == 0;
}

static int should_skip(const wchar_t *full, const WIN32_FIND_DATAW *fd) {
  DWORD at = fd->dwFileAttributes;
  const wchar_t *n = fd->cFileName;
  if (at & FILE_ATTRIBUTE_DIRECTORY) return 1;
  if (at & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)) return 1;
  if (at & FILE_ATTRIBUTE_REPARSE_POINT) return 1;
  if (_wcsicmp(n, L"desktop.ini") == 0 || _wcsicmp(n, L"thumbs.db") == 0) return 1;
  if (n[0] == L'~' && n[1] == L'$') return 1;                 /* Office lock files */
  if (ends_with_i(n, L".crdownload") || ends_with_i(n, L".tmp") || ends_with_i(n, L".part") ||
      ends_with_i(n, L".partial") || ends_with_i(n, L".download") || ends_with_i(n, L".opdownload")) return 1;
  if (_wcsicmp(full, g_self) == 0) return 1;                  /* never move the app itself */
  return 0;
}

/* extension without the dot; returns 0 if the name has no usable extension */
static int get_ext(const wchar_t *name, wchar_t *out, int n) {
  const wchar_t *dot = wcsrchr(name, L'.');
  out[0] = 0;
  if (!dot || dot == name || dot[1] == 0) return 0;          /* ".gitignore", "file." */
  int len = (int)wcslen(dot + 1);
  if (len > 12 || len >= n) return 0;
  for (const wchar_t *p = dot + 1; *p; p++)
    if (!iswalnum(*p) && *p != L'_' && *p != L'-') return 0;  /* "John.Smith resume" */
  wcscpy(out, dot + 1);
  return 1;
}

/* two-part extensions kept together: backup.tar.gz -> TAR.GZ */
static int compound_ext(const wchar_t *name, wchar_t *out, int n) {
  static const wchar_t *C[] = { L".tar.gz", L".tar.bz2", L".tar.xz", L".tar.zst", L".tar.lz", L".tar.lzma", L".tar.z" };
  size_t len = wcslen(name);
  for (int i = 0; i < (int)(sizeof C / sizeof C[0]); i++) {
    size_t cl = wcslen(C[i]);
    if (len > cl && _wcsicmp(name + len - cl, C[i]) == 0 && (int)cl < n) { wcscpy(out, C[i] + 1); return 1; }
  }
  return 0;
}

static const wchar_t *category_for(const wchar_t *name) {
  wchar_t ext[32], key[40];
  if (!get_ext(name, ext, 32)) return L"Others";
  _snwprintf(key, 40, L".%s.", ext); key[39] = 0;
  CharLowerW(key);
  for (int i = 0; i < NCATS; i++) if (wcsstr(CATS[i].exts, key)) return CATS[i].folder;
  return L"Others";
}

static int exists(const wchar_t *p) { return GetFileAttributesW(p) != INVALID_FILE_ATTRIBUTES; }

/* dir\name, or dir\name (1).ext, (2)... if taken */
static int unique_dest(const wchar_t *dir, const wchar_t *name, wchar_t *out, int n) {
  _snwprintf(out, n, L"%s\\%s", dir, name); out[n-1] = 0;
  if (!exists(out)) return 1;
  const wchar_t *dot = wcsrchr(name, L'.');
  if (dot == name) dot = NULL;
  int base = dot ? (int)(dot - name) : (int)wcslen(name);
  for (int k = 1; k < 10000; k++) {
    _snwprintf(out, n, L"%s\\%.*s (%d)%s", dir, base, name, k, dot ? dot : L""); out[n-1] = 0;
    if (!exists(out)) return 1;
  }
  return 0;
}

/* ------------------------------------------------------------------ */
/* Undo log (UTF-16LE text: first line = label, then src<TAB>dst)     */
/* ------------------------------------------------------------------ */
typedef struct { wchar_t *buf; size_t len, cap; } WBuf;
static void wb_add(WBuf *b, const wchar_t *s) {
  size_t l = wcslen(s);
  if (b->len + l + 1 > b->cap) {
    size_t nc = (b->cap ? b->cap * 2 : 8192); while (nc < b->len + l + 1) nc *= 2;
    wchar_t *nb = (wchar_t *)realloc(b->buf, nc * sizeof(wchar_t)); if (!nb) return;
    b->buf = nb; b->cap = nc;
  }
  memcpy(b->buf + b->len, s, l * sizeof(wchar_t)); b->len += l; b->buf[b->len] = 0;
}

static int write_utf16_file(const wchar_t *path, const wchar_t *text, size_t len) {
  HANDLE h = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if (h == INVALID_HANDLE_VALUE) return 0;
  DWORD w; WORD bom = 0xFEFF;
  WriteFile(h, &bom, 2, &w, NULL);
  if (len) WriteFile(h, text, (DWORD)(len * sizeof(wchar_t)), &w, NULL);
  CloseHandle(h); return 1;
}

/* ------------------------------------------------------------------ */
/* Core operations                                                     */
/* ------------------------------------------------------------------ */
static int folder_ok(const wchar_t *folder) {
  DWORD a = GetFileAttributesW(folder);
  return a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY);
}

/* ---- Desktop helpers: Windows shows the user's Desktop merged with Public Desktop ---- */
static void norm_path(const wchar_t *in, wchar_t *out, int n) {
  wcsncpy(out, in, n - 1); out[n-1] = 0;
  for (wchar_t *p = out; *p; p++) if (*p == L'/') *p = L'\\';
  size_t l = wcslen(out);
  while (l > 3 && out[l-1] == L'\\') out[--l] = 0;
}
static int same_path(const wchar_t *a, const wchar_t *b) {
  wchar_t x[PBUF], y[PBUF]; norm_path(a, x, PBUF); norm_path(b, y, PBUF);
  return x[0] && _wcsicmp(x, y) == 0;
}
static int shell_folder(int csidl, wchar_t *out) {
  wchar_t p[MAX_PATH] = L"";
  if (FAILED(SHGetFolderPathW(NULL, csidl, NULL, SHGFP_TYPE_CURRENT, p)) || !p[0]) return 0;
  wcsncpy(out, p, PBUF - 1); out[PBUF-1] = 0; return 1;
}
/* if folder is the user's Desktop, put the all-users (Public) Desktop path in common and return 1 */
static int is_user_desktop(const wchar_t *folder, wchar_t *common) {
  wchar_t d[PBUF];
  if (!shell_folder(CSIDL_DESKTOPDIRECTORY, d) || !same_path(folder, d)) return 0;
  if (!shell_folder(CSIDL_COMMON_DESKTOPDIRECTORY, common) || same_path(common, folder) || !folder_ok(common)) return 0;
  return 1;
}
static int is_elevated(void) {
#ifdef FORCE_NOT_ELEVATED   /* test builds only */
  return 0;
#endif
  HANDLE tok; TOKEN_ELEVATION e; DWORD sz = 0; int r = 0;
  if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &tok)) {
    if (GetTokenInformation(tok, TokenElevation, &e, sizeof e, &sz)) r = e.TokenIsElevated != 0;
    CloseHandle(tok);
  }
  return r;
}

/* scan one folder (scan) and move its files into category folders under dest */
static void sort_one(int mode, const wchar_t *scan, const wchar_t *dest, int isCommon, int verbose,
                     WBuf *undo, int *moved, int *errs, int *denied) {
  wchar_t pat[PBUF], src[PBUF], dir[PBUF], dst[PBUF], line[PBUF * 2 + 4];
  WIN32_FIND_DATAW fd;
  _snwprintf(pat, PBUF, L"%s\\*", scan); pat[PBUF-1] = 0;
  HANDLE h = FindFirstFileW(pat, &fd);
  if (h == INVALID_HANDLE_VALUE) return;
  do {
    _snwprintf(src, PBUF, L"%s\\%s", scan, fd.cFileName); src[PBUF-1] = 0;
    if (should_skip(src, &fd)) continue;
    wchar_t sub[64];
    if (mode == MODE_EXT) {
      if (compound_ext(fd.cFileName, sub, 64)) CharUpperW(sub);
      else if (get_ext(fd.cFileName, sub, 64)) CharUpperW(sub); else wcscpy(sub, L"NO_EXTENSION");
    } else {
      wcscpy(sub, category_for(fd.cFileName));
    }
    _snwprintf(dir, PBUF, L"%s\\%s", dest, sub); dir[PBUF-1] = 0;
    DWORD da = GetFileAttributesW(dir); int created = 0;
    if (da != INVALID_FILE_ATTRIBUTES && !(da & FILE_ATTRIBUTE_DIRECTORY)) {   /* a FILE named like the folder */
      (*errs)++; if (verbose) log_line(L"Skipped %s (a file named \"%s\" is in the way)", fd.cFileName, sub); continue;
    }
    if (da == INVALID_FILE_ATTRIBUTES) {
      if (!CreateDirectoryW(dir, NULL)) { (*errs)++; if (verbose) log_line(L"Could not create folder %s", sub); continue; }
      created = 1;
    }
    if (!unique_dest(dir, fd.cFileName, dst, PBUF)) { (*errs)++; continue; }
    if (MoveFileExW(src, dst, MOVEFILE_COPY_ALLOWED)) {   /* no REPLACE flag: never overwrites */
      (*moved)++;
      _snwprintf(line, PBUF * 2 + 4, L"%s\t%s\n", src, dst); line[PBUF*2+3] = 0;   /* real source: undo puts it back there */
      wb_add(undo, line);
      if (verbose) {
        const wchar_t *dn = wcsrchr(dst, L'\\');
        log_line(L"   %s%s  \x2192  %s\\%s", isCommon ? L"[Public Desktop] " : L"", fd.cFileName, sub, dn ? dn + 1 : dst);
      }
    } else {
      DWORD e = GetLastError();
      if (created) RemoveDirectoryW(dir);              /* don't leave an empty folder behind */
      if (isCommon && e == ERROR_ACCESS_DENIED) (*denied)++;    /* needs admin: reported once, below */
      else {
        (*errs)++;
        if (verbose) log_line(L"   ! %s could not be moved (open in another app?)", fd.cFileName);
      }
    }
  } while (FindNextFileW(h, &fd));
  FindClose(h);
}

/* returns number moved; *errors gets failures; g_lastDenied = Public Desktop items that need admin */
static int run_sort(int mode, const wchar_t *folder, int verbose, int *errors) {
  int moved = 0, errs = 0, denied = 0;
  wchar_t pat[PBUF], common[PBUF];
  WIN32_FIND_DATAW fd;
  g_lastDenied = 0;
  _snwprintf(pat, PBUF, L"%s\\*", folder); pat[PBUF-1] = 0;
  HANDLE h = FindFirstFileW(pat, &fd);
  if (h == INVALID_HANDLE_VALUE) { if (errors) *errors = 1; return -1; }
  FindClose(h);
  WBuf undo = { 0 };
  wb_add(&undo, mode == MODE_EXT ? L"Sort by Extension\n" : L"Sort by Type\n");
  sort_one(mode, folder, folder, 0, verbose, &undo, &moved, &errs, &denied);
  /* Desktop selected: also sort shortcuts that live in Public Desktop (C:\Users\Public\Desktop),
     into the same folders inside the user's Desktop (Desktop\LNK etc.) */
  if (is_user_desktop(folder, common))
    sort_one(mode, common, folder, 1, verbose, &undo, &moved, &errs, &denied);
  if (moved > 0) write_utf16_file(g_undoLog, undo.buf, undo.len);   /* keep previous log when nothing moved */
  free(undo.buf);
  g_lastDenied = denied;
  if (errors) *errors = errs;
  return moved;
}

static int make_date_folder(const wchar_t *folder, wchar_t *name, int n, int *already) {
  SYSTEMTIME st; GetLocalTime(&st);
  wchar_t p[PBUF];
  _snwprintf(name, n, L"%02d-%02d-%04d", st.wDay, st.wMonth, st.wYear); name[n-1] = 0;
  _snwprintf(p, PBUF, L"%s\\%s", folder, name); p[PBUF-1] = 0;
  *already = 0;
  if (exists(p)) { *already = 1; return 1; }
  return CreateDirectoryW(p, NULL) ? 1 : 0;
}

/* returns moved back count, -1 if no log */
static int run_undo(int verbose, int *errors, wchar_t *label, int labeln) {
  HANDLE h = CreateFileW(g_undoLog, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
  if (h == INVALID_HANDLE_VALUE) return -1;
  DWORD size = GetFileSize(h, NULL), rd = 0;
  wchar_t *buf = (wchar_t *)calloc(size / 2 + 2, sizeof(wchar_t));
  if (!buf) { CloseHandle(h); return -1; }
  ReadFile(h, buf, size, &rd, NULL); CloseHandle(h);
  wchar_t *text = buf; if (text[0] == 0xFEFF) text++;
  /* split lines */
  int cap = 256, nl = 0; wchar_t **lines = (wchar_t **)malloc(cap * sizeof(wchar_t *));
  for (wchar_t *p = text; *p; ) {
    wchar_t *e = wcschr(p, L'\n'); if (e) *e = 0;
    if (nl == cap) { cap *= 2; lines = (wchar_t **)realloc(lines, cap * sizeof(wchar_t *)); }
    lines[nl++] = p;
    if (!e) break;
    p = e + 1;
  }
  if (label && nl > 0) { wcsncpy(label, lines[0], labeln - 1); label[labeln-1] = 0; }
  int back = 0, errs = 0;
  WBuf keep = { 0 };
  g_lastDenied = 0;
  for (int i = nl - 1; i >= 1; i--) {        /* reverse order */
    wchar_t *tab = wcschr(lines[i], L'\t'); if (!tab) continue;
    *tab = 0; const wchar_t *src = lines[i], *dst = tab + 1;
    if (!exists(dst)) { errs++; continue; }
    wchar_t target[PBUF];
    if (exists(src)) {   /* original spot taken again -> name (1).ext */
      wchar_t d[PBUF]; wcsncpy(d, src, PBUF - 1); d[PBUF-1] = 0;
      wchar_t *bs = wcsrchr(d, L'\\'); if (!bs) { errs++; continue; }
      *bs = 0;
      if (!unique_dest(d, bs + 1, target, PBUF)) { errs++; continue; }
    } else { wcsncpy(target, src, PBUF - 1); target[PBUF-1] = 0; }
    if (MoveFileExW(dst, target, MOVEFILE_COPY_ALLOWED)) {
      back++;
      wchar_t parent[PBUF]; wcsncpy(parent, dst, PBUF - 1); parent[PBUF-1] = 0;
      wchar_t *bs = wcsrchr(parent, L'\\'); if (bs) { *bs = 0; RemoveDirectoryW(parent); }  /* only if empty */
      continue;
    }
    if (GetLastError() == ERROR_ACCESS_DENIED) g_lastDenied++;   /* e.g. back into Public Desktop without admin */
    errs++;
    /* keep this entry so a second Undo (e.g. as admin) can still put it back */
    wchar_t kl[PBUF * 2 + 4]; _snwprintf(kl, PBUF * 2 + 4, L"%s\t%s\n", src, dst); kl[PBUF*2+3] = 0;
    wb_add(&keep, kl);
  }
  if (keep.len > 0) {   /* some files could not go back: rewrite the log with only those */
    WBuf out = { 0 };
    wb_add(&out, nl > 0 ? lines[0] : L"Sort"); wb_add(&out, L"\n");
    wb_add(&out, keep.buf);
    write_utf16_file(g_undoLog, out.buf, out.len);
    free(out.buf);
  } else DeleteFileW(g_undoLog);
  free(keep.buf);
  free(lines); free(buf);
  if (errors) *errors = errs;
  (void)verbose;
  return back;
}

/* ------------------------------------------------------------------ */
/* Settings                                                            */
/* ------------------------------------------------------------------ */
static void init_paths(void) {
  GetModuleFileNameW(NULL, g_self, PBUF);
  wchar_t appdata[PBUF];
  DWORD n = GetEnvironmentVariableW(L"APPDATA", appdata, PBUF);
  if (n == 0 || n >= PBUF) { wcscpy(appdata, g_self); wchar_t *b = wcsrchr(appdata, L'\\'); if (b) *b = 0; }
  _snwprintf(g_settingsDir, PBUF, L"%s\\DarshanFileManager", appdata);
  CreateDirectoryW(g_settingsDir, NULL);
  _snwprintf(g_ini, PBUF, L"%s\\settings.ini", g_settingsDir);
  _snwprintf(g_undoLog, PBUF, L"%s\\last_sort_undo.log", g_settingsDir);
  if (!exists(g_ini)) write_utf16_file(g_ini, L"[Settings]\r\n", 12);  /* UTF-16 so Gujarati paths survive */
}

static void default_folder(wchar_t *out, int n) {
  PWSTR p = NULL;
  if (SUCCEEDED(SHGetKnownFolderPath(&FOLDERID_Downloads, 0, NULL, &p)) && p) {
    wcsncpy(out, p, n - 1); out[n-1] = 0; CoTaskMemFree(p); return;
  }
  wchar_t up[PBUF];
  if (GetEnvironmentVariableW(L"USERPROFILE", up, PBUF)) { _snwprintf(out, n, L"%s\\Downloads", up); out[n-1] = 0; }
  else wcscpy(out, L"C:\\");
}

static int ini_int(const wchar_t *k, int def) { return (int)GetPrivateProfileIntW(L"Settings", k, def, g_ini); }
static void ini_set_int(const wchar_t *k, int v) { wchar_t b[32]; _snwprintf(b, 32, L"%d", v); WritePrivateProfileStringW(L"Settings", k, b, g_ini); }

static void save_settings(void) {
  if (!g_wnd) return;
  wchar_t p[PBUF]; GetWindowTextW(g_path, p, PBUF);
  WritePrivateProfileStringW(L"Settings", L"Folder", p, g_ini);
  ini_set_int(L"AutoMode", SendMessageW(g_auto, BM_GETCHECK, 0, 0) == BST_CHECKED);
  int sel = (int)SendMessageW(g_interval, CB_GETCURSEL, 0, 0); if (sel < 0) sel = 1;
  ini_set_int(L"IntervalMinutes", g_intervals[sel]);
  ini_set_int(L"AutoSort", SendMessageW(g_rtype, BM_GETCHECK, 0, 0) == BST_CHECKED ? MODE_TYPE : MODE_EXT);
  ini_set_int(L"MinimizeToTray", SendMessageW(g_tray, BM_GETCHECK, 0, 0) == BST_CHECKED);
}

/* ------------------------------------------------------------------ */
/* Start with Windows                                                  */
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
  tray_icon(1, g_trayTipShown ? NULL : L"Still running in auto mode. Double-click the icon to open.");
  g_trayTipShown = 1;
}
static void restore_from_tray(void) {
  if (g_inTray) { tray_icon(0, NULL); g_inTray = 0; }
  ShowWindow(g_wnd, SW_SHOW); ShowWindow(g_wnd, SW_RESTORE); SetForegroundWindow(g_wnd);
}

/* ------------------------------------------------------------------ */
/* UI helpers                                                          */
/* ------------------------------------------------------------------ */
static int get_folder(wchar_t *out) {
  GetWindowTextW(g_path, out, PBUF);
  size_t l = wcslen(out);
  while (l > 3 && (out[l-1] == L'\\' || out[l-1] == L'/')) out[--l] = 0;
  if (!folder_ok(out)) { set_status(L"That folder doesn't exist. Use Browse... to pick one."); log_line(L"Folder not found: %s", out); return 0; }
  return 1;
}

static int risky_folder(const wchar_t *f) {
  wchar_t win[MAX_PATH] = L"", up[PBUF] = L"";
  GetWindowsDirectoryW(win, MAX_PATH); GetEnvironmentVariableW(L"USERPROFILE", up, PBUF);
  if (wcslen(f) <= 3) return 1;                                   /* C:\ */
  if (win[0] && _wcsnicmp(f, win, wcslen(win)) == 0) return 1;
  if (wcsstr(f, L"\\Program Files") || wcsstr(f, L"\\ProgramData")) return 1;
  if (up[0] && _wcsicmp(f, up) == 0) return 1;
  return 0;
}

static void do_sort(int mode, int automatic) {
  wchar_t f[PBUF]; int errs = 0;
  if (!get_folder(f)) return;
  if (!automatic && risky_folder(f)) {
    if (MessageBoxW(g_wnd, L"This looks like a system or root folder. Sorting it can break programs.\n\nSort it anyway?", APP_NAME, MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) != IDYES) return;
  }
  if (automatic && risky_folder(f)) { set_status(L"Auto mode skipped: system folder selected."); return; }
  HCURSOR old = SetCursor(LoadCursor(NULL, IDC_WAIT));
  const wchar_t *what = mode == MODE_EXT ? L"Sort by Extension" : L"Sort by Type";
  int moved = run_sort(mode, f, 1, &errs);
  SetCursor(old);
  wchar_t t[16]; now_str(t, 16);
  if (moved < 0) { log_line(L"Could not read folder %s", f); set_status(L"Could not read the folder."); return; }
  if (moved == 0 && automatic && (g_lastDenied == 0 || g_deniedNoticeShown)) { set_status(L"Auto mode: checked at %s, nothing new to sort.", t); return; }
  log_line(L"%s%s: %d file%s moved%s", automatic ? L"[Auto] " : L"", what, moved, moved == 1 ? L"" : L"s",
           errs ? L" (some skipped, see above)" : L"");
  if (errs) set_status(L"%d file%s moved, %d could not be moved (open in another app?).", moved, moved == 1 ? L"" : L"s", errs);
  else set_status(L"%d file%s moved. %s", moved, moved == 1 ? L"" : L"s", moved ? L"Use Undo if you didn't mean it." : L"Folder is already clean \x2714");
  if (g_lastDenied > 0) {
    if (!automatic || !g_deniedNoticeShown) {
      if (!g_elevated) {
        log_line(L"%d shortcut%s %s in Public Desktop \x2014 click 'Run as admin' to sort them too",
                 g_lastDenied, g_lastDenied == 1 ? L"" : L"s", g_lastDenied == 1 ? L"is" : L"are");
        set_status(L"%d moved. %d Public Desktop shortcut%s need admin: click 'Run as admin'.",
                   moved, g_lastDenied, g_lastDenied == 1 ? L"" : L"s");
        if (g_adminBtn) SetFocus(g_adminBtn);
      } else {
        log_line(L"%d Public Desktop item%s could not be moved (access denied, even as admin)",
                 g_lastDenied, g_lastDenied == 1 ? L"" : L"s");
        set_status(L"%d moved, %d Public Desktop item%s blocked.", moved, g_lastDenied, g_lastDenied == 1 ? L"" : L"s");
      }
      g_deniedNoticeShown = 1;
    }
  }
  if (automatic && g_inTray && moved > 0) {
    wchar_t b[128]; _snwprintf(b, 128, L"%d file%s sorted in your folder.", moved, moved == 1 ? L"" : L"s"); tray_icon(1, b);
  }
}

static void apply_timer(void) {
  KillTimer(g_wnd, TIMER_AUTO);
  int on = SendMessageW(g_auto, BM_GETCHECK, 0, 0) == BST_CHECKED;
  if (on) {
    int sel = (int)SendMessageW(g_interval, CB_GETCURSEL, 0, 0); if (sel < 0) sel = 1;
    SetTimer(g_wnd, TIMER_AUTO, (UINT)g_intervals[sel] * 60u * 1000u, NULL);
    set_status(L"Auto mode ON: sorting every %d minute%s while the app is open.", g_intervals[sel], g_intervals[sel] == 1 ? L"" : L"s");
  }
}

static int CALLBACK browse_cb(HWND h, UINT msg, LPARAM lp, LPARAM data) {
  (void)lp; if (msg == BFFM_INITIALIZED && data) SendMessageW(h, BFFM_SETSELECTIONW, TRUE, data);
  return 0;
}
static void browse(void) {
  wchar_t cur[PBUF], out[PBUF];
  GetWindowTextW(g_path, cur, PBUF);
  BROWSEINFOW bi; ZeroMemory(&bi, sizeof bi);
  bi.hwndOwner = g_wnd; bi.lpszTitle = L"Choose the folder to organize";
  bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE; bi.lpfn = browse_cb; bi.lParam = (LPARAM)cur;
  PIDLIST_ABSOLUTE pidl = SHBrowseForFolderW(&bi);
  if (pidl) {
    if (SHGetPathFromIDListW(pidl, out)) { SetWindowTextW(g_path, out); save_settings(); log_line(L"Folder: %s", out); set_status(L"Folder selected."); }
    CoTaskMemFree(pidl);
  }
}

static HWND mk(const wchar_t *cls, const wchar_t *txt, DWORD style, int x, int y, int w, int h, int id) {
  HWND c = CreateWindowExW(0, cls, txt, WS_CHILD | WS_VISIBLE | style, S(x), S(y), S(w), S(h), g_wnd, (HMENU)(INT_PTR)id, g_inst, NULL);
  SendMessageW(c, WM_SETFONT, (WPARAM)g_font, TRUE);
  return c;
}

#define CW 540
#define CH 610

static void build_ui(void) {
  wchar_t f[PBUF];
  mk(L"STATIC", L"Folder to organize", 0, 20, 84, 300, 20, IDC_LBL1);
  g_path = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                           S(20), S(106), S(400), S(28), g_wnd, (HMENU)IDC_PATH, g_inst, NULL);
  SendMessageW(g_path, WM_SETFONT, (WPARAM)g_font, TRUE);
  mk(L"BUTTON", L"Browse...", WS_TABSTOP | BS_OWNERDRAW, 428, 105, 92, 30, IDC_BROWSE);

  mk(L"BUTTON", L"Actions", BS_GROUPBOX, 12, 146, 516, 112, IDC_G1);
  mk(L"BUTTON", L"\U0001F4C1  Date Folder", WS_TABSTOP | BS_OWNERDRAW, 26, 170, 156, 38, IDC_DATE);
  mk(L"BUTTON", L"\U0001F524  Sort by Extension", WS_TABSTOP | BS_OWNERDRAW, 192, 170, 160, 38, IDC_EXT);
  mk(L"BUTTON", L"\U0001F5C2  Sort by Type", WS_TABSTOP | BS_OWNERDRAW, 362, 170, 154, 38, IDC_TYPE);
  mk(L"BUTTON", L"\x21A9  Undo last sort", WS_TABSTOP | BS_OWNERDRAW, 26, 216, 156, 30, IDC_UNDO);
  mk(L"BUTTON", L"\U0001F4C2  Open folder", WS_TABSTOP | BS_OWNERDRAW, 192, 216, 160, 30, IDC_OPEN);
  if (!g_elevated)   /* only offered when not already elevated */
    g_adminBtn = mk(L"BUTTON", L"\U0001F6E1  Run as admin", WS_TABSTOP | BS_OWNERDRAW, 362, 216, 154, 30, IDC_ADMIN);

  mk(L"BUTTON", L"Auto mode", BS_GROUPBOX, 12, 268, 516, 116, IDC_G2);
  g_auto = mk(L"BUTTON", L"Auto mode", BS_AUTOCHECKBOX | WS_TABSTOP, 26, 292, 110, 24, IDC_AUTO);
  mk(L"STATIC", L"every", 0, 146, 295, 40, 22, IDC_LBL2);
  g_interval = CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
                               S(188), S(291), S(64), S(200), g_wnd, (HMENU)IDC_INTERVAL, g_inst, NULL);
  SendMessageW(g_interval, WM_SETFONT, (WPARAM)g_font, TRUE);
  for (int i = 0; i < 4; i++) { wchar_t b[8]; _snwprintf(b, 8, L"%d", g_intervals[i]); SendMessageW(g_interval, CB_ADDSTRING, 0, (LPARAM)b); }
  mk(L"STATIC", L"minutes, using:", 0, 260, 295, 120, 22, IDC_LBL3);
  g_rext  = mk(L"BUTTON", L"Extension", BS_AUTORADIOBUTTON | WS_GROUP | WS_TABSTOP, 26, 322, 110, 24, IDC_R_EXT);
  g_rtype = mk(L"BUTTON", L"Type", BS_AUTORADIOBUTTON, 146, 322, 90, 24, IDC_R_TYPE);
  g_tray  = mk(L"BUTTON", L"Minimize to tray", BS_AUTOCHECKBOX | WS_GROUP | WS_TABSTOP, 26, 352, 170, 24, IDC_TRAYCHK);
  g_startup = mk(L"BUTTON", L"Start with Windows", BS_AUTOCHECKBOX | WS_TABSTOP, 236, 352, 200, 24, IDC_STARTUP);

  mk(L"STATIC", L"Activity", 0, 20, 394, 200, 20, IDC_LBL4);
  g_log = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | LBS_NOINTEGRALHEIGHT | LBS_NOSEL,
                          S(12), S(416), S(516), S(154), g_wnd, (HMENU)IDC_LOG, g_inst, NULL);
  SendMessageW(g_log, WM_SETFONT, (WPARAM)g_font, TRUE);
  SendMessageW(g_log, LB_SETHORIZONTALEXTENT, S(1400), 0);
  g_status = mk(L"STATIC", L"Ready.", SS_LEFTNOWORDWRAP | SS_ENDELLIPSIS, 14, 578, 512, 22, IDC_STATUS);

  /* load settings */
  wchar_t def[PBUF]; default_folder(def, PBUF);
  GetPrivateProfileStringW(L"Settings", L"Folder", def, f, PBUF, g_ini);
  if (g_cliFolder[0] && folder_ok(g_cliFolder)) wcscpy(f, g_cliFolder);   /* --folder from Run as admin relaunch */
  if (!folder_ok(f)) wcscpy(f, def);
  SetWindowTextW(g_path, f);
  int iv = ini_int(L"IntervalMinutes", 5), sel = 1;
  for (int i = 0; i < 4; i++) if (g_intervals[i] == iv) sel = i;
  SendMessageW(g_interval, CB_SETCURSEL, sel, 0);
  int mode = ini_int(L"AutoSort", MODE_TYPE);
  SendMessageW(mode == MODE_EXT ? g_rext : g_rtype, BM_SETCHECK, BST_CHECKED, 0);
  SendMessageW(g_tray, BM_SETCHECK, ini_int(L"MinimizeToTray", 1) ? BST_CHECKED : BST_UNCHECKED, 0);
  SendMessageW(g_auto, BM_SETCHECK, ini_int(L"AutoMode", 0) ? BST_CHECKED : BST_UNCHECKED, 0);
  int st = startup_is_on();
  SendMessageW(g_startup, BM_SETCHECK, st ? BST_CHECKED : BST_UNCHECKED, 0);
  if (st) startup_set(1);   /* refresh path in case the exe was moved */
  log_line(L"Welcome, Darshan! Pick a folder and press a button.");
  if (g_elevated) log_line(L"Running as Admin: Public Desktop shortcuts can be sorted too.");
  apply_timer();
  if (g_elevated) set_status(L"Ready. (Admin)");
}


/* Owner-drawn buttons: emoji drawn with Segoe UI Emoji so it shows on every Windows 10/11 */
static HFONT g_fontEmoji, g_fontBold;
static void draw_button(const DRAWITEMSTRUCT *di) {
  wchar_t txt[128]; GetWindowTextW(di->hwndItem, txt, 128);
  int primary = di->CtlID == IDC_DATE || di->CtlID == IDC_EXT || di->CtlID == IDC_TYPE;
  int pressed = (di->itemState & ODS_SELECTED) != 0;
  HDC dc = di->hDC; RECT r = di->rcItem;
  FillRect(dc, &r, g_white);
  COLORREF bg = primary ? (pressed ? RGB(80, 10, 26) : RGB(122, 22, 44)) : (pressed ? RGB(222, 222, 226) : RGB(242, 242, 245));
  COLORREF bd = primary ? RGB(212, 166, 62) : RGB(200, 200, 206);
  HBRUSH b = CreateSolidBrush(bg); HPEN p = CreatePen(PS_SOLID, S(1), bd);
  HGDIOBJ ob = SelectObject(dc, b), op = SelectObject(dc, p);
  RoundRect(dc, r.left, r.top, r.right, r.bottom, S(10), S(10));
  SelectObject(dc, ob); SelectObject(dc, op); DeleteObject(b); DeleteObject(p);
  SetBkMode(dc, TRANSPARENT);
  SetTextColor(dc, primary ? RGB(255, 255, 255) : RGB(40, 40, 48));
  wchar_t *sep = wcsstr(txt, L"  "), *label = txt, icon[8] = L"";
  if (sep) { int n = (int)(sep - txt); if (n > 7) n = 7; wcsncpy(icon, txt, n); icon[n] = 0; label = sep + 2; }
  HFONT of = (HFONT)SelectObject(dc, primary ? g_fontBold : g_font);
  SIZE ls, is = { 0, 0 }; GetTextExtentPoint32W(dc, label, (int)wcslen(label), &ls);
  if (icon[0]) { SelectObject(dc, g_fontEmoji); GetTextExtentPoint32W(dc, icon, (int)wcslen(icon), &is); is.cx += S(6); }
  int x = (r.left + r.right - (is.cx + ls.cx)) / 2, off = pressed ? S(1) : 0;
  if (icon[0]) { RECT ir = { x + off, r.top + off, x + is.cx + off, r.bottom + off }; DrawTextW(dc, icon, -1, &ir, DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_NOPREFIX); }
  SelectObject(dc, primary ? g_fontBold : g_font);
  RECT tr = { x + is.cx + off, r.top + off, r.right, r.bottom + off };
  DrawTextW(dc, label, -1, &tr, DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_NOPREFIX);
  SelectObject(dc, of);
  if (di->itemState & ODS_FOCUS) { RECT fr = r; InflateRect(&fr, -S(4), -S(4)); SetTextColor(dc, 0); DrawFocusRect(dc, &fr); }
}

static void paint_header(HDC dc) {
  RECT r; GetClientRect(g_wnd, &r); r.bottom = S(68);
  HBRUSH b = CreateSolidBrush(RGB(110, 18, 38)); FillRect(dc, &r, b); DeleteObject(b);
  RECT line = r; line.top = r.bottom - S(3);
  b = CreateSolidBrush(RGB(212, 166, 62)); FillRect(dc, &line, b); DeleteObject(b);
  HICON ic = (HICON)LoadImageW(g_inst, MAKEINTRESOURCEW(IDI_APP), IMAGE_ICON, S(40), S(40), 0);
  if (ic) { DrawIconEx(dc, S(16), S(13), ic, S(40), S(40), 0, NULL, DI_NORMAL); DestroyIcon(ic); }
  SetBkMode(dc, TRANSPARENT);
  HFONT of = (HFONT)SelectObject(dc, g_fontHead);
  SetTextColor(dc, RGB(255, 255, 255));
  TextOutW(dc, S(66), S(10), APP_NAME, (int)wcslen(APP_NAME));
  if (g_elevated) {
    SIZE ts; GetTextExtentPoint32W(dc, APP_NAME, (int)wcslen(APP_NAME), &ts);
    SelectObject(dc, g_fontSub); SetTextColor(dc, RGB(240, 205, 120));
    TextOutW(dc, S(66) + ts.cx + S(8), S(16), L"(Admin)", 7);
  }
  SelectObject(dc, g_fontSub);
  SetTextColor(dc, RGB(240, 205, 120));
  const wchar_t *sub = L"Keep your Downloads clean \x2022 by Darshan Corporation";
  TextOutW(dc, S(67), S(40), sub, (int)wcslen(sub));
  SelectObject(dc, of);
}

static void undo_last(void) {
  wchar_t label[128] = L"";
  if (!exists(g_undoLog)) { set_status(L"Nothing to undo yet."); log_line(L"Nothing to undo."); return; }
  if (MessageBoxW(g_wnd, L"Move the files from the last sort back where they were?", APP_NAME, MB_YESNO | MB_ICONQUESTION) != IDYES) return;
  int errs = 0; int back = run_undo(1, &errs, label, 128);
  if (back < 0) { set_status(L"Nothing to undo."); return; }
  log_line(L"Undo (%s): %d file%s moved back%s", label, back, back == 1 ? L"" : L"s", errs ? L", some were missing" : L"");
  set_status(L"Undo done: %d file%s moved back.", back, back == 1 ? L"" : L"s");
  if (g_lastDenied > 0) {
    log_line(L"%d file%s could not go back to Public Desktop \x2014 click 'Run as admin', then Undo again",
             g_lastDenied, g_lastDenied == 1 ? L"" : L"s");
    set_status(L"%d moved back. %d need admin: click 'Run as admin', then Undo again.", back, g_lastDenied);
  }
}

/* Relaunch this exe elevated (UAC prompt), passing the current folder, then close this instance */
static void run_as_admin(void) {
  wchar_t f[PBUF], args[PBUF + 32];
  GetWindowTextW(g_path, f, PBUF);
  save_settings();
  size_t l = wcslen(f);
  if (l && f[l-1] == L'\\') _snwprintf(args, PBUF + 32, L"--folder \"%s\\\"", f);   /* "C:\\" keeps the quote */
  else _snwprintf(args, PBUF + 32, L"--folder \"%s\"", f);
  args[PBUF+31] = 0;
  /* release the single-instance mutex first so the elevated copy can start */
  if (g_mutex) { ReleaseMutex(g_mutex); CloseHandle(g_mutex); g_mutex = NULL; }
  SHELLEXECUTEINFOW sei; ZeroMemory(&sei, sizeof sei);
  sei.cbSize = sizeof sei; sei.hwnd = g_wnd; sei.lpVerb = L"runas"; sei.lpFile = g_self;
  sei.lpParameters = args; sei.nShow = SW_SHOWNORMAL; sei.fMask = SEE_MASK_NOASYNC;
  if (ShellExecuteExW(&sei)) { DestroyWindow(g_wnd); return; }
  DWORD e = GetLastError();
  g_mutex = CreateMutexW(NULL, TRUE, MUTEX_NAME);   /* still the running copy */
  if (e == ERROR_CANCELLED) { log_line(L"Admin permission was not given (UAC: No)."); set_status(L"Run as admin cancelled."); }
  else { log_line(L"Could not restart as admin (error %lu).", e); set_status(L"Could not restart as admin."); }
}

static LRESULT CALLBACK wndproc(HWND h, UINT m, WPARAM w, LPARAM l) {
  switch (m) {
  case WM_CREATE: g_wnd = h; build_ui(); return 0;
  case WM_PAINT: { PAINTSTRUCT ps; HDC dc = BeginPaint(h, &ps); paint_header(dc); EndPaint(h, &ps); return 0; }
  case WM_DRAWITEM: draw_button((const DRAWITEMSTRUCT *)l); return TRUE;
  case WM_CTLCOLORSTATIC: {
    HDC dc = (HDC)w; SetBkColor(dc, RGB(255, 255, 255));
    if ((HWND)l == g_status) SetTextColor(dc, RGB(90, 90, 90));
    return (LRESULT)g_white; }
  case WM_COMMAND: {
    int id = LOWORD(w), code = HIWORD(w);
    switch (id) {
    case IDC_BROWSE: browse(); break;
    case IDC_DATE: {
      wchar_t f[PBUF], name[32]; int already = 0;
      if (!get_folder(f)) break;
      if (make_date_folder(f, name, 32, &already)) {
        if (already) { log_line(L"Folder %s already exists", name); set_status(L"Today's folder %s is already there.", name); }
        else { log_line(L"Created folder %s", name); set_status(L"Created folder %s \x2714", name); }
      } else { log_line(L"Could not create folder %s", name); set_status(L"Could not create the folder."); }
      break; }
    case IDC_EXT:  do_sort(MODE_EXT, 0); break;
    case IDC_TYPE: do_sort(MODE_TYPE, 0); break;
    case IDC_UNDO: undo_last(); break;
    case IDC_ADMIN: run_as_admin(); break;
    case IDC_OPEN: { wchar_t f[PBUF]; if (get_folder(f)) ShellExecuteW(h, L"open", f, NULL, NULL, SW_SHOWNORMAL); break; }
    case IDC_AUTO:
      save_settings(); apply_timer();
      if (SendMessageW(g_auto, BM_GETCHECK, 0, 0) == BST_CHECKED) {
        log_line(L"Auto mode ON"); do_sort(SendMessageW(g_rtype, BM_GETCHECK, 0, 0) == BST_CHECKED ? MODE_TYPE : MODE_EXT, 1);
      } else { log_line(L"Auto mode OFF"); set_status(L"Auto mode OFF."); }
      break;
    case IDC_INTERVAL: if (code == CBN_SELCHANGE) { save_settings(); apply_timer(); } break;
    case IDC_R_EXT: case IDC_R_TYPE: case IDC_TRAYCHK: save_settings(); break;
    case IDC_STARTUP: {
      int on = SendMessageW(g_startup, BM_GETCHECK, 0, 0) == BST_CHECKED;
      if (startup_set(on)) { log_line(on ? L"Will start with Windows" : L"Won't start with Windows"); set_status(on ? L"Starts with Windows (opens in the tray when auto mode is on)." : L"Start with Windows turned off."); }
      else { SendMessageW(g_startup, BM_SETCHECK, on ? BST_UNCHECKED : BST_CHECKED, 0); set_status(L"Could not change the startup setting."); }
      break; }
    case IDC_PATH: if (code == EN_KILLFOCUS) save_settings(); break;
    case IDM_OPEN: restore_from_tray(); break;
    case IDM_EXIT: DestroyWindow(h); break;
    }
    return 0; }
  case WM_TIMER:
    if (w == TIMER_AUTO) do_sort(SendMessageW(g_rtype, BM_GETCHECK, 0, 0) == BST_CHECKED ? MODE_TYPE : MODE_EXT, 1);
    return 0;
  case WM_SIZE:
    if (w == SIZE_MINIMIZED && SendMessageW(g_auto, BM_GETCHECK, 0, 0) == BST_CHECKED &&
        SendMessageW(g_tray, BM_GETCHECK, 0, 0) == BST_CHECKED) go_to_tray();
    return 0;
  case WM_TRAY:
    if (l == WM_LBUTTONDBLCLK || l == WM_LBUTTONUP) restore_from_tray();
    else if (l == WM_RBUTTONUP || l == WM_CONTEXTMENU) {
      POINT p; GetCursorPos(&p);
      HMENU mnu = CreatePopupMenu();
      AppendMenuW(mnu, MF_STRING, IDM_OPEN, L"Open");
      AppendMenuW(mnu, MF_SEPARATOR, 0, NULL);
      AppendMenuW(mnu, MF_STRING, IDM_EXIT, L"Exit");
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
/* Command line (no window): --sort ext|type "folder", --date "folder", --undo */
/* ------------------------------------------------------------------ */
static int cli(int argc, wchar_t **argv) {
  if (argc >= 4 && _wcsicmp(argv[1], L"--sort") == 0) {
    int mode = _wcsicmp(argv[2], L"ext") == 0 ? MODE_EXT : MODE_TYPE, errs = 0;
    int r = run_sort(mode, argv[3], 0, &errs);
    return r < 0 ? 1000 : r;
  }
  if (argc >= 3 && _wcsicmp(argv[1], L"--date") == 0) { wchar_t n[32]; int a; return make_date_folder(argv[2], n, 32, &a) ? 0 : 1; }
  if (argc >= 2 && _wcsicmp(argv[1], L"--undo") == 0) { int e = 0; int r = run_undo(0, &e, NULL, 0); return r < 0 ? 1000 : r; }
  return -1;
}

int WINAPI wWinMain(HINSTANCE inst, HINSTANCE prev, PWSTR cmd, int show) {
  (void)prev; (void)cmd;
  g_inst = inst;
  init_paths();
  int argc = 0; wchar_t **argv = CommandLineToArgvW(GetCommandLineW(), &argc);
  int startTray = 0;
  if (argv && argc >= 2) {
    if (_wcsicmp(argv[1], L"--tray") == 0) startTray = 1;
    else if (_wcsicmp(argv[1], L"--folder") == 0) {   /* open the window on this folder (Run as admin relaunch) */
      if (argc >= 3) { norm_path(argv[2], g_cliFolder, PBUF); }
    } else { int r = cli(argc, argv); if (r != -1) { LocalFree(argv); return r; } }
  }
  if (argv) LocalFree(argv);

  g_elevated = is_elevated();
  g_mutex = CreateMutexW(NULL, TRUE, MUTEX_NAME);
  DWORD me = GetLastError();
  /* an elevated copy owns the mutex -> a normal copy gets ACCESS_DENIED: treat as "already running" */
  if (me == ERROR_ALREADY_EXISTS || (!g_mutex && me == ERROR_ACCESS_DENIED)) {
    HWND other = FindWindowW(APP_CLASS, NULL);
    if (other) PostMessageW(other, WM_WAKEUP, 0, 0);
    return 0;
  }
  CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
  INITCOMMONCONTROLSEX icc = { sizeof icc, ICC_STANDARD_CLASSES | ICC_WIN95_CLASSES }; InitCommonControlsEx(&icc);

  HDC sdc = GetDC(NULL); g_dpi = GetDeviceCaps(sdc, LOGPIXELSY); ReleaseDC(NULL, sdc);
  if (g_dpi < 96) g_dpi = 96;
  g_font     = CreateFontW(-MulDiv(10, g_dpi, 72), 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
  g_fontHead = CreateFontW(-MulDiv(17, g_dpi, 72), 0, 0, 0, FW_SEMIBOLD, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
  g_fontSub  = CreateFontW(-MulDiv(9, g_dpi, 72), 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
  g_white = CreateSolidBrush(RGB(255, 255, 255));
  g_fontEmoji = CreateFontW(-MulDiv(11, g_dpi, 72), 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI Emoji");
  g_fontBold  = CreateFontW(-MulDiv(10, g_dpi, 72), 0, 0, 0, FW_SEMIBOLD, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");

  WNDCLASSEXW wc; ZeroMemory(&wc, sizeof wc);
  wc.cbSize = sizeof wc; wc.lpfnWndProc = wndproc; wc.hInstance = inst;
  wc.hIcon = LoadIconW(inst, MAKEINTRESOURCEW(IDI_APP));
  wc.hIconSm = (HICON)LoadImageW(inst, MAKEINTRESOURCEW(IDI_APP), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
  wc.hCursor = LoadCursor(NULL, IDC_ARROW); wc.hbrBackground = g_white; wc.lpszClassName = APP_CLASS;
  RegisterClassExW(&wc);

  DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
  RECT r = { 0, 0, S(CW), S(CH) }; AdjustWindowRect(&r, style, FALSE);
  HWND h = CreateWindowExW(0, APP_CLASS, g_elevated ? APP_NAME L" (Admin)" : APP_NAME, style, CW_USEDEFAULT, CW_USEDEFAULT, r.right - r.left, r.bottom - r.top, NULL, NULL, inst, NULL);
  if (!h) return 1;
  /* let a normal (non-admin) copy wake this window even when this one runs as admin (UIPI) */
  { typedef BOOL (WINAPI *CWMFEx)(HWND, UINT, DWORD, void *);
    CWMFEx f = (CWMFEx)(void *)GetProcAddress(GetModuleHandleW(L"user32.dll"), "ChangeWindowMessageFilterEx");
    if (f) f(h, WM_WAKEUP, 1 /* MSGFLT_ALLOW */, NULL); }

  int autoOn = SendMessageW(g_auto, BM_GETCHECK, 0, 0) == BST_CHECKED;
  int trayOn = SendMessageW(g_tray, BM_GETCHECK, 0, 0) == BST_CHECKED;
  if (startTray && autoOn && trayOn) { g_trayTipShown = 1; g_inTray = 0; go_to_tray(); }
  else { ShowWindow(h, startTray ? SW_SHOWMINNOACTIVE : show); UpdateWindow(h); }

  MSG msg;
  while (GetMessageW(&msg, NULL, 0, 0) > 0) {
    if (!IsDialogMessageW(h, &msg)) { TranslateMessage(&msg); DispatchMessageW(&msg); }
  }
  CoUninitialize();
  if (g_mutex) CloseHandle(g_mutex);
  return 0;
}
