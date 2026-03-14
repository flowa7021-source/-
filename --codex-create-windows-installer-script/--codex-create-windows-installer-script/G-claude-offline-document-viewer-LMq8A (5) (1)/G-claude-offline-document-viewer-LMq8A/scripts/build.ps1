# DocVision Build Script (PowerShell)
# Usage:
#   .\scripts\build.ps1                    # Release build
#   .\scripts\build.ps1 -Config Debug      # Debug build
#   .\scripts\build.ps1 -Clean             # Clean build directory
#   .\scripts\build.ps1 -Installer         # Build Inno Setup installer
#   .\scripts\build.ps1 -Portable          # Create portable ZIP
#   .\scripts\build.ps1 -All               # Build + Installer + Portable

param(
    [ValidateSet("Release", "Debug")]
    [string]$Config = "Release",
    [switch]$Clean,
    [switch]$Installer,
    [switch]$Portable,
    [switch]$All
)

$ErrorActionPreference = "Stop"
$ProductName = "DocVision"
$ProductVersion = "0.1.0"
$BuildDir = "build"
$Generator = "Visual Studio 17 2022"

Write-Host "============================================" -ForegroundColor Cyan
Write-Host "  $ProductName Build Script v$ProductVersion" -ForegroundColor Cyan
Write-Host "============================================" -ForegroundColor Cyan
Write-Host ""

# Clean
if ($Clean) {
    Write-Host "Cleaning build directory..." -ForegroundColor Yellow
    if (Test-Path $BuildDir) { Remove-Item -Recurse -Force $BuildDir }
    Write-Host "Done." -ForegroundColor Green
    exit 0
}

# Check prerequisites
if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    Write-Host "ERROR: CMake not found. Install CMake 3.20+ and add to PATH." -ForegroundColor Red
    exit 1
}

# vcpkg toolchain
$toolchain = ""
if ($env:VCPKG_ROOT) {
    Write-Host "Using vcpkg from: $env:VCPKG_ROOT" -ForegroundColor Gray
    $toolchain = "-DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake"
} else {
    Write-Host "WARNING: VCPKG_ROOT not set." -ForegroundColor Yellow
}

# Configure
Write-Host ""
Write-Host "[1/3] Configuring CMake ($Config)..." -ForegroundColor Cyan
$cmakeArgs = @("-S", ".", "-B", $BuildDir, "-G", $Generator, "-A", "x64",
               "-DCMAKE_BUILD_TYPE=$Config")
if ($toolchain) { $cmakeArgs += $toolchain }

& cmake @cmakeArgs
if ($LASTEXITCODE -ne 0) { Write-Host "CMake configuration failed." -ForegroundColor Red; exit 1 }

# Build
Write-Host ""
Write-Host "[2/3] Building..." -ForegroundColor Cyan
& cmake --build $BuildDir --config $Config --parallel
if ($LASTEXITCODE -ne 0) { Write-Host "Build failed." -ForegroundColor Red; exit 1 }

# Copy config
$outDir = "$BuildDir\$Config"
if (-not (Test-Path "$outDir\config")) { New-Item -ItemType Directory "$outDir\config" | Out-Null }
Copy-Item "resources\default_settings.json" "$outDir\config\" -Force
Copy-Item "resources\default_hotkeys.json" "$outDir\config\" -Force

Write-Host ""
Write-Host "[3/3] Build successful!" -ForegroundColor Green
Write-Host "  Output: $outDir\DocVision.exe" -ForegroundColor White

# Installer (Inno Setup)
if ($Installer -or $All) {
    Write-Host ""
    Write-Host "Building Inno Setup installer..." -ForegroundColor Cyan

    if (-not (Test-Path "$BuildDir\Release\DocVision.exe")) {
        Write-Host "ERROR: Release build required for installer." -ForegroundColor Red
        exit 1
    }

    # Find Inno Setup compiler
    $iscc = $null
    $isccPaths = @(
        "C:\Program Files (x86)\Inno Setup 6\ISCC.exe",
        "C:\Program Files\Inno Setup 6\ISCC.exe",
        "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe",
        "$env:ProgramFiles\Inno Setup 6\ISCC.exe"
    )
    foreach ($p in $isccPaths) {
        if (Test-Path $p) { $iscc = $p; break }
    }
    if (-not $iscc) {
        $iscc = Get-Command iscc -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source
    }
    if (-not $iscc) {
        Write-Host "ERROR: Inno Setup not found." -ForegroundColor Red
        Write-Host "  Download free from: https://jrsoftware.org/isdl.php" -ForegroundColor Yellow
        exit 1
    }

    Write-Host "Using Inno Setup: $iscc" -ForegroundColor Gray
    if (-not (Test-Path "$BuildDir\installer")) { New-Item -ItemType Directory "$BuildDir\installer" | Out-Null }

    & $iscc "installer\docvision.iss"
    if ($LASTEXITCODE -ne 0) { Write-Host "Inno Setup failed." -ForegroundColor Red; exit 1 }

    Write-Host "Installer: build\installer\$ProductName-$ProductVersion-Setup-x64.exe" -ForegroundColor Green
}

# Portable ZIP
if ($Portable -or $All) {
    Write-Host ""
    Write-Host "Creating portable ZIP..." -ForegroundColor Cyan

    $zipName = "$ProductName-$ProductVersion-Portable-x64"
    $stagingDir = "$BuildDir\$zipName"

    if (Test-Path $stagingDir) { Remove-Item -Recurse -Force $stagingDir }
    New-Item -ItemType Directory $stagingDir | Out-Null
    New-Item -ItemType Directory "$stagingDir\config" | Out-Null
    New-Item -ItemType Directory "$stagingDir\logs" | Out-Null
    New-Item -ItemType Directory "$stagingDir\cache" | Out-Null
    New-Item -ItemType Directory "$stagingDir\tessdata" | Out-Null

    Copy-Item "$outDir\DocVision.exe" $stagingDir
    Copy-Item "resources\default_settings.json" "$stagingDir\config\"
    Copy-Item "resources\default_hotkeys.json" "$stagingDir\config\"
    Copy-Item "LICENSE" $stagingDir

    # Mark as portable
    Set-Content "$stagingDir\portable.txt" "This is a portable installation of DocVision.`nAll data is stored in this directory."

    $zipPath = "$BuildDir\$zipName.zip"
    if (Test-Path $zipPath) { Remove-Item $zipPath }
    Compress-Archive -Path "$stagingDir\*" -DestinationPath $zipPath

    Write-Host "Portable ZIP: $zipPath" -ForegroundColor Green
}

Write-Host ""
Write-Host "All done!" -ForegroundColor Green
