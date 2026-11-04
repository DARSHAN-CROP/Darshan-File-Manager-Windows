<p align="center">
  <img src="screenshots/icon.png" width="110" alt="Darshan File Manager icon">
</p>

<h1 align="center">Darshan File Manager</h1>

<p align="center">
  A tiny, fast and portable Windows app that keeps your Downloads, Desktop or any folder clean — sort by type, extension or date, find duplicates, clean old files, batch-rename, see what takes space, and let Auto mode do it all in the background. Light & dark themes, 6 app colours, English & ગુજરાતી.
</p>

<p align="center">
  <img alt="Platform" src="https://img.shields.io/badge/platform-Windows%207%20%7C%2010%20%7C%2011-0078D6?logo=windows">
  <img alt="Language" src="https://img.shields.io/badge/language-C%20(Win32)-A8B9CC?logo=c">
  <img alt="Size" src="https://img.shields.io/badge/exe-~180%20KB-success">
  <img alt="License" src="https://img.shields.io/badge/license-MIT-blue">
  <img alt="Version" src="https://img.shields.io/badge/version-2.0-orange">
  <img alt="Theme" src="https://img.shields.io/badge/theme-light%20%7C%20dark-7B1E3A">
</p>

<p align="center">
  <img src="screenshots/light_sort.png" width="420" alt="Light theme">
  <img src="screenshots/dark_sort.png" width="420" alt="Dark theme">
</p>

---

## ⬇️ Download

### 👉 [**Download DarshanFileManager.exe (v2.0)**](https://github.com/DARSHAN-CROP/Darshan-File-Manager-Windows/raw/main/release/DarshanFileManager.exe)

Double-click it — no installation needed. If Windows SmartScreen shows *"Windows protected your PC"*, click **More info → Run anyway** (the app is not code-signed).

---

## ✨ Features

| | Feature | What it does |
|---|---|---|
| 🗂 | **Sort by Type** | Moves files into smart category folders — `Images`, `Videos`, `Music`, `PDF`, `Documents`, `Ebooks`, `Archives`, `Programs`, `Code`, `Fonts`, `Design`, `Contacts_Calendar`, `Torrents`, `Others`. Knows **400+ file extensions**. |
| 🔤 | **Sort by Extension** | One folder per extension — `JPG`, `PDF`, `MP4`, … (handles `.tar.gz`). |
| 📅 | **Sort by Date** 🆕 | `2026\11-November` folders by the file's modified date. |
| 📁 | **Date Folder** | Creates today's `DD-MM-YYYY` folder. |
| 🧩 | **My rules** 🆕 | Your own rules: *extension is* `jpg, png` or *name contains* `invoice` → any folder (e.g. `Bills\Invoices`). Run before the built-in types; first match wins. Add / edit / delete in the app. |
| 👯 | **Duplicate finder** 🆕 | Finds files with exactly the same content (size + SHA-1), optionally in sub-folders. Shows groups and the space you can free; tick what to move to a `Duplicates` folder. The original is kept, nothing is ever deleted. |
| 🧹 | **Old files cleaner** 🆕 | Moves files not changed in 30 / 60 / 90 / 180 / 365 / any number of days into `Old Files`. Preview first. |
| ✏️ | **Batch rename** 🆕 | Filter by *name contains* / extension, then prefix, suffix, find → replace or numbering (`Photo_001`). Preview before applying; never overwrites. |
| 📊 | **Storage usage** 🆕 | Count and size per category as a bar chart. |
| ↩ | **Undo** | Every action (sort, rules, duplicates, old files, rename, auto mode) can be undone with one click. |
| ⏱ | **Auto mode** | Every 1, 5, 15 or 30 minutes. 🆕 Tick what it does: my rules, sort by type/extension, year/month folders, old files, duplicates. Tray balloon with a summary, e.g. *"12 sorted, 3 duplicates moved, 5 old files moved"*. |
| 🎨 | **Themes & colours** 🆕 | Light / Dark / System (follows Windows, incl. dark title bar). App colour: Maroon, Gold, Blue, Green, Purple, Orange. |
| 🌐 | **English / ગુજરાતી** 🆕 | Whole app in Gujarati with one click (Nirmala UI font). |
| 🖥 | **Start with Windows** · 🔽 **Tray** · 🛡 **Run as admin** | Runs quietly in the tray; admin mode also sorts the all-users Desktop shortcuts. |
| 📜 | **Activity log** | Shows exactly which file went where. |

### Safe by design
- Never overwrites — duplicate names become `file (1).jpg`, `file (2).jpg`, …
- Nothing is ever deleted: duplicates and old files are *moved* to folders, and Undo puts them back.
- Skips files that are still downloading (`.crdownload`, `.part`, `.tmp`, …) and system files.
- Warns before touching system/root folders; Auto mode never touches them.
- Single instance, no installer, no internet, no telemetry.

