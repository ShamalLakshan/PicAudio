; PicAudio Windows NSIS Installer Script
; Produces a native, clean Windows setup installer for PicAudio

!include "MUI2.nsh"
!include "FileFunc.nsh"
!include "LogicLib.nsh"

!ifndef VERSION
  !define VERSION "0.1.0"
!endif

!ifndef ARCH
  !define ARCH "x64"
!endif

!ifndef INPUTDIR
  !define INPUTDIR "..\..\build\portable-stage"
!endif

!ifndef OUTPUTFILE
  !define OUTPUTFILE "..\..\dist\PicAudio-${VERSION}-windows-${ARCH}-setup.exe"
!endif

!define PRODUCT_NAME "PicAudio"
!define PRODUCT_PUBLISHER "PicAudio Team"
!define PRODUCT_WEB_SITE "https://github.com/ShamalLakshan/PicAudio"
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\PicAudio.exe"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\PicAudio"

Name "${PRODUCT_NAME} ${VERSION}"
OutFile "${OUTPUTFILE}"
InstallDir "$PROGRAMFILES64\PicAudio"
InstallDirRegKey HKLM "${PRODUCT_DIR_REGKEY}" ""
ShowInstDetails show
ShowUnInstDetails show
RequestExecutionLevel admin
SetCompressor /SOLID lzma

; Interface Configuration
!define MUI_ABORTWARNING
!define MUI_ICON "PicAudio.ico"
!define MUI_UNICON "PicAudio.ico"

; Installer Pages
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE_RUN "$INSTDIR\PicAudio.exe"
!define MUI_FINISHPAGE_RUN_TEXT "Launch PicAudio"
!insertmacro MUI_PAGE_FINISH

; Uninstaller Pages
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

; Languages
!insertmacro MUI_LANGUAGE "English"

Section "MainSection" SEC01
  SetRegView 64
  SetOutPath "$INSTDIR"
  SetOverwrite try

  ; Copy all files from staging directory (excluding portable marker)
  File /r /x "portable.dat" "${INPUTDIR}\*.*"

  ; Create Start Menu Shortcuts
  CreateDirectory "$SMPROGRAMS\PicAudio"
  CreateShortcut "$SMPROGRAMS\PicAudio\PicAudio.lnk" "$INSTDIR\PicAudio.exe" "" "$INSTDIR\PicAudio.ico" 0
  CreateShortcut "$SMPROGRAMS\PicAudio\Uninstall PicAudio.lnk" "$INSTDIR\uninstall.exe"

  ; Create Desktop Shortcut
  CreateShortcut "$DESKTOP\PicAudio.lnk" "$INSTDIR\PicAudio.exe" "" "$INSTDIR\PicAudio.ico" 0

  ; Write App Paths key
  WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "" "$INSTDIR\PicAudio.exe"
  WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "Path" "$INSTDIR"

  ; Write Uninstaller and Add/Remove Programs metadata
  WriteUninstaller "$INSTDIR\uninstall.exe"
  WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "DisplayName" "${PRODUCT_NAME}"
  WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${VERSION}"
  WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\PicAudio.ico,0"
  WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "QuietUninstallString" '"$INSTDIR\uninstall.exe" /S'
  WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "InstallLocation" "$INSTDIR"
  WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
  WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
  WriteRegDWORD HKLM "${PRODUCT_UNINST_KEY}" "NoModify" 1
  WriteRegDWORD HKLM "${PRODUCT_UNINST_KEY}" "NoRepair" 1

  ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
  IntFmt $0 "0x%08X" $0
  WriteRegDWORD HKLM "${PRODUCT_UNINST_KEY}" "EstimatedSize" "$0"
SectionEnd

Section "Uninstall"
  SetRegView 64

  ; Delete Shortcuts
  Delete "$DESKTOP\PicAudio.lnk"
  Delete "$SMPROGRAMS\PicAudio\PicAudio.lnk"
  Delete "$SMPROGRAMS\PicAudio\Uninstall PicAudio.lnk"
  RMDir "$SMPROGRAMS\PicAudio"

  ; Delete Registry Keys
  DeleteRegKey HKLM "${PRODUCT_UNINST_KEY}"
  DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"

  ; Delete Installed Files and Directory
  RMDir /r "$INSTDIR"
SectionEnd
