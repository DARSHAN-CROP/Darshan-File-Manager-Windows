/* core.c - Darshan File Manager 2.0 file logic (no UI) */
#include "core.h"
#include <shellapi.h>
#include <shlobj.h>
#include <wincrypt.h>
#include <stdio.h>
#include <stdlib.h>

wchar_t g_settingsDir[PBUF], g_ini[PBUF], g_undoLog[PBUF], g_self[PBUF];
int g_lastDenied = 0;
Rule g_rules[MAX_RULES];
int g_nrules = 0;

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

int cat_count(void) { return NCATS; }
const wchar_t *cat_name(int i) { return (i >= 0 && i < NCATS) ? CATS[i].folder : L"Others"; }

/* ------------------------------------------------------------------ */
/* Helpers                                                             */
/* ------------------------------------------------------------------ */
void wb_add(WBuf *b, const wchar_t *s) {
  size_t l = wcslen(s);
  if (b->len + l + 1 > b->cap) {
    size_t nc = (b->cap ? b->cap * 2 : 8192); while (nc < b->len + l + 1) nc *= 2;
    wchar_t *nb = (wchar_t *)realloc(b->buf, nc * sizeof(wchar_t)); if (!nb) return;
    b->buf = nb; b->cap = nc;
  }
  memcpy(b->buf + b->len, s, l * sizeof(wchar_t)); b->len += l; b->buf[b->len] = 0;
}
void wb_free(WBuf *b) { free(b->buf); b->buf = NULL; b->len = b->cap = 0; }

