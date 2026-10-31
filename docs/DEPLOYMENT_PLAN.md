# PicAudio — Cross-Platform Compilation & Deployment Architecture

This document defines the strategy for compiling, packaging, and deploying PicAudio across Windows, Linux, and macOS, supporting both 64-bit and 32-bit architectures. It outlines the path from rapid-iteration portable builds for device testing to final production installers with native operating system directory integration.

---

## 1. Target Architecture & Platform Matrix

| Platform | Target Architecture | Compiler Toolchain | Portable Format (Testing) | Production Release Format |
| :--- | :--- | :--- | :--- | :--- |
| **Linux** | `x86_64` (64-bit Intel/AMD) | GCC 12+ / Clang 15+ | Standalone `.tar.gz` / AppImage | Flatpak (Flathub), AppImage, `.deb`, `.rpm` |
| **Linux** | `i686` (32-bit x86) | GCC 12+ (multilib / container) | Standalone `.tar.gz` / AppImage | AppImage, `.deb` |
| **Linux** | `aarch64` (64-bit ARM) | GCC 12+ / Clang (QEMU/cross) | Standalone `.tar.gz` / AppImage | Flatpak, AppImage, `.deb` |
| **Linux** | `armv7l` (32-bit ARMhf) | GCC 12+ (QEMU/cross) | Standalone `.tar.gz` | `.deb` |
| **Windows** | `x86_64` (64-bit x64) | MSVC 2022 / MinGW-w64 | Standalone `.zip` | NSIS / Inno Setup installer (`.exe`), `.msi` |
| **Windows** | `i686` (32-bit x86) | MSVC 2022 (x86) / MinGW-w64 | Standalone `.zip` | NSIS installer (`.exe`) |
| **Windows** | `arm64` (Windows on ARM) | MSVC 2022 (ARM64 cross) | Standalone `.zip` | NSIS installer (`.exe`) |
| **macOS** | Universal 2 (`x86_64` + `arm64`) | Apple Clang / Xcode 15+ | Standalone `.zip` (`.app`) | Notarized `.dmg` drag-and-drop installer |

> **Note on 32-bit macOS:** Apple completely dropped 32-bit execution support in macOS 10.15 Catalina (2019). The macOS kernel and dynamic loader no longer execute 32-bit Mach-O binaries, and modern Xcode toolchains do not target 32-bit macOS. Therefore, macOS support is strictly 64-bit Universal Binary (`x86_64` Intel + `arm64` Apple Silicon). 32-bit support is actively targeted for Linux and Windows.

---

## 2. Compilation Strategy: Native Matrix Runners vs. Cross-Compilation

Compiling a modern C++20 application with Qt 6 Quick/QML across multiple operating systems requires evaluating how host build tools interact with target code.

### 2.1 The Challenge with Qt 6 QML Cross-Compilation
Qt 6 relies on host-side tools during build time:
- `moc` (Meta-Object Compiler)
- `rcc` (Resource Compiler)
- `qmlcachegen` / `qmllint` / `qmlimportscanner` (QML compiler pipeline)

Cross-compiling Qt 6 from a single Linux workstation to target Windows and macOS requires either cross-compiling the entire Qt 6 toolchain or maintaining complex multi-sysroot cross-environments. Additionally, macOS code signing (`codesign`) and notarization (`xcrun notarytool`) strictly require native macOS environments.

### 2.2 Recommended Compilation Approach: GitHub Actions Native Matrix
The most reliable and maintainable strategy is using **native GitHub Actions runners** for each major operating system:

1. **Linux Runner (`ubuntu-latest` / `ubuntu-22.04`):**
   - Native compilation for `x86_64`.
   - Containerized compilation via Docker for `i686` (32-bit Debian/Ubuntu container) and `aarch64` / `armv7l` (using `docker/setup-qemu-action`).
2. **Windows Runner (`windows-latest` / Windows Server 2022):**
   - Native compilation with MSVC 2022 or MinGW-w64.
   - 64-bit build using `vcvars64.bat`.
   - 32-bit build using `vcvars32.bat` (or MSYS2 MinGW32).
