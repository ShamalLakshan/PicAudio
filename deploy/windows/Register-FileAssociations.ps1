<#
.SYNOPSIS
    Registers or unregisters PicAudio audio file type associations in Windows.
.DESCRIPTION
    Associates supported audio and playlist file extensions with PicAudio in HKCU:\Software\Classes,
    adding Explorer context menu verbs for "Play with PicAudio" and "Enqueue in PicAudio".
.PARAMETER ExePath
    Path to PicAudio.exe. Defaults to the executable in the current directory or build folder.
.PARAMETER Unregister
    If specified, removes all file associations and context menu verbs.
#>

param(
    [string]$ExePath = "",
    [switch]$Unregister
)

$ErrorActionPreference = "Stop"

$SupportedExtensions = @(
    ".flac", ".wav", ".aiff", ".aif", ".mp3", ".m4a", ".alac", ".mp4",
    ".m4b", ".aac", ".ogg", ".oga", ".opus", ".spx", ".dsf", ".dff",
    ".ape", ".wv", ".wma", ".cue", ".m3u", ".m3u8", ".pls"
)

$ProgId = "PicAudio.AudioFile"
$RootKey = "HKCU:\Software\Classes"

if ($Unregister) {
    Write-Host "Unregistering PicAudio file associations..." -ForegroundColor Yellow

    # Remove ProgID
    if (Test-Path "$RootKey\$ProgId") {
        Remove-Item -Recurse -Force "$RootKey\$ProgId"
    }

    # Clean up extension mappings
    foreach ($ext in $SupportedExtensions) {
        $extPath = "$RootKey\$ext\OpenWithProgids"
        if (Test-Path $extPath) {
            Remove-ItemProperty -Path $extPath -Name $ProgId -ErrorAction SilentlyContinue
        }
    }

    Write-Host "PicAudio file associations unregistered successfully." -ForegroundColor Green
    return
}

# Find PicAudio.exe
if (-not $ExePath) {
    $candidates = @(
        "$PSScriptRoot\PicAudio.exe",
        "$PSScriptRoot\..\..\build\app\Release\PicAudio.exe",
        "$PSScriptRoot\..\..\build\app\PicAudio.exe",
        "$PSScriptRoot\..\..\build\bin\PicAudio.exe"
    )
    foreach ($c in $candidates) {
        if (Test-Path $c) {
            $ExePath = (Resolve-Path $c).Path
            break
        }
    }
}

if (-not $ExePath -or -not (Test-Path $ExePath)) {
    Write-Error "PicAudio.exe not found. Please provide -ExePath pointing to PicAudio.exe"
}

Write-Host "Registering PicAudio file associations for $ExePath..." -ForegroundColor Cyan

# 1. Create ProgID with shell verbs
New-Item -Path "$RootKey\$ProgId" -Force | Out-Null
Set-ItemProperty -Path "$RootKey\$ProgId" -Name "(default)" -Value "Audio Track"
Set-ItemProperty -Path "$RootKey\$ProgId" -Name "FriendlyTypeName" -Value "PicAudio Audio Track"

# Default icon
New-Item -Path "$RootKey\$ProgId\DefaultIcon" -Force | Out-Null
Set-ItemProperty -Path "$RootKey\$ProgId\DefaultIcon" -Name "(default)" -Value "`"$ExePath`",0"

# Open Verb: "Play with PicAudio"
New-Item -Path "$RootKey\$ProgId\shell\open" -Force | Out-Null
Set-ItemProperty -Path "$RootKey\$ProgId\shell\open" -Name "(default)" -Value "&Play with PicAudio"
New-Item -Path "$RootKey\$ProgId\shell\open\command" -Force | Out-Null
Set-ItemProperty -Path "$RootKey\$ProgId\shell\open\command" -Name "(default)" -Value "`"$ExePath`" `"%1`""

# Enqueue Verb: "Enqueue in PicAudio"
New-Item -Path "$RootKey\$ProgId\shell\enqueue" -Force | Out-Null
Set-ItemProperty -Path "$RootKey\$ProgId\shell\enqueue" -Name "(default)" -Value "&Enqueue in PicAudio"
New-Item -Path "$RootKey\$ProgId\shell\enqueue\command" -Force | Out-Null
Set-ItemProperty -Path "$RootKey\$ProgId\shell\enqueue\command" -Name "(default)" -Value "`"$ExePath`" --enqueue `"%1`""

# 2. Register OpenWithProgids for each extension
foreach ($ext in $SupportedExtensions) {
    $extKey = "$RootKey\$ext"
    New-Item -Path $extKey -Force | Out-Null
    $openWithKey = "$extKey\OpenWithProgids"
    New-Item -Path $openWithKey -Force | Out-Null
    New-ItemProperty -Path $openWithKey -Name $ProgId -PropertyType Binary -Force | Out-Null
}

Write-Host "Successfully registered PicAudio file associations!" -ForegroundColor Green
