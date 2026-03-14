# DocVision — Offline Document Viewer

Offline, portable, multi-format document viewer and annotator for Windows 10/11.

## Features

- **Multi-format**: PDF, DjVu, CBZ (comics), EPUB
- **Offline-only**: zero network requests, no telemetry
- **Portable**: all data stored next to executable, no registry writes
- **Annotations**: highlights, underline, strikethrough, freehand ink, sticky notes, stamps
- **OCR**: built-in Tesseract OCR for scanned documents
- **Tabbed interface**: open multiple documents simultaneously
- **Search**: full-text search with OCR index support
- **Print**: Windows printing with preview
- **Themes**: light, dark, night-inversion modes
- **HiDPI**: Per-Monitor DPI Awareness v2
- **Command palette**: quick access to all commands (Ctrl+Shift+P)

## Quick Start

### Build from source

```powershell
# Option 1: Build script
.\scripts\build.ps1

# Option 2: Manual CMake
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

### Create installer

```powershell
# NSIS installer + portable ZIP
.\scripts\build.ps1 -All

# Or just the installer
.\scripts\build.ps1 -Installer

# Or just portable ZIP
.\scripts\build.ps1 -Portable
```

### Using vcpkg for dependencies

```powershell
set VCPKG_ROOT=C:\vcpkg
.\scripts\build.ps1
```

See [docs/Build.md](docs/Build.md) for detailed build instructions.

## Project Structure

```
DocVision/
├── include/          # Headers (core, ui, formats, annotations, ocr, ...)
├── src/              # Implementation
├── resources/        # App manifest, RC file, default configs
├── installer/        # NSIS installer script
├── scripts/          # Build scripts (bat, ps1)
├── docs/             # Architecture docs, build guide
├── CMakeLists.txt    # Build system
└── vcpkg.json        # Dependency manifest
```

## Architecture

See [docs/Architecture.md](docs/Architecture.md) for the full architecture overview.

## License

AGPL-3.0 — see [LICENSE](LICENSE)

Third-party libraries are listed in the LICENSE file with their respective licenses.
