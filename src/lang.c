/* lang.c - string table */
#include "lang.h"
int g_lang = 0;
#define X_EN(id, en, gu) en,
#define X_GU(id, en, gu) gu,
static const wchar_t *EN[] = { STRINGS(X_EN) };
static const wchar_t *GU[] = { STRINGS(X_GU) };
const wchar_t *L_(int id) { if (id < 0 || id >= S_COUNT) return L""; return g_lang == 1 ? GU[id] : EN[id]; }
const wchar_t *L_en(int id) { if (id < 0 || id >= S_COUNT) return L""; return EN[id]; }
