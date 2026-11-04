/* core.h - Darshan File Manager 2.0: file logic shared by the window and the command line */
#ifndef DFM_CORE_H
#define DFM_CORE_H
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#include <windows.h>
#include <wchar.h>

#define PBUF 2048

/* implemented by the front end (window log or console) */
void log_line(const wchar_t *fmt, ...);

extern wchar_t g_settingsDir[PBUF], g_ini[PBUF], g_undoLog[PBUF], g_self[PBUF];
extern int g_lastDenied;

/* ---------- growable wide string ---------- */
typedef struct { wchar_t *buf; size_t len, cap; } WBuf;
void wb_add(WBuf *b, const wchar_t *s);
void wb_free(WBuf *b);

/* ---------- helpers ---------- */
int  exists(const wchar_t *p);
int  folder_ok(const wchar_t *folder);
int  ends_with_i(const wchar_t *s, const wchar_t *suf);
int  get_ext(const wchar_t *name, wchar_t *out, int n);
int  compound_ext(const wchar_t *name, wchar_t *out, int n);
const wchar_t *category_for(const wchar_t *name);
int  unique_dest(const wchar_t *dir, const wchar_t *name, wchar_t *out, int n);
int  write_utf16_file(const wchar_t *path, const wchar_t *text, size_t len);
void norm_path(const wchar_t *in, wchar_t *out, int n);
int  same_path(const wchar_t *a, const wchar_t *b);
int  is_user_desktop(const wchar_t *folder, wchar_t *common);
int  mkdirs(const wchar_t *path);
void format_size(ULONGLONG b, wchar_t *out, int n);
void init_paths(void);

/* categories */
#define NCATS_MAX 16
int  cat_count(void);                 /* built-in categories (without Others) */
const wchar_t *cat_name(int i);       /* i == cat_count() -> "Others" */
int  cat_index(const wchar_t *name);  /* index of the category for this file name */

/* ---------- undo log: first line = label, then src<TAB>dst ---------- */
void undo_begin(WBuf *u, const wchar_t *label);
void undo_add(WBuf *u, const wchar_t *src, const wchar_t *dst);
void undo_commit(WBuf *u, int moved);  /* writes the log only when something moved; frees */
int  run_undo(int *errors, wchar_t *label, int labeln);
int  move_logged(const wchar_t *src, const wchar_t *dstDir, const wchar_t *name, WBuf *undo, wchar_t *dstOut);

/* ---------- custom rules ---------- */
#define MAX_RULES 64
enum { RULE_EXT = 0, RULE_NAME = 1 };
typedef struct { int type; wchar_t pat[256]; wchar_t folder[256]; } Rule;
extern Rule g_rules[MAX_RULES];
extern int g_nrules;
void rules_load(void);
void rules_save(void);
int  rule_valid(int type, const wchar_t *pat, const wchar_t *folder);
int  rule_match_one(const Rule *r, const wchar_t *name);
int  rules_match(const wchar_t *name, wchar_t *folderOut, int n);  /* index of first matching rule, -1 */

/* ---------- sorting ---------- */
#define SF_RULES 1   /* custom rules first */
#define SF_TYPE  2   /* category folders */
#define SF_EXT   4   /* one folder per extension */
#define SF_DATE  8   /* Year\MM-Month (inside the category when combined) */
void date_subdir(const FILETIME *mt, wchar_t *out, int n);
int  run_sort_flags(int flags, const wchar_t *folder, int verbose, int *errors, WBuf *undo, int *byRules);
int  make_date_folder(const wchar_t *folder, wchar_t *name, int n, int *already);

/* ---------- file lists ---------- */
typedef struct { wchar_t *path; ULONGLONG size; FILETIME mt; int group; int keep; BYTE hash[20]; } FItem;
typedef struct { FItem *v; int n, cap; } FList;
void flist_add(FList *l, const wchar_t *path, ULONGLONG size, FILETIME mt);
void flist_free(FList *l);

/* ---------- duplicates ---------- */
typedef struct { FList files; int groups, dupes; ULONGLONG wasted; } DupResult;
int  hash_file(const wchar_t *path, BYTE out[20]);
int  dup_scan(const wchar_t *folder, int sub, DupResult *r);   /* only files in groups of 2+ are kept, sorted by group */
int  dup_move(const wchar_t *folder, DupResult *r, const int *sel, int verbose, WBuf *undo, int *errs);
void dup_free(DupResult *r);

/* ---------- old files ---------- */
int  old_scan(const wchar_t *folder, int days, FList *out, ULONGLONG *total);
int  old_move(const wchar_t *folder, FList *l, int verbose, WBuf *undo, int *errs);

/* ---------- batch rename ---------- */
typedef struct {
  wchar_t contains[128], ext[64], prefix[128], suffix[128], find[128], repl[128], base[128];
  int numbering, start, digits;
} RenameOpts;
typedef struct { wchar_t **from, **to; int n, cap; } RenamePlan;
int  rename_filter(const wchar_t *name, const RenameOpts *o);
void rename_one(const wchar_t *name, const RenameOpts *o, int index, wchar_t *out, int n);
int  rename_plan(const wchar_t *folder, const RenameOpts *o, RenamePlan *p);
int  rename_apply(const wchar_t *folder, RenamePlan *p, int verbose, WBuf *undo, int *errs);
void rename_free(RenamePlan *p);

/* ---------- storage ---------- */
typedef struct { int count; ULONGLONG bytes; } CatStat;
int  stats_scan(const wchar_t *folder, int sub, CatStat out[NCATS_MAX], ULONGLONG *total);

/* ---------- auto mode ---------- */
typedef struct { int rules, sort, sortMode, date, old, oldDays, dupes; } AutoCfg;   /* sortMode: SF_TYPE or SF_EXT */
typedef struct { int sorted, byRules, dupes, old, errs; } AutoSummary;
int  auto_run(const wchar_t *folder, const AutoCfg *c, int verbose, AutoSummary *s);

#endif
