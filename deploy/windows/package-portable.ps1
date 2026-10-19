<#
.SYNOPSIS
    Packages PicAudio into a standalone portable ZIP archive for Windows.
.DESCRIPTION
    Deploys Qt libraries, QML modules, plugins, and third-party dependencies into a
    self-contained portable directory and produces a release ZIP.
#>

param(
    [string]$BuildDir = "build",
    [string]$OutputDir = "dist",
    [string]$Arch = "x64"
)

$ErrorActionPreference = "Stop"

$RepoRoot = Resolve-Path "$PSScriptRoot\..\.."
$BuildPath = Join-Path $RepoRoot $BuildDir
$OutputPath = Join-Path $RepoRoot $OutputDir
$StagePath = Join-Path $BuildPath "portable-stage"

Write-Host "=== PicAudio Windows Portable Packager ===" -ForegroundColor Cyan
Write-Host "Repo Root: $RepoRoot"
Write-Host "Build Dir: $BuildPath"
Write-Host "Architecture: $Arch"

# Check executable
$ExePath = Join-Path $BuildPath "app\Release\PicAudio.exe"
if (-not (Test-Path $ExePath)) {
    $ExePath = Join-Path $BuildPath "app\PicAudio.exe"
}
if (-not (Test-Path $ExePath)) {
    $ExePath = Join-Path $BuildPath "bin\PicAudio.exe"
}
if (-not (Test-Path $ExePath)) {
    Write-Error "PicAudio.exe not found in $BuildPath. Please build the project first."
}

# Clean and recreate staging directory
if (Test-Path $StagePath) {
    Remove-Item -Recurse -Force $StagePath
}
New-Item -ItemType Directory -Path $StagePath | Out-Null
New-Item -ItemType Directory -Path $OutputPath -Force | Out-Null

# 1. Copy executable and dependencies
Write-Host "--> Copying executable and libraries..."
Copy-Item $ExePath -Destination (Join-Path $StagePath "PicAudio.exe")

# 2. Run windeployqt
$QmlDir = Join-Path $RepoRoot "adapters\ui_qt\qml"
Write-Host "--> Running windeployqt with QML scanning..."
& windeployqt --qmldir $QmlDir --compiler-runtime --no-translations (Join-Path $StagePath "PicAudio.exe")

# 3. Copy themes
Write-Host "--> Bundling themes..."
$ThemesDir = Join-Path $RepoRoot "themes"
$StageThemes = Join-Path $StagePath "themes"
New-Item -ItemType Directory -Path $StageThemes -Force | Out-Null
Copy-Item "$ThemesDir\*.json" -Destination $StageThemes

# 4. Mark as portable
New-Item -ItemType File -Path (Join-Path $StagePath "portable.dat") -Force | Out-Null
New-Item -ItemType Directory -Path (Join-Path $StagePath "data") -Force | Out-Null

# 5. Create ZIP archive
$ZipFile = Join-Path $OutputPath "PicAudio-windows-$Arch-portable.zip"
if (Test-Path $ZipFile) {
    Remove-Item -Force $ZipFile
}

Write-Host "--> Compressing to $ZipFile..."
Compress-Archive -Path "$StagePath\*" -DestinationPath $ZipFile -Force

Write-Host "=== Windows Portable ZIP created successfully! ===" -ForegroundColor Green
Write-Host "File: $ZipFile"
