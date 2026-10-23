# Installation & Running Guide

This guide covers instructions and system requirements for running pre-compiled PicAudio packages on fresh installations of Linux and Windows.

---

## Linux (AppImage)

The Linux package is distributed as a single self-contained `PicAudio-x86_64.AppImage`. It bundles Qt 6, QML components, TagLib, SQLite, and the GStreamer audio pipeline.

### System Requirements

- **Architecture**: 64-bit x86 (`x86_64` / `amd64`)
- **glibc**: Version 2.35 or newer (Ubuntu 22.04+, Debian 12+, Fedora 36+, Arch Linux, or equivalent modern distribution)
- **Display**: Wayland or X11 (both are supported out of the box)
- **FUSE**: AppImages require FUSE 2 to mount. If your system does not have FUSE installed:
  - **Ubuntu / Debian**:
    ```bash
    sudo apt install libfuse2
    ```
  - **Fedora**:
    ```bash
    sudo dnf install fuse-libs
    ```
  - **Arch Linux / EndeavourOS**:
    ```bash
    sudo pacman -S fuse2
    ```
  - *No-FUSE fallback*: If you cannot install FUSE, you can run the AppImage directly by extracting it in memory:
    ```bash
    ./PicAudio-x86_64.AppImage --appimage-extract-and-run
    ```

### How to Run

1. Open your terminal in the directory where the file was downloaded.
2. Grant execution permissions:
   ```bash
   chmod +x PicAudio-x86_64.AppImage
   ```
3. Launch the application:
   ```bash
   ./PicAudio-x86_64.AppImage
   ```

### Portable Mode

To keep your library database, settings, and cache isolated to the same directory (e.g., on a USB drive or without modifying your user profile):

```bash
./PicAudio-x86_64.AppImage --portable
```

Alternatively, create an empty file named `portable.dat` in the same directory as the AppImage. When present, all configuration, databases, artwork cache, and GStreamer registries will be stored inside a local `data/` folder next to the AppImage.

---

## Windows (Portable Bundle)

The Windows release is provided as a standalone portable package (`PicAudio-windows-x64-portable.zip` or self-extracting `.exe`). It bundles all required Qt 6 libraries, QML runtimes, TagLib, SQLite, and Microsoft Visual C++ CRT runtime DLLs.

### System Requirements

- **Operating System**: Windows 10 (version 1809+) or Windows 11 (64-bit)
- **Runtime Dependencies**: None. Microsoft Visual C++ redistributables and Qt libraries are bundled alongside the executable.
- **Audio Output**: Uses Windows Media Foundation (built into Windows).

### How to Run

1. **Extract Archive**:
   - If using `.zip`: Right-click the archive, select **Extract All...**, and extract to your desired folder (e.g., `C:\PicAudio` or a portable USB drive).
   - If using the self-extracting `.exe`: Run the executable and extract to your desired destination.
2. Open the extracted folder.
3. Double-click `PicAudio.exe`.

### Portable Mode & Data Storage

The portable bundle includes a `portable.dat` marker file by default. All data—including track libraries, playlists, SQLite database, and user settings—is saved locally in the `data/` subfolder rather than `%APPDATA%`.

### Optional: Register Audio File Associations

To associate common audio file formats (`.flac`, `.wav`, `.mp3`, `.m4a`, `.opus`, etc.) with PicAudio:
- Open PowerShell in the extracted folder as an Administrator and run:
  ```powershell
  .\Register-FileAssociations.ps1
  ```
- Alternatively, double-click `register-file-associations.reg` to merge the registry settings directly.