int exists(const wchar_t *p) { return GetFileAttributesW(p) != INVALID_FILE_ATTRIBUTES; }
int folder_ok(const wchar_t *folder) {
  DWORD a = GetFileAttributesW(folder);
  return a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY);
}
int ends_with_i(const wchar_t *s, const wchar_t *suf) {
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
int get_ext(const wchar_t *name, wchar_t *out, int n) {
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
int compound_ext(const wchar_t *name, wchar_t *out, int n) {
  static const wchar_t *C[] = { L".tar.gz", L".tar.bz2", L".tar.xz", L".tar.zst", L".tar.lz", L".tar.lzma", L".tar.z" };
  size_t len = wcslen(name);
  for (int i = 0; i < (int)(sizeof C / sizeof C[0]); i++) {
    size_t cl = wcslen(C[i]);
    if (len > cl && _wcsicmp(name + len - cl, C[i]) == 0 && (int)cl < n) { wcscpy(out, C[i] + 1); return 1; }
  }
  return 0;
}

int cat_index(const wchar_t *name) {
  wchar_t ext[32], key[40];
  if (!get_ext(name, ext, 32)) return NCATS;
  _snwprintf(key, 40, L".%s.", ext); key[39] = 0;
  CharLowerW(key);
  for (int i = 0; i < NCATS; i++) if (wcsstr(CATS[i].exts, key)) return i;
  return NCATS;
}
const wchar_t *category_for(const wchar_t *name) { return cat_name(cat_index(name)); }

/* dir\name, or dir\name (1).ext, (2)... if taken */
int unique_dest(const wchar_t *dir, const wchar_t *name, wchar_t *out, int n) {
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

int write_utf16_file(const wchar_t *path, const wchar_t *text, size_t len) {
  HANDLE h = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if (h == INVALID_HANDLE_VALUE) return 0;
  DWORD w; WORD bom = 0xFEFF;
  WriteFile(h, &bom, 2, &w, NULL);
  if (len) WriteFile(h, text, (DWORD)(len * sizeof(wchar_t)), &w, NULL);
  CloseHandle(h); return 1;
}

void norm_path(const wchar_t *in, wchar_t *out, int n) {
  wcsncpy(out, in, n - 1); out[n-1] = 0;
  for (wchar_t *p = out; *p; p++) if (*p == L'/') *p = L'\\';
  size_t l = wcslen(out);
  while (l > 3 && out[l-1] == L'\\') out[--l] = 0;
}
int same_path(const wchar_t *a, const wchar_t *b) {
  wchar_t x[PBUF], y[PBUF]; norm_path(a, x, PBUF); norm_path(b, y, PBUF);
  return x[0] && _wcsicmp(x, y) == 0;
}
static int shell_folder(int csidl, wchar_t *out) {
  wchar_t p[MAX_PATH] = L"";
  if (FAILED(SHGetFolderPathW(NULL, csidl, NULL, SHGFP_TYPE_CURRENT, p)) || !p[0]) return 0;
  wcsncpy(out, p, PBUF - 1); out[PBUF-1] = 0; return 1;
}
/* if folder is the user's Desktop, put the all-users (Public) Desktop path in common and return 1 */
int is_user_desktop(const wchar_t *folder, wchar_t *common) {
  wchar_t d[PBUF];
  if (!shell_folder(CSIDL_DESKTOPDIRECTORY, d) || !same_path(folder, d)) return 0;
  if (!shell_folder(CSIDL_COMMON_DESKTOPDIRECTORY, common) || same_path(common, folder) || !folder_ok(common)) return 0;
  return 1;
}

/* create a folder and its missing parents; 1 = it exists now */
int mkdirs(const wchar_t *path) {
  if (folder_ok(path)) return 1;
  wchar_t p[PBUF]; wcsncpy(p, path, PBUF - 1); p[PBUF-1] = 0;
  for (wchar_t *s = p + 3; *s; s++) {
    if (*s == L'\\') { *s = 0; if (!folder_ok(p)) CreateDirectoryW(p, NULL); *s = L'\\'; }
  }
  if (!folder_ok(p)) CreateDirectoryW(p, NULL);
  return folder_ok(p);
}

void format_size(ULONGLONG b, wchar_t *out, int n) {
  if (b < 1024ULL) _snwprintf(out, n, L"%llu B", b);
  else if (b < 1024ULL * 1024) _snwprintf(out, n, L"%.1f KB", b / 1024.0);
  else if (b < 1024ULL * 1024 * 1024) _snwprintf(out, n, L"%.1f MB", b / (1024.0 * 1024));
  else _snwprintf(out, n, L"%.2f GB", b / (1024.0 * 1024 * 1024));
  out[n-1] = 0;
}

void init_paths(void) {
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

/* ------------------------------------------------------------------ */
/* Undo log                                                            */
/* ------------------------------------------------------------------ */
void undo_begin(WBuf *u, const wchar_t *label) { u->buf = NULL; u->len = u->cap = 0; wb_add(u, label); wb_add(u, L"\n"); }
void undo_add(WBuf *u, const wchar_t *src, const wchar_t *dst) {
  if (!u) return;
  wb_add(u, src); wb_add(u, L"\t"); wb_add(u, dst); wb_add(u, L"\n");
}
void undo_commit(WBuf *u, int moved) {
  if (moved > 0) write_utf16_file(g_undoLog, u->buf, u->len);   /* keep the previous log when nothing moved */
  wb_free(u);
}

/* move src into dstDir (created if needed) as name (or name (1)...); logs to undo; 1 = moved */
int move_logged(const wchar_t *src, const wchar_t *dstDir, const wchar_t *name, WBuf *undo, wchar_t *dstOut) {
  wchar_t dst[PBUF];
  if (!mkdirs(dstDir)) return 0;
  if (!unique_dest(dstDir, name, dst, PBUF)) return 0;
  if (!MoveFileExW(src, dst, MOVEFILE_COPY_ALLOWED)) return 0;   /* no REPLACE flag: never overwrites */
  undo_add(undo, src, dst);
  if (dstOut) { wcsncpy(dstOut, dst, PBUF - 1); dstOut[PBUF-1] = 0; }
  return 1;
}

static int path_under(const wchar_t *p, const wchar_t *root) {
  size_t l = wcslen(root);
  return l && wcslen(p) > l && _wcsnicmp(p, root, l) == 0 && p[l] == L'\\';
}

/* returns moved back count, -1 if no log */
int run_undo(int *errors, wchar_t *label, int labeln) {
  HANDLE h = CreateFileW(g_undoLog, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
  if (h == INVALID_HANDLE_VALUE) return -1;
  DWORD size = GetFileSize(h, NULL), rd = 0;
  wchar_t *buf = (wchar_t *)calloc(size / 2 + 2, sizeof(wchar_t));
  if (!buf) { CloseHandle(h); return -1; }
  ReadFile(h, buf, size, &rd, NULL); CloseHandle(h);
  wchar_t *text = buf; if (text[0] == 0xFEFF) text++;
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
    wchar_t target[PBUF], srcDir[PBUF];
    wcsncpy(srcDir, src, PBUF - 1); srcDir[PBUF-1] = 0;
    wchar_t *sb = wcsrchr(srcDir, L'\\'); if (!sb) { errs++; continue; }
    *sb = 0;
    if (exists(src)) {   /* original spot taken again -> name (1).ext */
      if (!unique_dest(srcDir, sb + 1, target, PBUF)) { errs++; continue; }
    } else { wcsncpy(target, src, PBUF - 1); target[PBUF-1] = 0; }
    if (!folder_ok(srcDir)) mkdirs(srcDir);
    if (MoveFileExW(dst, target, MOVEFILE_COPY_ALLOWED)) {
      back++;
      /* remove the folders the action created, if now empty: the direct parent always (old behaviour),
         then further up only while still inside the file's original folder (Year\Month, rule sub-folders) */
      wchar_t parent[PBUF]; wcsncpy(parent, dst, PBUF - 1); parent[PBUF-1] = 0;
      wchar_t *bs = wcsrchr(parent, L'\\');
      if (bs) {
        *bs = 0;
        if (!same_path(parent, srcDir) && RemoveDirectoryW(parent)) {
          while ((bs = wcsrchr(parent, L'\\')) != NULL) {
            *bs = 0;
            if (!path_under(parent, srcDir) || !RemoveDirectoryW(parent)) break;
          }
        }
      }
      continue;
    }
    if (GetLastError() == ERROR_ACCESS_DENIED) g_lastDenied++;   /* e.g. back into Public Desktop without admin */
    errs++;
    undo_add(&keep, src, dst);   /* keep this entry so a second Undo (e.g. as admin) can still put it back */
  }
  if (keep.len > 0) {
    WBuf out = { 0 };
    wb_add(&out, nl > 0 ? lines[0] : L"Sort"); wb_add(&out, L"\n");
    wb_add(&out, keep.buf);
    write_utf16_file(g_undoLog, out.buf, out.len);
    wb_free(&out);
  } else DeleteFileW(g_undoLog);
  wb_free(&keep);
  free(lines); free(buf);
  if (errors) *errors = errs;
  return back;
}

/* ------------------------------------------------------------------ */
/* Custom rules  ([Rules] Count=N, R1=type|pattern|folder)             */
/* ------------------------------------------------------------------ */
void rules_load(void) {
  g_nrules = GetPrivateProfileIntW(L"Rules", L"Count", 0, g_ini);
  if (g_nrules < 0) g_nrules = 0;
  if (g_nrules > MAX_RULES) g_nrules = MAX_RULES;
  int k = 0;
  for (int i = 0; i < g_nrules; i++) {
    wchar_t key[16], v[600];
    _snwprintf(key, 16, L"R%d", i + 1); key[15] = 0;
    GetPrivateProfileStringW(L"Rules", key, L"", v, 600, g_ini);
    wchar_t *a = wcschr(v, L'|'); if (!a) continue;
    wchar_t *b = wcschr(a + 1, L'|'); if (!b) continue;
    *a = 0; *b = 0;
    g_rules[k].type = _wtoi(v) == RULE_NAME ? RULE_NAME : RULE_EXT;
    wcsncpy(g_rules[k].pat, a + 1, 255); g_rules[k].pat[255] = 0;
    wcsncpy(g_rules[k].folder, b + 1, 255); g_rules[k].folder[255] = 0;
    if (rule_valid(g_rules[k].type, g_rules[k].pat, g_rules[k].folder)) k++;
  }
  g_nrules = k;
}
void rules_save(void) {
  wchar_t v[600], key[16];
  WritePrivateProfileStringW(L"Rules", NULL, NULL, g_ini);   /* clear the section */
  _snwprintf(v, 16, L"%d", g_nrules); v[15] = 0;
  WritePrivateProfileStringW(L"Rules", L"Count", v, g_ini);
  for (int i = 0; i < g_nrules; i++) {
    _snwprintf(key, 16, L"R%d", i + 1); key[15] = 0;
    _snwprintf(v, 600, L"%d|%s|%s", g_rules[i].type, g_rules[i].pat, g_rules[i].folder); v[599] = 0;
    WritePrivateProfileStringW(L"Rules", key, v, g_ini);
  }
}
/* folder: relative, no drive, no "..", no forbidden characters; pattern: not empty and no '|' */
int rule_valid(int type, const wchar_t *pat, const wchar_t *folder) {
  (void)type;
  if (!pat[0] || !folder[0] || wcschr(pat, L'|') || wcschr(folder, L'|')) return 0;
  const wchar_t *p = pat; while (*p == L' ' || *p == L',' || *p == L'.' || *p == L';') p++;
  if (!*p) return 0;
  if (folder[0] == L'\\' || folder[0] == L'/' || folder[0] == L' ' || folder[0] == L'.') return 0;
  if (wcsstr(folder, L"..")) return 0;
  for (p = folder; *p; p++) if (wcschr(L":*?\"<>|", *p)) return 0;
  return 1;
}
int rule_match_one(const Rule *r, const wchar_t *name) {
  if (r->type == RULE_NAME) {
    wchar_t a[MAX_PATH], b[256];
    wcsncpy(a, name, MAX_PATH - 1); a[MAX_PATH-1] = 0; wcsncpy(b, r->pat, 255); b[255] = 0;
    CharLowerW(a); CharLowerW(b);
    return wcsstr(a, b) != NULL;
  }
  /* extension list: "jpg, png .gif;webp" ; also matches compound "tar.gz" */
  wchar_t ext[64] = L"", cext[64] = L"";
  int has = get_ext(name, ext, 64); int hasC = compound_ext(name, cext, 64);
  if (!has && !hasC) return 0;
  wchar_t list[256]; wcsncpy(list, r->pat, 255); list[255] = 0;
  wchar_t *p = list;
  while (*p) {
    while (*p && wcschr(L" ,;", *p)) p++;
    wchar_t *t = p;
    while (*p && !wcschr(L" ,;", *p)) p++;
    if (*p) *p++ = 0;
    while (*t == L'.' || *t == L'*') t++;
    if (!*t) continue;
    if ((has && _wcsicmp(t, ext) == 0) || (hasC && _wcsicmp(t, cext) == 0)) return 1;
  }
  return 0;
}
int rules_match(const wchar_t *name, wchar_t *folderOut, int n) {
  for (int i = 0; i < g_nrules; i++) if (rule_match_one(&g_rules[i], name)) {
    if (folderOut) {
      wcsncpy(folderOut, g_rules[i].folder, n - 1); folderOut[n-1] = 0;
      for (wchar_t *p = folderOut; *p; p++) if (*p == L'/') *p = L'\\';
      size_t l = wcslen(folderOut); while (l && (folderOut[l-1] == L'\\' || folderOut[l-1] == L' ')) folderOut[--l] = 0;
    }
    return i;
  }
  return -1;
}

/* ------------------------------------------------------------------ */
/* Sorting                                                             */
/* ------------------------------------------------------------------ */
static const wchar_t *MONTHS[] = { L"January", L"February", L"March", L"April", L"May", L"June", L"July",
                                   L"August", L"September", L"October", L"November", L"December" };
void date_subdir(const FILETIME *mt, wchar_t *out, int n) {
  FILETIME lt; SYSTEMTIME st;
  FileTimeToLocalFileTime(mt, &lt); FileTimeToSystemTime(&lt, &st);
  if (st.wMonth < 1 || st.wMonth > 12) st.wMonth = 1;
  _snwprintf(out, n, L"%04d\\%02d-%s", st.wYear, st.wMonth, MONTHS[st.wMonth - 1]); out[n-1] = 0;
}

static void sort_one(int flags, const wchar_t *scan, const wchar_t *dest, int isCommon, int verbose,
                     WBuf *undo, int *moved, int *errs, int *denied, int *byRules) {
  wchar_t pat[PBUF], src[PBUF], dir[PBUF], dst[PBUF];
  WIN32_FIND_DATAW fd;
  _snwprintf(pat, PBUF, L"%s\\*", scan); pat[PBUF-1] = 0;
  HANDLE h = FindFirstFileW(pat, &fd);
  if (h == INVALID_HANDLE_VALUE) return;
  do {
    _snwprintf(src, PBUF, L"%s\\%s", scan, fd.cFileName); src[PBUF-1] = 0;
    if (should_skip(src, &fd)) continue;
    wchar_t sub[600] = L"", tmp[300];
    int ruled = 0;
    if ((flags & SF_RULES) && rules_match(fd.cFileName, sub, 300) >= 0) ruled = 1;
    if (!ruled) {
      if (flags & SF_EXT) {
        if (compound_ext(fd.cFileName, sub, 64)) CharUpperW(sub);
        else if (get_ext(fd.cFileName, sub, 64)) CharUpperW(sub); else wcscpy(sub, L"NO_EXTENSION");
      } else if (flags & SF_TYPE) {
        wcscpy(sub, category_for(fd.cFileName));
      }
      if (flags & SF_DATE) {
        date_subdir(&fd.ftLastWriteTime, tmp, 300);
        if (sub[0]) { wcscat(sub, L"\\"); } wcscat(sub, tmp);
      }
    }
    if (!sub[0]) continue;                 /* rules-only run and no rule matched */
    _snwprintf(dir, PBUF, L"%s\\%s", dest, sub); dir[PBUF-1] = 0;
    /* a FILE named like one of the folders on the way */
    { wchar_t chk[PBUF]; wcscpy(chk, dir); int blocked = 0;
      for (wchar_t *s = chk + wcslen(dest) + 1; ; s++) {
        if (*s == L'\\' || *s == 0) { wchar_t c = *s; *s = 0;
          DWORD da = GetFileAttributesW(chk);
          if (da != INVALID_FILE_ATTRIBUTES && !(da & FILE_ATTRIBUTE_DIRECTORY)) blocked = 1;
          *s = c; if (!c || blocked) break; }
      }
      if (blocked) { (*errs)++; if (verbose) log_line(L"Skipped %s (a file named like the folder \"%s\" is in the way)", fd.cFileName, sub); continue; }
    }
    int created = !folder_ok(dir);
    if (created && !mkdirs(dir)) { (*errs)++; if (verbose) log_line(L"Could not create folder %s", sub); continue; }
    if (!unique_dest(dir, fd.cFileName, dst, PBUF)) { (*errs)++; continue; }
    if (MoveFileExW(src, dst, MOVEFILE_COPY_ALLOWED)) {
      (*moved)++; if (ruled && byRules) (*byRules)++;
      undo_add(undo, src, dst);
      if (verbose) {
        const wchar_t *dn = wcsrchr(dst, L'\\');
        log_line(L"   %s%s%s  \x2192  %s\\%s", isCommon ? L"[Public Desktop] " : L"", ruled ? L"[Rule] " : L"",
                 fd.cFileName, sub, dn ? dn + 1 : dst);
      }
    } else {
      DWORD e = GetLastError();
      if (created) RemoveDirectoryW(dir);
      if (isCommon && e == ERROR_ACCESS_DENIED) (*denied)++;
      else { (*errs)++; if (verbose) log_line(L"   ! %s could not be moved (open in another app?)", fd.cFileName); }
    }
  } while (FindNextFileW(h, &fd));
  FindClose(h);
}

static const wchar_t *sort_label(int flags) {
  if (flags == SF_RULES) return L"Custom rules";
  if (flags & SF_EXT) return L"Sort by Extension";
  if (flags & SF_TYPE) return L"Sort by Type";
  if (flags & SF_DATE) return L"Sort by Date";
  return L"Sort";
}

/* returns number moved (-1 = folder unreadable); writes its own undo log when undo == NULL */
int run_sort_flags(int flags, const wchar_t *folder, int verbose, int *errors, WBuf *undo, int *byRules) {
  int moved = 0, errs = 0, denied = 0, br = 0;
  wchar_t pat[PBUF], common[PBUF];
  WIN32_FIND_DATAW fd;
  g_lastDenied = 0;
  _snwprintf(pat, PBUF, L"%s\\*", folder); pat[PBUF-1] = 0;
  HANDLE h = FindFirstFileW(pat, &fd);
  if (h == INVALID_HANDLE_VALUE) { if (errors) *errors = 1; return -1; }
  FindClose(h);
  WBuf own; WBuf *u = undo;
  if (!u) { undo_begin(&own, sort_label(flags)); u = &own; }
  sort_one(flags, folder, folder, 0, verbose, u, &moved, &errs, &denied, &br);
  if (is_user_desktop(folder, common))   /* Desktop: also the Public Desktop shortcuts */
    sort_one(flags, common, folder, 1, verbose, u, &moved, &errs, &denied, &br);
  if (!undo) undo_commit(&own, moved);
  g_lastDenied = denied;
  if (errors) *errors = errs;
  if (byRules) *byRules = br;
  return moved;
}

int make_date_folder(const wchar_t *folder, wchar_t *name, int n, int *already) {
  SYSTEMTIME st; GetLocalTime(&st);
  wchar_t p[PBUF];
  _snwprintf(name, n, L"%02d-%02d-%04d", st.wDay, st.wMonth, st.wYear); name[n-1] = 0;
  _snwprintf(p, PBUF, L"%s\\%s", folder, name); p[PBUF-1] = 0;
  *already = 0;
  if (exists(p)) { *already = 1; return 1; }
  return CreateDirectoryW(p, NULL) ? 1 : 0;
}

/* ------------------------------------------------------------------ */
/* File lists and walking                                              */
/* ------------------------------------------------------------------ */
void flist_add(FList *l, const wchar_t *path, ULONGLONG size, FILETIME mt) {
  if (l->n == l->cap) {
    int nc = l->cap ? l->cap * 2 : 256;
    FItem *nv = (FItem *)realloc(l->v, nc * sizeof(FItem)); if (!nv) return;
    l->v = nv; l->cap = nc;
  }
  FItem *it = &l->v[l->n++]; ZeroMemory(it, sizeof *it);
  it->path = _wcsdup(path); it->size = size; it->mt = mt;
}
void flist_free(FList *l) { for (int i = 0; i < l->n; i++) free(l->v[i].path); free(l->v); l->v = NULL; l->n = l->cap = 0; }

static int is_tool_folder(const wchar_t *name) {
  return _wcsicmp(name, L"Duplicates") == 0 || _wcsicmp(name, L"Old Files") == 0;
}
/* collect files (sub: recurse; skipTools: don't enter Duplicates / Old Files) */
static void walk(const wchar_t *dir, int sub, int skipTools, FList *out, int depth) {
  wchar_t pat[PBUF], full[PBUF]; WIN32_FIND_DATAW fd;
  _snwprintf(pat, PBUF, L"%s\\*", dir); pat[PBUF-1] = 0;
  HANDLE h = FindFirstFileW(pat, &fd);
  if (h == INVALID_HANDLE_VALUE) return;
  do {
    if (!wcscmp(fd.cFileName, L".") || !wcscmp(fd.cFileName, L"..")) continue;
    _snwprintf(full, PBUF, L"%s\\%s", dir, fd.cFileName); full[PBUF-1] = 0;
    if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      if (!sub || depth > 32) continue;
      if (fd.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_REPARSE_POINT)) continue;
      if (skipTools && depth == 0 && is_tool_folder(fd.cFileName)) continue;
      walk(full, sub, skipTools, out, depth + 1);
      continue;
    }
    if (should_skip(full, &fd)) continue;
    ULONGLONG size = ((ULONGLONG)fd.nFileSizeHigh << 32) | fd.nFileSizeLow;
    flist_add(out, full, size, fd.ftLastWriteTime);
  } while (FindNextFileW(h, &fd));
  FindClose(h);
}

static const wchar_t *base_name(const wchar_t *p) { const wchar_t *b = wcsrchr(p, L'\\'); return b ? b + 1 : p; }

/* ------------------------------------------------------------------ */
/* Duplicates: same size, then same SHA-1                              */
/* ------------------------------------------------------------------ */
int hash_file(const wchar_t *path, BYTE out[20]) {
  HCRYPTPROV prov = 0; HCRYPTHASH hh = 0; int ok = 0;
  HANDLE f = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
  if (f == INVALID_HANDLE_VALUE) return 0;
  if (CryptAcquireContextW(&prov, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT) &&
      CryptCreateHash(prov, CALG_SHA1, 0, 0, &hh)) {
    static BYTE buf[1 << 16]; DWORD rd; ok = 1;
    while (ReadFile(f, buf, sizeof buf, &rd, NULL) && rd > 0) if (!CryptHashData(hh, buf, rd, 0)) { ok = 0; break; }
    DWORD len = 20;
    if (ok && !CryptGetHashParam(hh, HP_HASHVAL, out, &len, 0)) ok = 0;
  }
  if (hh) CryptDestroyHash(hh);
  if (prov) CryptReleaseContext(prov, 0);
  CloseHandle(f);
  return ok;
}

static int cmp_size(const void *a, const void *b) {
  const FItem *x = (const FItem *)a, *y = (const FItem *)b;
  if (x->size != y->size) return x->size < y->size ? -1 : 1;
  return 0;
}
static int cmp_group(const void *a, const void *b) {
  const FItem *x = (const FItem *)a, *y = (const FItem *)b;
  if (x->group != y->group) return x->group - y->group;
  if (x->keep != y->keep) return y->keep - x->keep;          /* kept file first */
  int c = CompareFileTime(&x->mt, &y->mt); if (c) return c;
  return _wcsicmp(x->path, y->path);
}
/* the file to keep in a group: oldest modified, then shortest name, then path order */
/* names that look like a copy: "x (1).jpg", "x - Copy.pdf", "copy of x" */
static int copyish(const wchar_t *path) {
  wchar_t n[MAX_PATH]; wcsncpy(n, base_name(path), MAX_PATH - 1); n[MAX_PATH-1] = 0; CharLowerW(n);
  wchar_t *dot = wcsrchr(n, L'.'); if (dot && dot != n) *dot = 0;
  if (wcsstr(n, L"copy") || wcsstr(n, L"\x0aa8\x0a95\x0ab2")) return 1;      /* "copy", Gujarati "nakal" */
  size_t l = wcslen(n);
  if (l >= 4 && n[l-1] == L')') { size_t i = l - 2; while (i > 0 && iswdigit(n[i])) i--; if (i < l - 2 && n[i] == L'(' && i > 0 && n[i-1] == L' ') return 1; }
  return 0;
}
static int better_keep(const FItem *a, const FItem *b) {
  int ca = copyish(a->path), cb = copyish(b->path); if (ca != cb) return ca < cb;
  int c = CompareFileTime(&a->mt, &b->mt); if (c) return c < 0;
  size_t la = wcslen(base_name(a->path)), lb = wcslen(base_name(b->path));
  if (la != lb) return la < lb;
  return _wcsicmp(a->path, b->path) < 0;
}

int dup_scan(const wchar_t *folder, int sub, DupResult *r) {
  FList all = { 0 };
  ZeroMemory(r, sizeof *r);
  walk(folder, sub, 1, &all, 0);
  if (all.n > 1) qsort(all.v, all.n, sizeof(FItem), cmp_size);
  int group = 0;
  for (int i = 0; i < all.n; ) {
    int j = i; while (j < all.n && all.v[j].size == all.v[i].size) j++;
    if (j - i >= 2 && all.v[i].size > 0) {
      int *ok = (int *)calloc(j - i, sizeof(int)), *done = (int *)calloc(j - i, sizeof(int));
      for (int k = i; k < j; k++) ok[k - i] = hash_file(all.v[k].path, all.v[k].hash);
      for (int k = i; k < j; k++) {
        if (!ok[k - i] || done[k - i]) continue;
        int members = 1;
        for (int m = k + 1; m < j; m++) if (ok[m - i] && !done[m - i] && !memcmp(all.v[k].hash, all.v[m].hash, 20)) members++;
        if (members < 2) continue;
        group++;
        int best = -1;
        for (int m = k; m < j; m++) if (ok[m - i] && !done[m - i] && (m == k || !memcmp(all.v[k].hash, all.v[m].hash, 20))) {
          done[m - i] = 1;
          FItem *it = &all.v[m];
          flist_add(&r->files, it->path, it->size, it->mt);
          FItem *c = &r->files.v[r->files.n - 1]; memcpy(c->hash, it->hash, 20); c->group = group;
          if (best < 0 || better_keep(c, &r->files.v[best])) best = r->files.n - 1;
        }
        r->files.v[best].keep = 1;
        r->dupes += members - 1;
        r->wasted += (ULONGLONG)(members - 1) * all.v[k].size;
      }
      free(ok); free(done);
    }
    i = j;
  }
  r->groups = group;
  if (r->files.n > 1) qsort(r->files.v, r->files.n, sizeof(FItem), cmp_group);
  flist_free(&all);
  return r->dupes;
}

/* sel[i] = 1 to move file i (NULL = every file not marked keep). A group is never emptied completely. */
int dup_move(const wchar_t *folder, DupResult *r, const int *sel, int verbose, WBuf *undo, int *errs) {
  wchar_t dir[PBUF]; int moved = 0, e = 0;
  _snwprintf(dir, PBUF, L"%s\\Duplicates", folder); dir[PBUF-1] = 0;
  WBuf own; WBuf *u = undo; if (!u) { undo_begin(&own, L"Duplicates"); u = &own; }
  for (int i = 0; i < r->files.n; ) {
    int j = i; while (j < r->files.n && r->files.v[j].group == r->files.v[i].group) j++;
    int staying = 0;
    for (int k = i; k < j; k++) if (!(sel ? sel[k] : !r->files.v[k].keep)) staying++;
    for (int k = i; k < j; k++) {
      int want = sel ? sel[k] : !r->files.v[k].keep;
      if (!want) continue;
      if (staying == 0) { staying = 1; continue; }   /* keep at least one copy */
      wchar_t dst[PBUF];
      if (move_logged(r->files.v[k].path, dir, base_name(r->files.v[k].path), u, dst)) {
        moved++;
        if (verbose) log_line(L"   %s  \x2192  Duplicates\\%s", base_name(r->files.v[k].path), base_name(dst));
      } else { e++; if (verbose) log_line(L"   ! %s could not be moved", base_name(r->files.v[k].path)); }
    }
    i = j;
  }
  if (!undo) undo_commit(&own, moved);
  if (errs) *errs = e;
  return moved;
}
void dup_free(DupResult *r) { flist_free(&r->files); ZeroMemory(r, sizeof *r); }

/* ------------------------------------------------------------------ */
/* Old files: top-level files not modified in N days                   */
/* ------------------------------------------------------------------ */
int old_scan(const wchar_t *folder, int days, FList *out, ULONGLONG *total) {
  FList all = { 0 }; FILETIME nowft; ULARGE_INTEGER now, t;
  ZeroMemory(out, sizeof *out);
  GetSystemTimeAsFileTime(&nowft); now.LowPart = nowft.dwLowDateTime; now.HighPart = nowft.dwHighDateTime;
  ULONGLONG limit = (ULONGLONG)(days < 1 ? 1 : days) * 864000000000ULL;   /* 100-ns ticks per day */
  walk(folder, 0, 1, &all, 0);
  ULONGLONG tot = 0;
  for (int i = 0; i < all.n; i++) {
    t.LowPart = all.v[i].mt.dwLowDateTime; t.HighPart = all.v[i].mt.dwHighDateTime;
    if (now.QuadPart > t.QuadPart && now.QuadPart - t.QuadPart >= limit) {
      flist_add(out, all.v[i].path, all.v[i].size, all.v[i].mt); tot += all.v[i].size;
    }
  }
  flist_free(&all);
  if (total) *total = tot;
  return out->n;
}
int old_move(const wchar_t *folder, FList *l, int verbose, WBuf *undo, int *errs) {
  wchar_t dir[PBUF]; int moved = 0, e = 0;
  _snwprintf(dir, PBUF, L"%s\\Old Files", folder); dir[PBUF-1] = 0;
  WBuf own; WBuf *u = undo; if (!u) { undo_begin(&own, L"Old files"); u = &own; }
  for (int i = 0; i < l->n; i++) {
    if (!exists(l->v[i].path)) { e++; continue; }
    wchar_t dst[PBUF];
    if (move_logged(l->v[i].path, dir, base_name(l->v[i].path), u, dst)) {
      moved++; if (verbose) log_line(L"   %s  \x2192  Old Files\\%s", base_name(l->v[i].path), base_name(dst));
    } else { e++; if (verbose) log_line(L"   ! %s could not be moved", base_name(l->v[i].path)); }
  }
  if (!undo) undo_commit(&own, moved);
  if (errs) *errs = e;
  return moved;
}

/* ------------------------------------------------------------------ */
/* Batch rename                                                        */
/* ------------------------------------------------------------------ */
static int icontains(const wchar_t *s, const wchar_t *sub) {
  if (!sub[0]) return 1;
  size_t n = wcslen(sub);
  for (; *s; s++) if (_wcsnicmp(s, sub, n) == 0) return 1;
  return 0;
}
int rename_filter(const wchar_t *name, const RenameOpts *o) {
  if (!icontains(name, o->contains)) return 0;
  if (o->ext[0]) {
    wchar_t e[64]; const wchar_t *want = o->ext; while (*want == L'.' || *want == L'*') want++;
    if (!get_ext(name, e, 64) || _wcsicmp(e, want) != 0) return 0;
  }
  return 1;
}
/* new name for one file: [prefix]( base+number | stem with find->replace )[suffix].ext */
void rename_one(const wchar_t *name, const RenameOpts *o, int index, wchar_t *out, int n) {
  wchar_t stem[MAX_PATH], ext[64] = L"", body[MAX_PATH * 2];
  wcsncpy(stem, name, MAX_PATH - 1); stem[MAX_PATH-1] = 0;
  if (get_ext(name, ext, 64)) stem[wcslen(stem) - wcslen(ext) - 1] = 0;
  if (o->numbering) {
    int d = o->digits < 1 ? 1 : (o->digits > 9 ? 9 : o->digits);
    _snwprintf(body, MAX_PATH * 2, L"%s%0*d", o->base, d, o->start + index);
  } else if (o->find[0]) {
    body[0] = 0; size_t fl = wcslen(o->find); const wchar_t *s = stem;
    while (*s) {
      if (_wcsnicmp(s, o->find, fl) == 0) { wcsncat(body, o->repl, MAX_PATH * 2 - wcslen(body) - 1); s += fl; }
      else { size_t l = wcslen(body); if (l < MAX_PATH * 2 - 1) { body[l] = *s; body[l+1] = 0; } s++; }
    }
  } else wcscpy(body, stem);
  body[MAX_PATH * 2 - 1] = 0;
  _snwprintf(out, n, L"%s%s%s%s%s", o->prefix, body, o->suffix, ext[0] ? L"." : L"", ext); out[n-1] = 0;
  /* characters Windows does not allow in names */
  for (wchar_t *p = out; *p; p++) if (wcschr(L"\\/:*?\"<>|", *p)) *p = L'_';
}
static int cmp_wstr(const void *a, const void *b) { return _wcsicmp(*(wchar_t * const *)a, *(wchar_t * const *)b); }
int rename_plan(const wchar_t *folder, const RenameOpts *o, RenamePlan *p) {
  FList all = { 0 };
  ZeroMemory(p, sizeof *p);
  walk(folder, 0, 1, &all, 0);
  wchar_t **names = (wchar_t **)calloc(all.n + 1, sizeof(wchar_t *)); int nn = 0;
  for (int i = 0; i < all.n; i++) if (rename_filter(base_name(all.v[i].path), o)) names[nn++] = _wcsdup(base_name(all.v[i].path));
  if (nn > 1) qsort(names, nn, sizeof(wchar_t *), cmp_wstr);
  p->cap = nn + 1; p->from = (wchar_t **)calloc(p->cap, sizeof(wchar_t *)); p->to = (wchar_t **)calloc(p->cap, sizeof(wchar_t *));
  for (int i = 0; i < nn; i++) {
    wchar_t nw[MAX_PATH * 2];
    rename_one(names[i], o, i, nw, MAX_PATH * 2);
    if (!nw[0] || _wcsicmp(nw, L".") == 0) { free(names[i]); names[i] = NULL; continue; }
    if (wcscmp(nw, names[i]) == 0) { free(names[i]); names[i] = NULL; continue; }  /* nothing changes */
    /* two files in this batch would get the same name: add (2), (3)... */
    wchar_t cand[MAX_PATH * 2]; wcscpy(cand, nw);
    for (int k = 2; ; k++) {
      int clash = 0;
      for (int q = 0; q < p->n; q++) if (_wcsicmp(p->to[q], cand) == 0) { clash = 1; break; }
      if (!clash) {   /* a file already there that is not itself being renamed */
        wchar_t full[PBUF]; _snwprintf(full, PBUF, L"%s\\%s", folder, cand); full[PBUF-1] = 0;
        if (exists(full)) {
          int moving = 0;
          for (int q = 0; q < nn; q++) if (names[q] && _wcsicmp(names[q], cand) == 0) { moving = 1; break; }
          if (!moving) clash = 1;
        }
      }
      if (!clash) break;
      wchar_t e2[64] = L""; wchar_t st2[MAX_PATH * 2]; wcscpy(st2, nw);
      if (get_ext(nw, e2, 64)) st2[wcslen(st2) - wcslen(e2) - 1] = 0;
      _snwprintf(cand, MAX_PATH * 2, L"%s (%d)%s%s", st2, k, e2[0] ? L"." : L"", e2); cand[MAX_PATH*2-1] = 0;
    }
    p->from[p->n] = names[i]; p->to[p->n] = _wcsdup(cand); p->n++;
  }
  free(names); flist_free(&all);
  return p->n;
}
/* two passes through temporary names so A->B, B->A swaps and chains never overwrite anything */
int rename_apply(const wchar_t *folder, RenamePlan *p, int verbose, WBuf *undo, int *errs) {
  int done = 0, e = 0;
  WBuf own; WBuf *u = undo; if (!u) { undo_begin(&own, L"Batch rename"); u = &own; }
  wchar_t **tmp = (wchar_t **)calloc(p->n + 1, sizeof(wchar_t *));
  for (int i = 0; i < p->n; i++) {
    wchar_t a[PBUF], t[PBUF];
    _snwprintf(a, PBUF, L"%s\\%s", folder, p->from[i]); a[PBUF-1] = 0;
    _snwprintf(t, PBUF, L"%s\\~dfm_rename_%d.tmp", folder, i); t[PBUF-1] = 0;
    if (exists(a) && !exists(t) && MoveFileExW(a, t, 0)) tmp[i] = _wcsdup(t); else e++;
  }
  for (int i = 0; i < p->n; i++) {
    if (!tmp[i]) continue;
    wchar_t a[PBUF], dst[PBUF];
    _snwprintf(a, PBUF, L"%s\\%s", folder, p->from[i]); a[PBUF-1] = 0;
    if (unique_dest(folder, p->to[i], dst, PBUF) && MoveFileExW(tmp[i], dst, 0)) {
      done++; undo_add(u, a, dst);
      if (verbose) log_line(L"   %s  \x2192  %s", p->from[i], base_name(dst));
    } else { MoveFileExW(tmp[i], a, 0); e++; }
    free(tmp[i]);
  }
  free(tmp);
  if (!undo) undo_commit(&own, done);
  if (errs) *errs = e;
  return done;
}
void rename_free(RenamePlan *p) {
  for (int i = 0; i < p->n; i++) { free(p->from[i]); free(p->to[i]); }
  free(p->from); free(p->to); ZeroMemory(p, sizeof *p);
}

/* ------------------------------------------------------------------ */
/* Storage usage per category                                          */
/* ------------------------------------------------------------------ */
int stats_scan(const wchar_t *folder, int sub, CatStat out[NCATS_MAX], ULONGLONG *total) {
  FList all = { 0 }; ULONGLONG tot = 0;
  ZeroMemory(out, sizeof(CatStat) * NCATS_MAX);
  walk(folder, sub, 0, &all, 0);
  for (int i = 0; i < all.n; i++) {
    int c = cat_index(base_name(all.v[i].path));
    out[c].count++; out[c].bytes += all.v[i].size; tot += all.v[i].size;
  }
  int n = all.n; flist_free(&all);
  if (total) *total = tot;
  return n;
}

/* ------------------------------------------------------------------ */
/* Auto mode pipeline: duplicates -> old files -> rules + sort          */
/* ------------------------------------------------------------------ */
int auto_run(const wchar_t *folder, const AutoCfg *c, int verbose, AutoSummary *s) {
  WBuf u; int e = 0;
  ZeroMemory(s, sizeof *s);
  if (!folder_ok(folder)) return -1;
  undo_begin(&u, L"Auto mode");
  if (c->dupes) {
    DupResult r; dup_scan(folder, 1, &r);
    if (r.dupes) s->dupes = dup_move(folder, &r, NULL, verbose, &u, &e), s->errs += e;
    dup_free(&r);
  }
  if (c->old) {
    FList l; ULONGLONG t;
    if (old_scan(folder, c->oldDays, &l, &t) > 0) s->old = old_move(folder, &l, verbose, &u, &e), s->errs += e;
    flist_free(&l);
  }
  int flags = (c->rules ? SF_RULES : 0) | (c->sort ? (c->sortMode == SF_EXT ? SF_EXT : SF_TYPE) : 0) | (c->date ? SF_DATE : 0);
  if (flags) {
    int m = run_sort_flags(flags, folder, verbose, &e, &u, &s->byRules);
    if (m > 0) s->sorted = m;
    s->errs += e;
  }
  undo_commit(&u, s->sorted + s->dupes + s->old);
  return s->sorted + s->dupes + s->old;
}