3. **macOS Runner (`macos-latest` / `macos-14` Apple Silicon or `macos-13` Intel):**
   - Native compilation producing Universal Binaries using `CMAKE_OSX_ARCHITECTURES="x86_64;arm64"`.

### 2.3 Dependency Management Strategy
To avoid platform-specific dependency failures (such as `pkg-config` missing on standard Windows runner environments), dependencies should be managed consistently:

- **Qt 6**: Installed via `jurplel/install-qt-action` in CI (powered by `aqtinstall`) or via system package managers (Homebrew on macOS, APT on Linux).
- **SQLite 3**: Standard CMake `find_package(SQLite3 REQUIRED)` built into CMake 3.14+, falling back to system libraries.
- **TagLib**: Built via CMake FetchContent/vcpkg or resolved via `find_package(TagLib CONFIG)` / system packages (`libtag1-dev`, `brew install taglib`).
- **GoogleTest**: Built via `FetchContent` or system dev package.

---

## 3. Phase 1: Rapid Testing Stage — Portable Builds

During active development and cross-device testing, running full installers is slow and litters test machines. Portable builds package all binaries, shared libraries, and assets into a single directory or archive that can be copied via USB drive or downloaded directly, running without administrator privileges.

### 3.1 Portable Data Isolation in `app/main.cpp`
To keep portable builds completely isolated from the host machine's user directory, the application checks for a `portable.dat` file or a local `data/` folder in the same directory as the executable:

```cpp
// Portable detection logic in main.cpp:
QString exeDir = QCoreApplication::applicationDirPath();
QString portableDir = exeDir + "/data";
bool isPortable = QFile::exists(exeDir + "/portable.dat") || QDir(portableDir).exists();

QString appDataDir;
if (isPortable) {
    appDataDir = portableDir;
    QDir().mkpath(appDataDir);
} else {
    appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(appDataDir);
}
```

This guarantees:
- SQLite database (`picaudio_library.db`), cached cover art (`covers/`), and user configuration stay inside `./data/`.
- Deleting the portable directory completely removes the application without orphaned files.

### 3.2 Platform-Specific Portable Bundles

#### Windows Portable Bundle (`.zip`)
1. Compile `PicAudio.exe` using CMake in `Release` mode.
2. Run Qt deployment tool:
   ```cmd
   windeployqt --qmldir adapters/ui_qt/qml --no-translations bin/PicAudio.exe
   ```
3. Copy third-party runtime DLLs (`sqlite3.dll`, `taglib.dll`) into the binary directory.
4. Copy `themes/` and resource assets into the directory.
5. Create empty marker file `portable.dat` and an empty `data/` directory.
6. Compress into `PicAudio-vX.Y.Z-windows-<arch>-portable.zip`.

#### Linux Portable Bundle (`.tar.gz` and AppImage)
1. **Option A: Tarball with launcher script (`PicAudio.sh`)**
   - Structure:
     ```
     PicAudio-portable/
     ├── bin/
     │   └── PicAudio
     ├── lib/
     │   └── (bundled Qt6 shared objects, libtag.so, libsqlite3.so)
     ├── qml/
     ├── themes/
     ├── data/
     ├── portable.dat
     └── PicAudio.sh
     ```
   - `PicAudio.sh` sets `LD_LIBRARY_PATH`, `QT_PLUGIN_PATH`, and launches `bin/PicAudio`.
2. **Option B: Type-2 AppImage**
   - Single executable binary bundling application, Qt runtime, and dependencies.
   - Run `linuxdeploy --plugin qt --appdir AppDir --output appimage`.
   - When run with an adjacent `PicAudio.AppImage.home/` directory, AppImages automatically isolate user data.

#### macOS Portable Bundle (`.zip`)
1. Build `PicAudio.app` bundle via CMake.
2. Run deployment tool:
   ```bash
   macdeployqt PicAudio.app -qmldir=adapters/ui_qt/qml
   ```
