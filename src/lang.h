/* lang.h - Darshan File Manager 2.0 UI strings: English / ગુજરાતી */
#ifndef DFM_LANG_H
#define DFM_LANG_H
#include <wchar.h>

#define STRINGS(X) \
 X(S_NONE, L"", L"") \
 X(S_APPSUB, L"Keep your folders clean  \x2022  by Darshan Corporation", L"તમારા ફોલ્ડર સ્વચ્છ રાખો  \x2022  Darshan Corporation") \
 X(S_FOLDER, L"Folder to organize", L"ગોઠવવાનું ફોલ્ડર") \
 X(S_BROWSE, L"Browse...", L"પસંદ કરો...") \
 X(S_TAB_SORT, L"Sort", L"ગોઠવો") \
 X(S_TAB_DUP, L"Duplicates", L"ડુપ્લિકેટ") \
 X(S_TAB_OLD, L"Old files", L"જૂની ફાઇલો") \
 X(S_TAB_REN, L"Rename", L"નામ બદલો") \
 X(S_TAB_RULES, L"Rules", L"નિયમો") \
 X(S_TAB_STO, L"Storage", L"સ્ટોરેજ") \
 X(S_TAB_SET, L"Settings", L"સેટિંગ્સ") \
 X(S_ACTIONS, L"Actions", L"ક્રિયાઓ") \
 X(S_DATEFOLDER, L"Date Folder", L"તારીખ ફોલ્ડર") \
 X(S_SORTEXT, L"Sort by Extension", L"એક્સટેન્શન પ્રમાણે") \
 X(S_SORTTYPE, L"Sort by Type", L"પ્રકાર પ્રમાણે") \
 X(S_SORTDATE, L"Sort by Date", L"તારીખ પ્રમાણે") \
 X(S_UNDO, L"Undo last action", L"છેલ્લું કામ પાછું") \
 X(S_UNDO_SHORT, L"Undo", L"પાછું") \
 X(S_OPENFOLDER, L"Open folder", L"ફોલ્ડર ખોલો") \
 X(S_RUNADMIN, L"Run as admin", L"Admin તરીકે") \
 X(S_AUTOMODE, L"Auto mode", L"ઓટો મોડ") \
 X(S_EVERY, L"every", L"દર") \
 X(S_MINUTES, L"minutes", L"મિનિટે") \
 X(S_AUTORUNS, L"Auto mode does:", L"ઓટો મોડમાં શું થાય:") \
 X(S_A_RULES, L"My rules (first)", L"મારા નિયમો (પહેલા)") \
 X(S_A_SORT, L"Sort by", L"ગોઠવો:") \
 X(S_TYPE, L"Type", L"પ્રકાર") \
 X(S_EXTENSION, L"Extension", L"એક્સટેન્શન") \
 X(S_A_DATE, L"Year / Month folders", L"વર્ષ / મહિનો ફોલ્ડર") \
 X(S_A_OLD, L"Old files (%d+ days)", L"જૂની ફાઇલો (%d+ દિવસ)") \
 X(S_A_DUP, L"Duplicates", L"ડુપ્લિકેટ") \
 X(S_TRAY, L"Minimize to tray", L"ટ્રેમાં રાખો") \
 X(S_STARTUP, L"Start with Windows", L"Windows સાથે શરૂ કરો") \
 X(S_DUP_TITLE, L"Duplicate file finder", L"ડુપ્લિકેટ ફાઇલ શોધક") \
 X(S_SUBFOLDERS, L"Include subfolders", L"સબફોલ્ડર પણ") \
 X(S_SCAN, L"Scan", L"શોધો") \
 X(S_MOVEDUP, L"Move to Duplicates", L"Duplicates માં ખસેડો") \
 X(S_DUP_HINT, L"Finds files with exactly the same content (size + SHA-1).", L"એકદમ સરખી ફાઇલો શોધે છે (સાઇઝ + SHA-1).") \
 X(S_DUP_SUM, L"%d groups  \x2022  %d duplicates  \x2022  %s can be freed", L"%d જૂથ  \x2022  %d ડુપ્લિકેટ  \x2022  %s જગ્યા બચશે") \
 X(S_DUP_NONE, L"No duplicates found \x2714", L"કોઈ ડુપ્લિકેટ નથી \x2714") \
 X(S_COL_GROUP, L"Group", L"જૂથ") \
 X(S_COL_FILE, L"File", L"ફાઇલ") \
 X(S_COL_SIZE, L"Size", L"સાઇઝ") \
 X(S_COL_MOD, L"Modified", L"છેલ્લો ફેરફાર") \
 X(S_COL_ACTION, L"Action", L"શું થશે") \
 X(S_KEEP, L"Keep", L"રાખો") \
 X(S_MOVE, L"Move", L"ખસેડો") \
 X(S_OLD_TITLE, L"Old files cleaner", L"જૂની ફાઇલો સાફ કરો") \
 X(S_OLD_NOTMOD, L"Not modified in", L"આટલા દિવસથી ન બદલાયેલી") \
 X(S_DAYS, L"days", L"દિવસ") \
 X(S_CUSTOM, L"Custom", L"બીજું") \
 X(S_PREVIEW, L"Preview", L"જુઓ") \
 X(S_MOVEOLD, L"Move to Old Files", L"Old Files માં ખસેડો") \
 X(S_OLD_SUM, L"%d files older than %d days  \x2022  %s", L"%d ફાઇલો %d દિવસથી જૂની  \x2022  %s") \
 X(S_OLD_HINT, L"Press Preview to see which files will move.", L"કઈ ફાઇલો ખસશે તે જોવા 'જુઓ' દબાવો.") \
 X(S_REN_TITLE, L"Batch rename", L"એકસાથે નામ બદલો") \
 X(S_REN_CONTAINS, L"Name contains", L"નામમાં છે") \
 X(S_REN_EXT, L"Extension", L"એક્સટેન્શન") \
 X(S_REN_PREFIX, L"Prefix", L"આગળ ઉમેરો") \
 X(S_REN_SUFFIX, L"Suffix", L"પાછળ ઉમેરો") \
 X(S_REN_FIND, L"Find", L"શોધો") \
 X(S_REN_REPLACE, L"Replace with", L"બદલે મૂકો") \
 X(S_REN_NUMBER, L"Numbering", L"ક્રમાંક") \
 X(S_REN_START, L"Start", L"શરૂ") \
 X(S_REN_DIGITS, L"Digits", L"અંક") \
 X(S_APPLY, L"Rename", L"નામ બદલો") \
 X(S_REN_SUM, L"%d files will be renamed", L"%d ફાઇલોના નામ બદલાશે") \
 X(S_REN_HINT, L"Set a filter and a pattern, then press Preview.", L"ફિલ્ટર અને પેટર્ન લખો, પછી 'જુઓ' દબાવો.") \
 X(S_COL_OLDNAME, L"Current name", L"હાલનું નામ") \
 X(S_COL_NEWNAME, L"New name", L"નવું નામ") \
 X(S_RULES_TITLE, L"My rules", L"મારા નિયમો") \
 X(S_USE_RULES, L"Use my rules when sorting", L"ગોઠવતી વખતે મારા નિયમો વાપરો") \
 X(S_R_EXT, L"Extension is", L"એક્સટેન્શન છે") \
 X(S_R_NAME, L"Name contains", L"નામમાં છે") \
 X(S_COL_NUM, L"#", L"#") \
 X(S_COL_IF, L"If", L"જો") \
 X(S_COL_VALUE, L"Value", L"કિંમત") \
 X(S_COL_FOLDER, L"Move to folder", L"આ ફોલ્ડરમાં") \
 X(S_ADD, L"Add rule", L"નિયમ ઉમેરો") \
 X(S_UPDATE, L"Update", L"સુધારો") \
 X(S_DELETE, L"Delete", L"કાઢો") \
 X(S_RULES_HINT, L"Rules run before the built-in types. First match wins.", L"નિયમો પહેલા લાગુ થાય છે. પહેલો મેળ જીતે.") \
 X(S_RULE_LBL, L"If the file", L"જો ફાઇલનું") \
 X(S_VALUE_LBL, L"Value (e.g. jpg, png)", L"કિંમત (દા.ત. jpg, png)") \
 X(S_FOLDER_LBL, L"Folder (e.g. Photos)", L"ફોલ્ડર (દા.ત. Photos)") \
 X(S_RULE_BAD, L"Type a value and a folder name (no : * ? \" < > |).", L"કિંમત અને ફોલ્ડરનું નામ લખો (: * ? \" < > | વગર).") \
 X(S_RULE_FULL, L"You can have up to 64 rules.", L"વધુમાં વધુ 64 નિયમ.") \
 X(S_RULE_SAVED, L"Rules saved: %d", L"નિયમો સચવાયા: %d") \
 X(S_STO_TITLE, L"Storage usage", L"સ્ટોરેજ વપરાશ") \
 X(S_ANALYZE, L"Analyze", L"તપાસો") \
 X(S_STO_SUM, L"%d files  \x2022  %s total", L"%d ફાઇલો  \x2022  કુલ %s") \
 X(S_STO_HINT, L"Press Analyze to see what takes the space.", L"શું જગ્યા રોકે છે તે જોવા 'તપાસો' દબાવો.") \
 X(S_FILES, L"files", L"ફાઇલો") \
 X(S_APPEARANCE, L"Appearance", L"દેખાવ") \
 X(S_THEME, L"Theme", L"થીમ") \
 X(S_LIGHT, L"Light", L"લાઇટ") \
 X(S_DARK, L"Dark", L"ડાર્ક") \
 X(S_SYSTEM, L"System (follow Windows)", L"સિસ્ટમ (Windows પ્રમાણે)") \
 X(S_COLOR, L"App color", L"એપ રંગ") \
 X(S_LANGUAGE, L"Language", L"ભાષા") \
 X(S_C_MAROON, L"Maroon", L"મરૂન") \
 X(S_C_GOLD, L"Gold", L"સોનેરી") \
 X(S_C_BLUE, L"Blue", L"વાદળી") \
 X(S_C_GREEN, L"Green", L"લીલો") \
 X(S_C_PURPLE, L"Purple", L"જાંબલી") \
 X(S_C_ORANGE, L"Orange", L"નારંગી") \
 X(S_ABOUT, L"About", L"વિશે") \
 X(S_ABOUT1, L"Darshan File Manager 2.0  \x2022  by Darshan Corporation", L"Darshan File Manager 2.0  \x2022  Darshan Corporation") \
 X(S_ABOUT2, L"Your choices are saved automatically.", L"તમારી પસંદગી આપોઆપ સચવાય છે.") \
 X(S_ACTIVITY, L"Activity", L"પ્રવૃત્તિ") \
 X(S_READY, L"Ready.", L"તૈયાર.") \
 X(S_WELCOME, L"Welcome, Darshan! Pick a folder and choose an action.", L"સ્વાગત છે, દર્શન! ફોલ્ડર પસંદ કરો અને કામ પસંદ કરો.") \
 X(S_ST_MOVED, L"%d files moved. Use Undo if you didn't mean it.", L"%d ફાઇલો ખસેડી. ભૂલથી થયું હોય તો 'પાછું' દબાવો.") \
 X(S_ST_CLEAN, L"Folder is already clean \x2714", L"ફોલ્ડર પહેલેથી સ્વચ્છ છે \x2714") \
 X(S_ST_ERRS, L"%d moved, %d could not be moved (open in another app?).", L"%d ખસેડી, %d ન ખસી (બીજી એપમાં ખુલ્લી છે?).") \
 X(S_NOFOLDER, L"That folder doesn't exist. Use Browse... to pick one.", L"એ ફોલ્ડર મળ્યું નહીં. 'પસંદ કરો...' થી પસંદ કરો.") \
 X(S_NOTHING_UNDO, L"Nothing to undo yet.", L"પાછું કરવા જેવું કંઈ નથી.") \
 X(S_UNDO_Q, L"Put back the files from the last action (%s)?", L"છેલ્લા કામ (%s) ની ફાઇલો પાછી મૂકી દઉં?") \
 X(S_UNDO_DONE, L"Undo done: %d files put back.", L"પાછું થઈ ગયું: %d ફાઇલો પાછી મૂકી.") \
 X(S_RISKY, L"This looks like a system or root folder. Changing it can break programs.\n\nContinue anyway?", L"આ system અથવા root ફોલ્ડર લાગે છે. તેમાં ફેરફારથી programs બગડી શકે.\n\nતો પણ ચાલુ રાખું?") \
 X(S_DATE_CREATED, L"Created folder %s \x2714", L"%s ફોલ્ડર બન્યું \x2714") \
 X(S_DATE_EXISTS, L"Today's folder %s is already there.", L"આજનું %s ફોલ્ડર પહેલેથી છે.") \
 X(S_AUTO_ON, L"Auto mode ON: every %d minutes while the app is running.", L"ઓટો મોડ ચાલુ: એપ ચાલે ત્યાં સુધી દર %d મિનિટે.") \
 X(S_AUTO_OFF, L"Auto mode OFF.", L"ઓટો મોડ બંધ.") \
 X(S_AUTO_NOTHING, L"Auto mode: checked at %s, nothing new.", L"ઓટો મોડ: %s વાગ્યે તપાસ્યું, કંઈ નવું નથી.") \
 X(S_AUTO_SKIP, L"Auto mode skipped: system folder selected.", L"ઓટો મોડ અટક્યો: system ફોલ્ડર પસંદ છે.") \
 X(S_SUM_SORTED, L"%d sorted", L"%d ગોઠવી") \
 X(S_SUM_DUP, L"%d duplicates moved", L"%d ડુપ્લિકેટ ખસેડી") \
 X(S_SUM_OLD, L"%d old files moved", L"%d જૂની ફાઇલો ખસેડી") \
 X(S_DUP_MOVED, L"%d duplicates moved to the Duplicates folder.", L"%d ડુપ્લિકેટ Duplicates ફોલ્ડરમાં ખસેડી.") \
 X(S_OLD_MOVED, L"%d old files moved to Old Files.", L"%d જૂની ફાઇલો Old Files માં ખસેડી.") \
 X(S_REN_DONE, L"%d files renamed.", L"%d ફાઇલોના નામ બદલ્યા.") \
 X(S_SCANNING, L"Working...", L"કામ ચાલુ છે...") \
 X(S_NOSEL, L"Nothing to do. Scan or Preview first.", L"કરવા જેવું કંઈ નથી. પહેલા 'શોધો' કે 'જુઓ' દબાવો.") \
 X(S_CONFIRM_DUP, L"Move %d duplicate files to the \"Duplicates\" folder?\n\nNothing is deleted, and Undo puts them back.", L"%d ડુપ્લિકેટ ફાઇલો \"Duplicates\" ફોલ્ડરમાં ખસેડું?\n\nકંઈ delete નહીં થાય, 'પાછું' થી પાછી આવશે.") \
 X(S_CONFIRM_OLD, L"Move %d files to the \"Old Files\" folder?\n\nUndo puts them back.", L"%d ફાઇલો \"Old Files\" ફોલ્ડરમાં ખસેડું?\n\n'પાછું' થી પાછી આવશે.") \
 X(S_CONFIRM_REN, L"Rename %d files?\n\nUndo puts the old names back.", L"%d ફાઇલોના નામ બદલું?\n\n'પાછું' થી જૂના નામ પાછા આવશે.") \
 X(S_TRAY_TIP, L"Still running in auto mode. Double-click the icon to open.", L"ઓટો મોડમાં ચાલુ છે. ખોલવા icon પર double-click કરો.") \
 X(S_TRAY_OPEN, L"Open", L"ખોલો") \
 X(S_TRAY_EXIT, L"Exit", L"બંધ કરો") \
 X(S_ADMIN_NEED, L"%d moved. %d Public Desktop shortcuts need admin: click 'Run as admin'.", L"%d ખસેડી. %d Public Desktop shortcut માટે 'Admin તરીકે' દબાવો.") \
 X(S_FOLDER_SEL, L"Folder selected.", L"ફોલ્ડર પસંદ થયું.") \
 X(S_BROWSE_TITLE, L"Choose the folder to organize", L"ગોઠવવાનું ફોલ્ડર પસંદ કરો") \
 X(S_THEME_SET, L"Theme: %s  \x2022  Color: %s", L"થીમ: %s  \x2022  રંગ: %s")

#define X_ENUM(id, en, gu) id,
enum StrId { STRINGS(X_ENUM) S_COUNT };
#undef X_ENUM

extern int g_lang;   /* 0 = English, 1 = Gujarati */
const wchar_t *L_(int id);
const wchar_t *L_en(int id);
#endif
