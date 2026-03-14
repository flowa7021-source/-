# Architecture — DocVision Offline Document Viewer

## Overview

DocVision is an offline, portable, multi-format document viewer and annotator for Windows 10–11.
It follows a PDFGear-inspired UX model (ribbon + Home-center, intuitive reading modes, integrated OCR/screenshot/search tools) while supporting PDF, DjVu, CBZ, and EPUB formats.

## Design Principles

1. **Offline-only** — zero network requests (no telemetry, updates, AI, CDN)
2. **Portable** — configuration stored next to exe, no registry writes
3. **Modular** — format plugins, swappable engines, clean interfaces
4. **Performance** — lazy render, page cache with memory budget, progressive display
5. **HiDPI-aware** — Per-Monitor DPI Awareness v2

## High-Level Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                        Application Shell                     │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌───────────────┐  │
│  │  Ribbon   │ │  Tabs    │ │ Side     │ │  Status Bar   │  │
│  │  (Home+)  │ │ Manager  │ │ Panel    │ │ (page/zoom)   │  │
│  └──────────┘ └──────────┘ └──────────┘ └───────────────┘  │
├─────────────────────────────────────────────────────────────┤
│                        UI Layer                              │
│  CommandManager · HotkeyManager · ThemeManager               │
├─────────────────────────────────────────────────────────────┤
│                        Core Layer                            │
│  ┌─────────────┐ ┌──────────┐ ┌───────────┐ ┌───────────┐  │
│  │  Document    │ │ Render   │ │ Selection │ │ Search    │  │
│  │  Abstraction │ │ Pipeline │ │ Manager   │ │ Engine    │  │
│  └─────────────┘ └──────────┘ └───────────┘ └───────────┘  │
├─────────────────────────────────────────────────────────────┤
│                     Format Plugins                           │
│  ┌──────┐  ┌───────┐  ┌──────┐  ┌───────┐                  │
│  │ PDF  │  │ DjVu  │  │ CBZ  │  │ EPUB  │                  │
│  └──────┘  └───────┘  └──────┘  └───────┘                  │
├─────────────────────────────────────────────────────────────┤
│                     Subsystems                               │
│  ┌────────────┐ ┌──────┐ ┌───────┐ ┌────────┐ ┌─────────┐  │
│  │ Annotation │ │ OCR  │ │ Print │ │Library │ │ Diag    │  │
│  │ System     │ │      │ │       │ │/Shelf  │ │ nostics │  │
│  └────────────┘ └──────┘ └───────┘ └────────┘ └─────────┘  │
└─────────────────────────────────────────────────────────────┘
```

## Module Descriptions

### Application Shell (`src/ui/`)
- **MainWindow** — top-level HWND, manages ribbon, tab bar, side panel, status bar
- **RibbonController** — Windows Ribbon Framework integration; Home tab concentrates all tools
- **TabManager** — multi-document tabs with drag reorder, close, session restore
- **SidePanel** — switchable panels: Thumbnails, Outline/TOC, Annotations list, Search results
- **StatusBar** — page indicator, zoom slider, view mode selector

### UI Layer (`src/ui/`)
- **CommandManager** — command routing, undo/redo stack
- **HotkeyManager** — configurable keyboard shortcuts, conflict detection, locale-aware
- **ThemeManager** — light/dark/night-inversion themes
- **CommandPalette** — quick command access (Ctrl+Shift+P)

### Core Layer (`src/core/`)
- **IDocument** — abstract interface for all document formats
- **RenderPipeline** — progressive rendering (draft → final), post-processing (brightness/contrast/gamma)
- **PageCache** — LRU cache with configurable memory budget (default 300MB), automatic eviction
- **SelectionManager** — text selection (native) and area selection (rectangle)
- **ViewportManager** — zoom, fit modes, scroll position, DPI scaling
- **NavigationHistory** — back/forward navigation stack

### Format Plugins (`src/formats/`)
Each plugin implements `IDocument` and `IPageRenderer`:

- **PDFPlugin** — uses MuPDF (AGPL) for render/text/annotations/save; QPDF for structural operations
- **DjVuPlugin** — uses DjVuLibre for decoding, outline, text layer extraction
- **CBZPlugin** — ZIP extraction (minizip/libzip), image decoding (stb_image/libjpeg-turbo/libpng)
- **EPUBPlugin** — ZIP extraction + WebView2 for HTML/CSS rendering, blocked external URLs

### Annotation Subsystem (`src/annotations/`)
- **AnnotationManager** — unified API across formats
- **Types**: StickyNote, Highlight, Underline, Strikethrough, Ink, FreeText, Stamp, Shape
- **Persistence**: native PDF annotations (MuPDF), sidecar JSON for DjVu/CBZ/EPUB
- **Lock/Freeze**: Locked flag (no delete/move), LockedContents flag (no content edit)
- **Flatten**: burn annotations into page content for permanent embedding
- **Visibility toggle**: show/hide all annotations

### OCR Subsystem (`src/ocr/`)
- **TesseractEngine** — Tesseract 5.x with LSTM models, Apache 2.0
- **OCR Pipeline**: render page → preprocess (deskew, denoise, binarize) → OCR → postprocess
- **Modes**: Area OCR, Page OCR, Document OCR (background task with progress)
- **Index**: OCR results cached by document content hash, stored as sidecar
- **"Make Searchable"**: embed text layer in PDF or store in sidecar index

### Search Engine (`src/search/`)
- **TextSearch** — native text search for PDF/EPUB
- **OCRSearch** — search through OCR index for scanned documents
- **SearchPanel** — results list with page numbers, match highlighting, navigation

### Print Subsystem (`src/print/`)
- **PrintManager** — Windows Print API integration
- **Options**: page range, with/without annotations, copies, scaling
- **Preview**: print preview with accurate rendering

### Library/Shelf (`src/library/`)
- **LibraryManager** — document catalog stored in portable SQLite database
- **Features**: recent files, pinned documents, folder import, tags, collections
- **Search**: full-text search over metadata and OCR indexes
- **Session**: save/restore open tabs and positions

### Diagnostics (`src/diagnostics/`)
- **Logger** — file-based logging with levels (debug/info/warn/error), rotatable
- **CrashHandler** — MiniDumpWriteDump from watchdog process
- **NetworkBlocker** — debug-mode monitor that asserts zero network activity

## Threading Model

```
Main Thread (UI)          — all Win32 messages, ribbon, painting
Render Thread (per doc)   — page rendering via format engine
OCR Thread Pool           — background OCR tasks
I/O Thread                — file loading, library indexing
```

Format engine calls are serialized per-document (engines like PDFium/MuPDF are not thread-safe).
Rendered bitmaps are posted to UI thread via custom messages.

## Data Storage Layout

```
DocVision/
├── DocVision.exe
├── config/
│   ├── settings.json        — app settings
│   ├── hotkeys.json          — custom hotkey bindings
│   ├── library.db            — SQLite library/shelf database
│   └── session.json          — last session state (open tabs, positions)
├── cache/
│   └── ocr/                  — OCR index cache (by content hash)
├── tessdata/                 — Tesseract language models
├── logs/
│   ├── docvision.log         — application log
│   └── crash/                — minidump files
└── plugins/                  — future extensibility
```

## DPI Strategy

1. Application manifest declares `<dpiAwareness>PerMonitorV2</dpiAwareness>`
2. Handle `WM_DPICHANGED` to resize window and re-render at new DPI
3. All UI elements use DPI-scaled coordinates
4. Rendered pages use device-pixel resolution for sharpness

## Performance Targets

| Metric | Target |
|--------|--------|
| First page display | < 1 second |
| Page navigation latency | < 200 ms |
| Memory per document | < 300 MB |
| Continuous scroll | 60 FPS on 500+ page docs |
| OCR (single page) | < 5 seconds |

## Error Handling Strategy

- Corrupted files: attempt partial loading, show friendly error with option to try simplified mode
- Engine errors: log + fallback render (blank page with error message)
- Memory pressure: aggressive cache eviction, degrade to lower-res renders
- Crash: watchdog process writes minidump, user-facing message with dump location
