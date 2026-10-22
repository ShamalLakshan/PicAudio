<#
.SYNOPSIS
    Packages PicAudio into a standalone portable ZIP archive and self-extracting EXE for Windows.
.DESCRIPTION
    Deploys Qt libraries, QML modules, plugins, C++ runtime, and third-party dependencies into a
    self-contained portable directory and produces release artifacts.
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

# Check executable across common CMake generator layouts
$CandidatePaths = @(
    (Join-Path $BuildPath "app\Release\PicAudio.exe"),
    (Join-Path $BuildPath "bin\Release\PicAudio.exe"),
    (Join-Path $BuildPath "Release\PicAudio.exe"),
    (Join-Path $BuildPath "app\PicAudio.exe"),
    (Join-Path $BuildPath "bin\PicAudio.exe"),
    (Join-Path $BuildPath "PicAudio.exe")
)

$ExePath = $null
foreach ($cand in $CandidatePaths) {
    if (Test-Path $cand) {
        $ExePath = $cand
        break
    }
}

if (-not $ExePath) {
    $Found = Get-ChildItem -Path $BuildPath -Filter "PicAudio.exe" -Recurse -File -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($Found) {
        $ExePath = $Found.FullName
    }
}

if (-not $ExePath) {
    Write-Error "PicAudio.exe not found in $BuildPath. Please build the project first."
}

$ExeDir = Split-Path $ExePath -Parent
Write-Host "--> Found binary at: $ExePath"

# Clean and recreate staging directory
if (Test-Path $StagePath) {
    Remove-Item -Recurse -Force $StagePath
}
New-Item -ItemType Directory -Path $StagePath | Out-Null
New-Item -ItemType Directory -Path $OutputPath -Force | Out-Null

# 1. Copy executable and neighboring DLL dependencies
Write-Host "--> Copying executable and neighboring libraries from $ExeDir..."
Copy-Item $ExePath -Destination (Join-Path $StagePath "PicAudio.exe") -Force

Get-ChildItem -Path $ExeDir -Filter "*.dll" -ErrorAction SilentlyContinue | ForEach-Object {
    Copy-Item $_.FullName -Destination $StagePath -Force
}

# 1.1 Copy vcpkg dynamic dependencies if available
if ($env:VCPKG_INSTALLATION_ROOT) {
    $VcpkgBin = Join-Path $env:VCPKG_INSTALLATION_ROOT "installed\$Arch-windows\bin"
    if (Test-Path $VcpkgBin) {
        Write-Host "--> Copying vcpkg runtime libraries from $VcpkgBin..."
        Get-ChildItem -Path $VcpkgBin -Filter "*.dll" -ErrorAction SilentlyContinue | ForEach-Object {
            Copy-Item $_.FullName -Destination $StagePath -Force
        }
    }
}

# 1.2 Copy MSVC CRT redistributable libraries if available to guarantee zero-dependency portable run
if ($env:VCToolsRedistDir) {
    $CrtDirs = Get-ChildItem -Path (Join-Path $env:VCToolsRedistDir $Arch) -Filter "Microsoft.VC*.CRT" -Directory -ErrorAction SilentlyContinue
    foreach ($dir in $CrtDirs) {
        Write-Host "--> Bundling MSVC CRT runtime libraries from $($dir.FullName)..."
        Get-ChildItem -Path $dir.FullName -Filter "*.dll" -ErrorAction SilentlyContinue | ForEach-Object {
            Copy-Item $_.FullName -Destination $StagePath -Force
        }
    }
}

# 2. Run windeployqt to deploy Qt libraries, plugins, and QML dependencies
$QmlDir = Join-Path $RepoRoot "adapters\ui_qt\qml"
Write-Host "--> Running windeployqt with QML scanning..."
$WindeployqtCmd = "windeployqt"
if (-not (Get-Command windeployqt -ErrorAction SilentlyContinue)) {
    if ($env:QT_DIR -and (Test-Path (Join-Path $env:QT_DIR "bin\windeployqt.exe"))) {
        $WindeployqtCmd = Join-Path $env:QT_DIR "bin\windeployqt.exe"
    } elseif ($env:Qt6_DIR -and (Test-Path (Join-Path $env:Qt6_DIR "bin\windeployqt.exe"))) {
        $WindeployqtCmd = Join-Path $env:Qt6_DIR "bin\windeployqt.exe"
    }
}
& $WindeployqtCmd --release --qmldir $QmlDir --compiler-runtime --no-translations (Join-Path $StagePath "PicAudio.exe")

# 3. Copy themes
Write-Host "--> Bundling themes..."
$ThemesDir = Join-Path $RepoRoot "themes"
$StageThemes = Join-Path $StagePath "themes"
New-Item -ItemType Directory -Path $StageThemes -Force | Out-Null
Copy-Item "$ThemesDir\*.json" -Destination $StageThemes -Force

# 4. Mark as portable
New-Item -ItemType File -Path (Join-Path $StagePath "portable.dat") -Force | Out-Null
New-Item -ItemType Directory -Path (Join-Path $StagePath "data") -Force | Out-Null

# 5. Include Windows registration utilities and icon
if (Test-Path "$PSScriptRoot\Register-FileAssociations.ps1") {
    Copy-Item "$PSScriptRoot\Register-FileAssociations.ps1" -Destination $StagePath -Force
}
if (Test-Path "$PSScriptRoot\register-file-associations.reg") {
    Copy-Item "$PSScriptRoot\register-file-associations.reg" -Destination $StagePath -Force
}
if (Test-Path "$PSScriptRoot\PicAudio.ico") {
    Copy-Item "$PSScriptRoot\PicAudio.ico" -Destination $StagePath -Force
}

# 6. Create ZIP archive
$ZipFile = Join-Path $OutputPath "PicAudio-windows-$Arch-portable.zip"
if (Test-Path $ZipFile) {
    Remove-Item -Force $ZipFile
}

Write-Host "--> Compressing to $ZipFile..."
Compress-Archive -Path "$StagePath\*" -DestinationPath $ZipFile -Force
Write-Host "=== Windows Portable ZIP created successfully! ===" -ForegroundColor Green
Write-Host "File: $ZipFile"

# 7. Create self-extracting portable EXE if 7-Zip is available
$7z = $null
if (Get-Command 7z -ErrorAction SilentlyContinue) {
    $7z = "7z"
} elseif (Test-Path "C:\Program Files\7-Zip\7z.exe") {
    $7z = "C:\Program Files\7-Zip\7z.exe"
}

if ($7z) {
    $ExeArchive = Join-Path $OutputPath "PicAudio-windows-$Arch-portable.exe"
    if (Test-Path $ExeArchive) {
        Remove-Item -Force $ExeArchive
    }
    Write-Host "--> Creating self-extracting portable EXE: $ExeArchive..."
    & $7z a -sfx -r "$ExeArchive" "$StagePath\*"
    if (Test-Path $ExeArchive) {
        Write-Host "=== Windows Portable SFX EXE created successfully! ===" -ForegroundColor Green
        Write-Host "File: $ExeArchive"
    }
}