---

## 📸 Screenshots

| Light | Dark |
|---|---|
| <img src="screenshots/light_sort.png" width="400"> | <img src="screenshots/dark_sort.png" width="400"> |
| <img src="screenshots/light_duplicates.png" width="400"> | <img src="screenshots/dark_duplicates.png" width="400"> |
| <img src="screenshots/light_oldfiles.png" width="400"> | <img src="screenshots/dark_oldfiles.png" width="400"> |
| <img src="screenshots/light_rename.png" width="400"> | <img src="screenshots/dark_rename.png" width="400"> |
| <img src="screenshots/light_rules.png" width="400"> | <img src="screenshots/dark_rules.png" width="400"> |
| <img src="screenshots/light_storage.png" width="400"> | <img src="screenshots/dark_storage.png" width="400"> |
| <img src="screenshots/light_settings.png" width="400"> | <img src="screenshots/dark_settings.png" width="400"> |

**ગુજરાતી**

<img src="screenshots/gujarati_light_sort.png" width="400"> <img src="screenshots/gujarati_dark_storage.png" width="400">

**App colours**

<img src="screenshots/accent_gold_light_sort.png" width="400"> <img src="screenshots/accent_blue_dark_duplicates.png" width="400">
<img src="screenshots/accent_green_light_storage.png" width="400"> <img src="screenshots/accent_purple_dark_settings.png" width="400">

**Auto mode result**

<img src="screenshots/light_auto_mode_result.png" width="400">

<sub>Screenshots were taken under Wine on Linux; on Windows the buttons also show their colour emoji icons.</sub>

---

## 🛠 Build from source

Plain Win32 C — no frameworks, no dependencies.

### On Windows (MSYS2 / MinGW-w64)
```bash
cd src
windres app.rc -O coff -o app.res
gcc -municode -mwindows -static -O2 -s main.c core.c lang.c app.res -o DarshanFileManager.exe -lcomctl32 -lshell32 -lole32 -luuid -lgdi32 -ladvapi32
```

### On Linux (cross-compile)
```bash
sudo apt install mingw-w64
cd src
x86_64-w64-mingw32-windres app.rc -O coff -o app.res
x86_64-w64-mingw32-gcc -municode -mwindows -static -O2 -s main.c core.c lang.c app.res -o DarshanFileManager.exe -lcomctl32 -lshell32 -lole32 -luuid -lgdi32 -ladvapi32
```

### Command line (for scripts and tests)
```
DarshanFileManager.exe --sort type|ext|date|rules "C:\Folder" [--rules] [--date]
DarshanFileManager.exe --dupes "C:\Folder" [--sub] [--move]
DarshanFileManager.exe --old "C:\Folder" 90 [--move]
DarshanFileManager.exe --rename "C:\Folder" --ext jpg --number Photo_ 1 3 [--apply]
DarshanFileManager.exe --stats "C:\Folder" [--sub]
DarshanFileManager.exe --rule-add ext "jpg, png" "Photos"   |  --rule-list  |  --rule-clear
DarshanFileManager.exe --auto "C:\Folder" [--with rules,type,ext,date,old,dupes] [--days 90]
DarshanFileManager.exe --undo
```

---

## 📂 Project structure

```
src/
├── main.c        # window: tabs, theme, owner-drawn controls, tray, auto mode, command line
├── core.c/.h     # file logic: sorting, rules, duplicates, old files, rename, storage, undo
├── lang.c/.h     # every UI string in English and Gujarati
├── app.rc        # icon, manifest, version info (2.0)
├── app.manifest  # DPI-aware, modern controls
├── app.ico
└── resource.h
release/DarshanFileManager.exe
screenshots/
```

Settings (theme, colour, language, rules, auto-mode ticks) are saved in `%APPDATA%\DarshanFileManager\settings.ini`; the undo log is `last_sort_undo.log` in the same folder.

📱 **Android version:** [Darshan File Manager for Android](https://github.com/DARSHAN-CROP/Darshan-File-Manager-Android)

---

## 👤 Author

**Darshankumar Savsani** — *Darshan Corporation*
Diploma in Computer Engineering, Atmiya University, Rajkot

- Organization: [DARSHAN CROP](https://github.com/DARSHAN-CROP)
- GitHub: [@1darshankumarsavsani-blip](https://github.com/1darshankumarsavsani-blip)
- Email: 1darshankumarsavsani@gmail.com

If you find this useful, please ⭐ star the repo!

## 📄 License

Released under the [MIT License](LICENSE).
