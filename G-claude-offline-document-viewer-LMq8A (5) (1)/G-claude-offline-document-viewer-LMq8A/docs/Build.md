# Build Instructions — DocVision

## Prerequisites

- **Windows 10 SDK** (10.0.19041.0 or later)
- **Visual Studio 2022** (Community or higher) with C++ Desktop Development workload
- **CMake 3.20+**
- **vcpkg** (recommended for dependency management)

## Dependencies

| Library | vcpkg package | Required |
|---------|--------------|----------|
| MuPDF | `mupdf` | Yes (PDF) |
| QPDF | `qpdf` | Yes (PDF) |
| DjVuLibre | `djvulibre` | Yes (DjVu) |
| Tesseract | `tesseract` | Yes (OCR) |
| Leptonica | `leptonica` | Yes (OCR) |
| nlohmann/json | `nlohmann-json` | Yes |
| SQLite3 | `sqlite3` | Yes |
| minizip-ng | `minizip-ng` | Yes (CBZ/EPUB) |
| stb | `stb` | Yes (images) |
| WebView2 | `webview2` | Yes (EPUB) |

## Build Steps

### 1. Install vcpkg (if not installed)

```powershell
git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
C:\vcpkg\bootstrap-vcpkg.bat
```

### 2. Install dependencies

```powershell
vcpkg install mupdf:x64-windows qpdf:x64-windows djvulibre:x64-windows ^
    tesseract:x64-windows leptonica:x64-windows nlohmann-json:x64-windows ^
    sqlite3:x64-windows minizip-ng:x64-windows stb:x64-windows ^
    webview2:x64-windows
```

### 3. Configure and build

```powershell
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64 ^
    -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake
cmake --build . --config Release
```

### 4. Run

```powershell
.\Release\DocVision.exe
```

## Build without optional features

Disable specific format support if needed:

```powershell
cmake .. -DDOCVISION_ENABLE_DJVU=OFF -DDOCVISION_ENABLE_OCR=OFF
```

## Building the Installer

DocVision uses **Inno Setup** for creating Windows installers.
Inno Setup is free, creates a single `.exe`, and works on Windows 10/11.

### Prerequisites for building installer

1. Install [Inno Setup 6](https://jrsoftware.org/isdl.php) (free)
2. Build DocVision in Release mode first

### Option A: Using build script (recommended)

```powershell
# Build Release + Inno Setup installer + Portable ZIP
.\scripts\build.ps1 -All

# Or just the installer
.\scripts\build.ps1 -Installer

# Or just the portable ZIP
.\scripts\build.ps1 -Portable
```

Output: `build\installer\DocVision-0.1.0-Setup-x64.exe`

### Option B: Using Inno Setup directly

```powershell
# Build Release first
cmake --build build --config Release

# Compile installer
iscc installer\docvision.iss
```

### Option C: Using CPack

```powershell
cd build
cmake --build . --config Release
cpack -G INNOSETUP   # Inno Setup installer (.exe)
cpack -G ZIP          # Portable ZIP
```

### Installer features

- Modern Windows installer UI (supports English + Russian)
- User-selectable file associations (PDF, DjVu, CBZ, EPUB)
- Desktop and Start Menu shortcuts
- Tesseract OCR data (optional)
- Clean uninstall from Control Panel
- 64-bit only, requires Windows 10+

## Portable distribution

Create a portable package using the build script:

```powershell
.\scripts\build.ps1 -Portable
```

Or manually:

```powershell
mkdir DocVision-portable
copy build\Release\DocVision.exe DocVision-portable\
xcopy /s config DocVision-portable\config\
xcopy /s tessdata DocVision-portable\tessdata\
mkdir DocVision-portable\logs
mkdir DocVision-portable\cache
echo "Portable" > DocVision-portable\portable.txt
```

The presence of `portable.txt` marks the installation as portable (all data stored locally).

## Tesseract language data

Download trained data files for OCR:
- Place `.traineddata` files in the `tessdata/` directory
- Russian: `rus.traineddata`
- English: `eng.traineddata`

Download from: https://github.com/tesseract-ocr/tessdata_best

## Troubleshooting

- **WebView2 Runtime not found**: Install Microsoft Edge WebView2 Runtime
- **DPI issues**: Ensure the application manifest is properly embedded (check build output)
- **Missing DLLs**: Ensure all vcpkg DLLs are in the same directory as the executable
