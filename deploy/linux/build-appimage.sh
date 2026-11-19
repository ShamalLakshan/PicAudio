#!/usr/bin/env bash
set -euo pipefail

# PicAudio Linux AppImage Generator
# Builds a self-contained portable AppImage for Linux x86_64

# Ensure AppImages (linuxdeploy, appimagetool, plugins) run cleanly without requiring FUSE
export APPIMAGE_EXTRACT_AND_RUN=1

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

BUILD_DIR="${REPO_ROOT}/build-appimage"
APP_DIR="${REPO_ROOT}/build-appimage/AppDir"
OUTPUT_DIR="${REPO_ROOT}/dist"
TOOLS_DIR="${BUILD_DIR}/tools"

echo "=== PicAudio AppImage Packaging ==="
echo "Repository Root: ${REPO_ROOT}"
echo "Build Directory: ${BUILD_DIR}"
echo "Output Directory: ${OUTPUT_DIR}"

# Detect clean build request or stale CMake cache paths
CLEAN_BUILD=false
for arg in "$@"; do
    case "$arg" in
        --clean|-c)
            CLEAN_BUILD=true
            ;;
    esac
done

if [ -f "${BUILD_DIR}/CMakeCache.txt" ]; then
    CACHED_SOURCE="$(grep -E '^CMAKE_HOME_DIRECTORY:INTERNAL=' "${BUILD_DIR}/CMakeCache.txt" 2>/dev/null | cut -d= -f2 || true)"
    CACHED_BINARY="$(grep -E '^CMAKE_CACHEFILE_DIR:INTERNAL=' "${BUILD_DIR}/CMakeCache.txt" 2>/dev/null | cut -d= -f2 || true)"
    if { [ -n "${CACHED_SOURCE}" ] && [ "${CACHED_SOURCE}" != "${REPO_ROOT}" ]; } || \
       { [ -n "${CACHED_BINARY}" ] && [ "${CACHED_BINARY}" != "${BUILD_DIR}" ]; }; then
        echo "--> Stale CMakeCache detected (path mismatch: source='${CACHED_SOURCE}', binary='${CACHED_BINARY}')."
        CLEAN_BUILD=true
    fi
fi

if [ "${CLEAN_BUILD}" = true ]; then
    echo "--> Cleaning stale build artifacts..."
    find "${BUILD_DIR}" -mindepth 1 -maxdepth 1 ! -name tools -exec rm -rf {} +
fi

mkdir -p "${BUILD_DIR}"
mkdir -p "${OUTPUT_DIR}"
mkdir -p "${TOOLS_DIR}"
rm -rf "${APP_DIR}"
mkdir -p "${APP_DIR}"

# 1. Compile the project
echo "--> Configuring and compiling CMake in Release mode..."
cmake -B "${BUILD_DIR}" -S "${REPO_ROOT}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DBUILD_TESTING=OFF

cmake --build "${BUILD_DIR}" --parallel

# 2. Stage files into AppDir
echo "--> Staging files into AppDir..."
DESTDIR="${APP_DIR}" cmake --install "${BUILD_DIR}"

# Ensure desktop file and icon are in place
mkdir -p "${APP_DIR}/usr/share/applications"
mkdir -p "${APP_DIR}/usr/share/icons/hicolor/scalable/apps"

cp "${SCRIPT_DIR}/picaudio.desktop" "${APP_DIR}/usr/share/applications/picaudio.desktop"
cp "${SCRIPT_DIR}/icons/picaudio.svg" "${APP_DIR}/usr/share/icons/hicolor/scalable/apps/picaudio.svg"

# AppImage root links
cp "${SCRIPT_DIR}/picaudio.desktop" "${APP_DIR}/picaudio.desktop"
cp "${SCRIPT_DIR}/icons/picaudio.svg" "${APP_DIR}/picaudio.svg"
cp "${SCRIPT_DIR}/icons/picaudio.svg" "${APP_DIR}/.DirIcon"

# 3. Fetch linuxdeploy and Qt plugin if not present
echo "--> Setting up linuxdeploy..."
LINUXDEPLOY="${TOOLS_DIR}/linuxdeploy-x86_64.AppImage"
LINUXDEPLOY_QT="${TOOLS_DIR}/linuxdeploy-plugin-qt-x86_64.AppImage"
export LINUXDEPLOY