3. Copy default themes into `PicAudio.app/Contents/Resources/themes/`.
4. Create `portable.dat` in `PicAudio.app/Contents/MacOS/` if portable mode is desired, or run directly.
5. Compress into `PicAudio-vX.Y.Z-macos-universal-portable.zip`.

---

## 4. Phase 2: Final Release — Installers & System Directory Integration

For production releases, PicAudio must adhere to standard desktop platform conventions, managing permissions, system menus, file associations, and standard directories cleanly.

### 4.1 Native Operating System Directory Standards

| Resource | Linux (XDG Base Directory) | Windows | macOS |
| :--- | :--- | :--- | :--- |
| **Executable Binary** | `/usr/bin/picaudio` or `/opt/picaudio/bin/` | `%ProgramFiles%\PicAudio\PicAudio\` | `/Applications/PicAudio.app/Contents/MacOS/` |
| **Shared Themes & Assets** | `/usr/share/picaudio/themes/` | `%ProgramFiles%\PicAudio\PicAudio\themes\` | `PicAudio.app/Contents/Resources/themes/` |
| **Database & Artwork Cache** | `~/.local/share/PicAudio/` (`$XDG_DATA_HOME`) | `%AppData%\PicAudio\PicAudio\` | `~/Library/Application Support/PicAudio/` |
| **User Settings & Config** | `~/.config/PicAudio/` (`$XDG_CONFIG_HOME`) | `%AppData%\PicAudio\PicAudio\settings.json` | `~/Library/Preferences/org.picaudio.picaudio.plist` |
| **Temporary Files & Logs** | `~/.cache/PicAudio/` (`$XDG_CACHE_HOME`) | `%LocalAppData%\PicAudio\PicAudio\cache\` | `~/Library/Caches/PicAudio/` |

### 4.2 System Integration Specifications

#### 1. File Type Associations & MIME Integration
Register PicAudio as the default handler for audio formats:
- Audio files: `.flac`, `.wav`, `.mp3`, `.m4a`, `.ogg`, `.opus`, `.alac`, `.aiff`
- Playlist & cue files: `.m3u`, `.m3u8`, `.cue`

- **Linux**: Install MIME definitions in `/usr/share/mime/packages/picaudio.xml` and register `MimeType=audio/flac;audio/x-wav;audio/mpeg;...` in the `.desktop` file.
- **Windows**: Register file extensions and ProgIDs under `HKEY_CURRENT_USER\Software\Classes` or `HKEY_LOCAL_MACHINE\Software\Classes` via installer script.
- **macOS**: Add `CFBundleDocumentTypes` entries in `Info.plist` with UTIs (`public.audio`, `public.mp3`, `org.xiph.flac`).

#### 2. Desktop Menus, Icons & Accelerators
- **Linux**: Install standard Freedesktop `.desktop` file to `/usr/share/applications/picaudio.desktop` with icons in `/usr/share/icons/hicolor/{32x32,64x64,128x128,256x256,scalable}/apps/picaudio.png` (`.svg`).
- **Windows**: Create Start Menu shortcut and optional Desktop shortcut. Register Add/Remove Programs entry with uninstaller executable.
- **macOS**: Ship high-resolution `.icns` application icon inside the `.app` bundle.

#### 3. Native Media Integration
- **Linux**: Register MPRIS2 interface on D-Bus (`org.mpris.MediaPlayer2.picaudio`).
- **Windows**: System Media Transport Controls (SMTC) integration.
- **macOS**: `MPNowPlayingInfoCenter` and `MPRemoteCommandCenter`.

### 4.3 Installer Packaging Formats

#### Windows: NSIS (Nullsoft Scriptable Install System)
- Standalone installer executable (`PicAudio-vX.Y.Z-windows-x64-Setup.exe`).
- Features:
  - User can choose install location (defaults to `%ProgramFiles%\PicAudio\PicAudio`).
  - Option to install for "All Users" or "Current User only" (non-admin).
  - Checkbox options for Desktop shortcut and Start Menu folder.
  - File association checkboxes.
  - Clean uninstaller removing registry entries and binary files without deleting user library databases.
- Automated generation via CPack NSIS generator or custom `.nsi` script.

#### Linux: Flatpak & Native Distribution Formats
- **Flatpak**: Primary target for cross-distribution deployment on Flathub.
  - Uses `org.kde.Platform` or `io.qt.qt6` runtime.
  - Sandboxed access to user music folders (`--filesystem=xdg-music:ro` or user-configured directories via portal).
- **Native packages (.deb / .rpm)**: Generated using CMake `CPack` with `CPACK_GENERATOR="DEB;RPM"`.
- **AppImage**: Provided alongside installers for users who prefer single-file executables.

#### macOS: Signed & Notarized `.dmg`
- Drag-and-drop disk image containing `PicAudio.app` and a symlink to `/Applications`.
- Styled background with directional arrow instructing user to drag the app to Applications.
- Automated creation via `create-dmg` tool.
- Code signed with Apple Developer ID Application certificate.
- Notarized via `xcrun notarytool submit` and stapled with `xcrun stapler staple`.

---

## 5. End-to-End Pipeline: From Testing to Final Release

```
Local Development & Testing
  ├── Unit tests: ctest --test-dir build --output-on-failure
  ├── Static checks: clang-format & clang-tidy
  └── Local build verification
        │
        ▼ (Git Push / Pull Request)
