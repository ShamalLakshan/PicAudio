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

# Extract project version from CMakeLists.txt or environment
$Version = "0.1.0"
$CMakeListsPath = Join-Path $RepoRoot "CMakeLists.txt"
if (Test-Path $CMakeListsPath) {
    $CMakeContent = Get-Content $CMakeListsPath -Raw
    if ($CMakeContent -match 'project\(PicAudio VERSION\s+([0-9.]+)') {
        $Version = $Matches[1]
    }
}
if ($env:PICAUDIO_VERSION) {
    $Version = $env:PICAUDIO_VERSION
}

Write-Host "=== PicAudio Windows Portable & Setup Packager ===" -ForegroundColor Cyan
Write-Host "Repo Root: $RepoRoot"
Write-Host "Build Dir: $BuildPath"
Write-Host "Architecture: $Arch"
Write-Host "Version: $Version"

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
$RedistRoot = $env:VCToolsRedistDir
if (-not $RedistRoot) {
    $VsRedistCand = Get-ChildItem -Path "C:\Program Files\Microsoft Visual Studio\*\*\VC\Redist\MSVC\*" -Directory -ErrorAction SilentlyContinue | Select-Object -Last 1
    if ($VsRedistCand) {
        $RedistRoot = $VsRedistCand.FullName
    }
}
if ($RedistRoot) {
    $CrtDirs = Get-ChildItem -Path (Join-Path $RedistRoot $Arch) -Filter "Microsoft.VC*.CRT" -Directory -ErrorAction SilentlyContinue
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
& $WindeployqtCmd --release --qmldir $QmlDir --compiler-runtime --no-translations --include-plugins qsqlite (Join-Path $StagePath "PicAudio.exe")

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

# 6. Create versioned and standard portable ZIP archives
$VersionedZip = Join-Path $OutputPath "PicAudio-$Version-windows-$Arch-portable.zip"
$StandardZip = Join-Path $OutputPath "PicAudio-windows-$Arch-portable.zip"

Write-Host "--> Compressing to $VersionedZip..."
Compress-Archive -Path "$StagePath\*" -DestinationPath $VersionedZip -Force
Copy-Item -Path $VersionedZip -Destination $StandardZip -Force

Write-Host "=== Windows Portable ZIP created successfully! ===" -ForegroundColor Green
Write-Host "Versioned File: $VersionedZip"
Write-Host "Standard File:  $StandardZip"

# 7. Build NSIS Setup Installer if makensis is available
$MakensisCmd = $null
if (Get-Command makensis -ErrorAction SilentlyContinue) {
    $MakensisCmd = "makensis"
} elseif (Test-Path "C:\Program Files (x86)\NSIS\makensis.exe") {
    $MakensisCmd = "C:\Program Files (x86)\NSIS\makensis.exe"
} elseif (Test-Path "C:\Program Files\NSIS\makensis.exe") {
    $MakensisCmd = "C:\Program Files\NSIS\makensis.exe"
}

if ($MakensisCmd -and (Test-Path "$PSScriptRoot\installer.nsi")) {
    $VersionedSetup = Join-Path $OutputPath "PicAudio-$Version-windows-$Arch-setup.exe"
    $StandardSetup = Join-Path $OutputPath "PicAudio-windows-$Arch-setup.exe"
    Write-Host "--> Building NSIS Setup Installer: $VersionedSetup..."

    & $MakensisCmd "/DVERSION=$Version" "/DARCH=$Arch" "/DINPUTDIR=$StagePath" "/DOUTPUTFILE=$VersionedSetup" "$PSScriptRoot\installer.nsi"
    if (Test-Path $VersionedSetup) {
        Copy-Item -Path $VersionedSetup -Destination $StandardSetup -Force
        Write-Host "=== Windows Setup Installer created successfully! ===" -ForegroundColor Green
        Write-Host "Versioned Setup: $VersionedSetup"
        Write-Host "Standard Setup:  $StandardSetup"
    }
}

# 8. Create self-extracting portable EXE if 7-Zip is available
$7z = $null
if (Get-Command 7z -ErrorAction SilentlyContinue) {
    $7z = "7z"
} elseif (Test-Path "C:\Program Files\7-Zip\7z.exe") {
    $7z = "C:\Program Files\7-Zip\7z.exe"
}

if ($7z) {
    $ExeArchive = Join-Path $OutputPath "PicAudio-$Version-windows-$Arch-portable.exe"
    Write-Host "--> Creating self-extracting portable EXE: $ExeArchive..."
    & $7z a -sfx -r "$ExeArchive" "$StagePath\*"
}

# 9. Generate SHA256 checksums for all release packages in OutputPath
Write-Host "--> Generating SHA256 checksums..."
Get-ChildItem -Path $OutputPath -Include "PicAudio*.zip", "PicAudio*.exe" -File | ForEach-Object {
    $Hash = (Get-FileHash -Path $_.FullName -Algorithm SHA256).Hash.ToLower()
    $ShaFile = "$($_.FullName).sha256"
    "$Hash  $($_.Name)" | Set-Content -Path $ShaFile -Encoding ASCII
}
Write-Host "All Windows artifacts generated in $OutputPath"