if [ ! -f "${LINUXDEPLOY}" ]; then
    echo "Downloading linuxdeploy..."
    curl -fsSL --retry 3 --retry-delay 2 --connect-timeout 15 -o "${LINUXDEPLOY}" \
        "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage"
    chmod +x "${LINUXDEPLOY}"
fi

if [ ! -f "${LINUXDEPLOY_QT}" ]; then
    echo "Downloading linuxdeploy-plugin-qt..."
    curl -fsSL --retry 3 --retry-delay 2 --connect-timeout 15 -o "${LINUXDEPLOY_QT}" \
        "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage"
    chmod +x "${LINUXDEPLOY_QT}"
fi

if ! command -v patchelf &> /dev/null && [ ! -f "${TOOLS_DIR}/patchelf" ]; then
    echo "Downloading patchelf..."
    curl -fsSL --retry 3 --retry-delay 2 --connect-timeout 15 \
        "https://github.com/NixOS/patchelf/releases/download/0.18.0/patchelf-0.18.0-x86_64.tar.gz" | \
        tar -xz -C "${TOOLS_DIR}" ./bin/patchelf --strip-components=2
    chmod +x "${TOOLS_DIR}/patchelf"
fi

if [ ! -d "${TOOLS_DIR}/appimagetool-prefix" ]; then
    echo "Extracting appimagetool from linuxdeploy..."
    (
        cd "${TOOLS_DIR}"
        "${LINUXDEPLOY}" --appimage-extract >/dev/null
        cp -r squashfs-root/plugins/linuxdeploy-plugin-appimage/appimagetool-prefix "${TOOLS_DIR}/appimagetool-prefix"
        rm -rf squashfs-root
    )
fi

# 4. Run linuxdeploy to bundle dependencies and generate AppImage
echo "--> Sanitizing Qt plugins and preparing bundle..."
if [ -d "/usr/lib/qt6/bin" ]; then
    export PATH="/usr/lib/qt6/bin:${TOOLS_DIR}:${PATH}"
else
    export PATH="${TOOLS_DIR}:${PATH}"
fi
export QML_SOURCES_PATHS="${REPO_ROOT}/adapters/ui_qt/qml"

# Support running in environments without FUSE (Docker, CI)
export APPIMAGE_EXTRACT_AND_RUN=1
# Disable strip to prevent failures on newer binutils / rolling distros
export NO_STRIP=true

# Locate base qmake/qmake6
REAL_QMAKE=""
if command -v qmake6 &> /dev/null; then
    REAL_QMAKE="$(which qmake6)"
elif command -v qmake &> /dev/null; then
    REAL_QMAKE="$(which qmake)"
elif [ -x "/usr/lib/qt6/bin/qmake" ]; then
    REAL_QMAKE="/usr/lib/qt6/bin/qmake"
fi