GitHub Actions Continuous Integration (CI)
  ├── Matrix: Ubuntu (x64, i686-docker), Windows (x64, x86), macOS (Universal)
  ├── Compilation check across all compilers (GCC, Clang, MSVC)
  └── Automated test suite execution
        │
        ▼ (Merge to main branch)
Continuous Delivery (CD) — Portable Test Builds
  ├── Trigger: Nightly schedule or workflow dispatch
  ├── Package portable artifacts (.zip, .tar.gz, AppImage)
  └── Upload to GitHub Actions artifacts / "nightly" pre-release tag
      └── Enables quick download and testing on real devices without installation
        │
        ▼ (Release Git Tag: v*.*.*)
Production Release Pipeline (CD)
  ├── 1. Build release binaries in optimized mode
  ├── 2. Run windeployqt / macdeployqt / linuxdeploy
  ├── 3. Package portable archives (.zip, .tar.gz)
  ├── 4. Package installers (NSIS .exe, Flatpak, AppImage, .deb, .dmg)
  ├── 5. Code signing (Authenticode for Windows, Developer ID for macOS)
  ├── 6. Notarization & stapling for macOS DMG
  ├── 7. Generate SHA-256 checksums (SHA256SUMS.txt)
  └── 8. Publish draft/official GitHub Release with release notes & downloads
```

---

## 6. Implementation Roadmap

### Step 1: CMake Build Preparation
- Add portable mode detection in `app/main.cpp`.
- Refactor `adapters/CMakeLists.txt` to support standard CMake dependency discovery (`find_package(SQLite3)` and configurable TagLib paths) alongside `pkg-config`.
- Configure `CPack` in root `CMakeLists.txt` for standard archive generation (`ZIP`, `TGZ`).

### Step 2: Portable CI Workflow (`.github/workflows/portable-builds.yml`)
- Create GitHub Actions workflow building matrix for:
  - Linux `x86_64` (standalone `.tar.gz`)
  - Linux `i686` (via 32-bit container)
  - Windows `x86_64` & `x86` (portable `.zip` with `windeployqt`)
  - macOS Universal (portable `.zip` with `macdeployqt`)
- Verify artifact downloads and smoke test on physical test devices.

### Step 3: Installer Specifications & Release Workflow (`.github/workflows/release.yml`)
- Create NSIS installer script for Windows (`deploy/windows/installer.nsi`).
- Create Flatpak manifest (`deploy/linux/org.picaudio.PicAudio.yml`).
- Create macOS DMG script using `create-dmg` (`deploy/macos/create-dmg.sh`).
- Hook release workflow to trigger on git tags `v*`.
