# Changelog

All notable changes to **Darshan File Manager (Windows)** are documented here.
The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project uses [Semantic Versioning](https://semver.org/).

## [2.0] - 2026-11-04
### Added
- **Themes:** Light, Dark and System (follows Windows, including the dark title bar). Saved in settings.
- **App colour:** Maroon, Gold, Blue, Green, Purple or Orange for buttons, header, tabs and highlights.
- **Language:** English / ગુજરાતી for the whole app (Nirmala UI font for Gujarati).
- **Tabs:** Sort, Duplicates, Old files, Rename, Rules, Storage, Settings.
- **Duplicate finder:** same size + SHA-1, optional sub-folders, groups with the space you can free; moves the copies to `Duplicates` (keeps the original, never deletes).
- **My rules:** "extension is …" / "name contains …" → any folder, applied before the built-in types (first match wins).
- **Old files cleaner:** move files not modified in 30/60/90/180/365/custom days to `Old Files`, with a preview.
- **Sort by Date:** `Year\MM-Month` folders by modified date.
- **Batch rename:** filter + prefix / suffix / find → replace / numbering (`Photo_001`), preview first, never overwrites.
- **Storage usage:** count and size per category as a bar chart.
- **Auto mode choices:** tick my rules, sort by type/extension, year/month folders, old files, duplicates; tray balloon with a summary ("12 sorted, 3 duplicates moved, 5 old files moved").
- **Command line:** `--sort`, `--dupes`, `--old`, `--rename`, `--stats`, `--rule-add/list/clear`, `--auto`, `--undo`.

### Changed
- Undo works for every action, including renames and nested folders it created.
- New flat, theme-aware look: owner-drawn buttons, checkboxes, combo boxes, lists and frames.
- Source split into `main.c` (window), `core.c` (file logic) and `lang.c` (strings).

## [1.1] - 2026-11-03
### Added
- **Run as admin** button that relaunches the app elevated for the selected folder.
- Sorting your Desktop now also sorts shortcuts on the Public (all-users) Desktop.
- "(Admin)" label in the title bar when running elevated.

### Changed
- Undo now restores files to both your Desktop and the Public Desktop.
- One clear message when shortcuts need admin rights, instead of one line per file.

## [1.0] - 2026-11-03
### Added
- Sort by Type (14 categories, 400+ extensions), Sort by Extension and Date Folder.
- Undo last sort.
- Auto mode (every 1, 5, 15 or 30 minutes), Minimize to tray and Start with Windows.
- Activity log, duplicate-safe file names and system-folder warnings.

[2.0]: https://github.com/DARSHAN-CROP/Darshan-File-Manager-Windows/releases/tag/v2.0
[1.1]: https://github.com/DARSHAN-CROP/Darshan-File-Manager-Windows/releases/tag/v1.1