if [ -n "${REAL_QMAKE}" ]; then
    # Filter out extraneous KDE image formats (e.g. kimg_jxr missing libjxrglue on Arch)
    QT_ORIG_PLUGINS="$("${REAL_QMAKE}" -query QT_INSTALL_PLUGINS 2>/dev/null || true)"
    CLEAN_PLUGINS="${BUILD_DIR}/clean-qt-plugins"
    rm -rf "${CLEAN_PLUGINS}"
    mkdir -p "${CLEAN_PLUGINS}"

    if [ -d "${QT_ORIG_PLUGINS}" ]; then
        for d in "${QT_ORIG_PLUGINS}"/*; do
            name="$(basename "$d")"
            if [ "$name" != "imageformats" ]; then
                ln -sf "$d" "${CLEAN_PLUGINS}/${name}"
            fi
        done

        # In imageformats, only link standard Qt formats (excluding kimg_*)
        mkdir -p "${CLEAN_PLUGINS}/imageformats"
        for f in "${QT_ORIG_PLUGINS}/imageformats"/libq*.so; do
            [ -f "$f" ] && ln -sf "$f" "${CLEAN_PLUGINS}/imageformats/$(basename "$f")"
        done

        # Wrapper script intercepting QT_INSTALL_PLUGINS query
        QMAKE_WRAPPER="${TOOLS_DIR}/qmake-wrapper"
        cat << EOF > "${QMAKE_WRAPPER}"
#!/usr/bin/env bash
REAL_QMAKE="${REAL_QMAKE}"
CLEAN_DIR="${CLEAN_PLUGINS}"

if [ "\$1" = "-query" ] && [ -z "\$2" ]; then
    "\${REAL_QMAKE}" -query | sed "s|^QT_INSTALL_PLUGINS:.*|QT_INSTALL_PLUGINS:\${CLEAN_DIR}|"
    exit 0
fi

if [ "\$1" = "-query" ] && [ "\$2" = "QT_INSTALL_PLUGINS" ]; then
    echo "\${CLEAN_DIR}"
    exit 0
fi

exec "\${REAL_QMAKE}" "\$@"
EOF
        chmod +x "${QMAKE_WRAPPER}"
        export QMAKE="${QMAKE_WRAPPER}"
    else
        export QMAKE="${REAL_QMAKE}"
    fi
fi

# Detect available Wayland platform plugins dynamically
WAYLAND_PLUGINS=""
PLATFORMS_DIR=""
if [ -n "${CLEAN_PLUGINS:-}" ] && [ -d "${CLEAN_PLUGINS}/platforms" ]; then
    PLATFORMS_DIR="${CLEAN_PLUGINS}/platforms"
elif [ -n "${QT_ORIG_PLUGINS:-}" ] && [ -d "${QT_ORIG_PLUGINS}/platforms" ]; then
    PLATFORMS_DIR="${QT_ORIG_PLUGINS}/platforms"
fi

if [ -n "${PLATFORMS_DIR}" ]; then
    for p in "${PLATFORMS_DIR}"/libqwayland*.so; do
        if [ -f "$p" ]; then
            plugin_name="$(basename "$p")"
            if [ -z "${WAYLAND_PLUGINS}" ]; then
                WAYLAND_PLUGINS="${plugin_name}"
            else
                WAYLAND_PLUGINS="${WAYLAND_PLUGINS};${plugin_name}"
            fi
        fi
    done
fi

if [ -n "${WAYLAND_PLUGINS}" ]; then
    export EXTRA_PLATFORM_PLUGINS="${WAYLAND_PLUGINS}"
    echo "--> Detected Wayland platform plugins: ${EXTRA_PLATFORM_PLUGINS}"
else
    unset EXTRA_PLATFORM_PLUGINS 2>/dev/null || true
    echo "--> No Wayland platform plugins detected; continuing with default xcb"
fi

# Run linuxdeploy with Qt plugin
echo "--> Bundling Qt and dynamic dependencies with linuxdeploy..."
"${LINUXDEPLOY}" \
    --appdir "${APP_DIR}" \
    --executable "${APP_DIR}/usr/bin/PicAudio" \
    --desktop-file "${APP_DIR}/usr/share/applications/picaudio.desktop" \
    --icon-file "${APP_DIR}/usr/share/icons/hicolor/scalable/apps/picaudio.svg" \
    --plugin qt

# 5. Bundle core multimedia, archive, and storage runtime libraries
echo "--> Bundling core engine libraries (FFmpeg, LibArchive, SQLite3, TagLib)..."
mkdir -p "${APP_DIR}/usr/lib"
for libprefix in "libavformat" "libavcodec" "libswresample" "libavutil" "libarchive" "libtag" "libsqlite3"; do
    found="$(ldconfig -p | grep -E "${libprefix}\.so\.[0-9]+" | awk '{print $NF}' | head -n1 || true)"
    if [ -n "${found}" ] && [ -f "${found}" ]; then
        echo "    Bundling ${found}..."
        cp -uL "${found}"* "${APP_DIR}/usr/lib/" 2>/dev/null || true
    fi
done

# Patch RPATH on all bundled libraries in AppDir
if command -v patchelf &>/dev/null || [ -f "${TOOLS_DIR}/patchelf" ]; then
    PATCHELF_CMD="$(command -v patchelf || echo "${TOOLS_DIR}/patchelf")"
    echo "--> Patching library RPATHs with patchelf..."
    for lib in "${APP_DIR}/usr/lib"/*.so*; do
        if [ -f "$lib" ] && [ ! -L "$lib" ]; then
            "${PATCHELF_CMD}" --set-rpath '$ORIGIN:$ORIGIN/../lib' "$lib" 2>/dev/null || true
        fi
    done
fi

# Bundle Wayland shell and platform integrations
echo "--> Bundling Wayland shell and graphics integrations..."
mkdir -p "${APP_DIR}/usr/plugins/platforms"
if [ -n "${PLATFORMS_DIR}" ]; then
    for p in "${PLATFORMS_DIR}"/libqwayland*.so; do
        if [ -f "$p" ]; then
            cp -f "$p" "${APP_DIR}/usr/plugins/platforms/"
        fi
    done
fi

if [ -n "${QT_ORIG_PLUGINS:-}" ] && [ -d "${QT_ORIG_PLUGINS}" ]; then
    for w in "${QT_ORIG_PLUGINS}"/wayland-*; do
        if [ -d "$w" ]; then
            cp -rf "$w" "${APP_DIR}/usr/plugins/"
        fi
    done
fi

# Remove system crypto libraries that cause TLS/dlopen segmentation faults on modern distros
rm -f "${APP_DIR}/usr/lib/libleancrypto"* "${APP_DIR}/usr/lib/libcrypt."* 2>/dev/null || true

# 6. Create custom AppRun launcher with Wayland/X11 fallback
echo "--> Setting up AppRun launcher..."
rm -f "${APP_DIR}/AppRun" "${APP_DIR}/AppRun.wrapped"
cat << 'EOF' > "${APP_DIR}/AppRun"
#!/usr/bin/env bash
set -e

HERE="$(dirname "$(readlink -f "${0}")")"
export APPDIR="${HERE}"

export PATH="${HERE}/usr/bin:${PATH}"
export LD_LIBRARY_PATH="${HERE}/usr/lib:${HERE}/usr/lib/x86_64-linux-gnu:${LD_LIBRARY_PATH:-}"
export QT_PLUGIN_PATH="${HERE}/usr/plugins"
export QML2_IMPORT_PATH="${HERE}/usr/qml"

# Graceful platform fallback: try Wayland first if session is Wayland, fallback to XCB/X11
if [ -z "${QT_QPA_PLATFORM:-}" ]; then
    export QT_QPA_PLATFORM="wayland;xcb"
fi

# Source all AppRun hooks (e.g. from linuxdeploy plugins)
if [ -d "${HERE}/apprun-hooks" ]; then
    for hook in "${HERE}"/apprun-hooks/*; do
        if [ -f "$hook" ]; then
            source "$hook"
        fi
    done
fi

exec "${HERE}/usr/bin/PicAudio" "$@"
EOF
chmod +x "${APP_DIR}/AppRun"

# 7. Generate final AppImage using appimagetool with .zsync delta update metadata
echo "--> Packaging final AppImage with appimagetool..."
VERSION="${PICAUDIO_VERSION:-$(grep -m1 'project(PicAudio VERSION' "${REPO_ROOT}/CMakeLists.txt" | sed -E 's/.*VERSION[[:space:]]+([0-9.]+).*/\1/')}"
ARCH="x86_64"
UPDATE_REPO="${GITHUB_REPOSITORY:-ShamalLakshan/PicAudio}"
UPDATE_CHANNEL="${PICAUDIO_UPDATE_CHANNEL:-latest}"
UPINFO="gh-releases-zsync|${UPDATE_REPO%/*}|${UPDATE_REPO#*/}|${UPDATE_CHANNEL}|PicAudio-*-$ARCH.AppImage.zsync"
export UPINFO

FINAL_APPIMAGE="${OUTPUT_DIR}/PicAudio-${VERSION}-${ARCH}.AppImage"
STANDARD_APPIMAGE="${OUTPUT_DIR}/PicAudio-${ARCH}.AppImage"

ARCH="${ARCH}" "${TOOLS_DIR}/appimagetool-prefix/AppRun" -u "${UPINFO}" "${APP_DIR}" "${FINAL_APPIMAGE}"
cp -f "${FINAL_APPIMAGE}" "${STANDARD_APPIMAGE}"
if [ -f "${FINAL_APPIMAGE}.zsync" ]; then
    cp -f "${FINAL_APPIMAGE}.zsync" "${STANDARD_APPIMAGE}.zsync"
fi

# Generate SHA256 checksums
(
    cd "${OUTPUT_DIR}"
    sha256sum "$(basename "${FINAL_APPIMAGE}")" > "$(basename "${FINAL_APPIMAGE}").sha256"
    sha256sum "$(basename "${STANDARD_APPIMAGE}")" > "$(basename "${STANDARD_APPIMAGE}").sha256"
)

if [ -f "${FINAL_APPIMAGE}" ]; then
    echo "=== AppImage created successfully! ==="
    echo "Versioned AppImage: ${FINAL_APPIMAGE}"
    echo "Standard AppImage:  ${STANDARD_APPIMAGE}"
    ls -lh "${OUTPUT_DIR}"/PicAudio-*.AppImage*
else
    echo "ERROR: PicAudio AppImage was not generated in ${OUTPUT_DIR}" >&2
    exit 1
fi

