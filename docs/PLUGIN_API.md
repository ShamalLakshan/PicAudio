# PicAudio Plugin Developer Guide & SDK API Reference

This document describes how to create, build, and distribute dynamic extensions for PicAudio using the public C-ABI Plugin SDK.

---

## 1. Overview & Architecture

PicAudio uses a compiler-agnostic C ABI boundary for runtime dynamic plugin loading. This ensures binary compatibility even if third-party plugins are compiled with different toolchains or standard library versions than the host application.

All dynamic plugins are packaged as self-contained directories containing:
1. `plugin.json`: Metadata manifest defining capabilities, identity, and version.
2. Shared dynamic library: `.so` (Linux), `.dll` (Windows), or `.dylib` (macOS).

### SDK Include Headers

Public SDK headers are located in `sdk/include/picaudio/plugin/`:
- `Defines.hpp`: Export macros, API version definitions, and POD structures.
- `IPlugin.hpp`: Base plugin lifecycle interface and factory function types.
- `IVisualizerPlugin.hpp`: Real-time audio PCM buffer and frequency spectrum interface.
- `ILibraryPlugin.hpp`: Lyrics fetching, metadata enrichment, and scrobbling interface.
- `IPlayerPlugin.hpp`: Pre/post DSP audio processing hooks and playback event callbacks.
- `IThemePlugin.hpp`: Dynamic runtime UI theme tokens and color override callbacks.
- `IUiPlugin.hpp`: Custom popup windows, preference card buttons, and host UI navigation callbacks.

Compatibility headers are also provided under `sdk/include/parakeet/plugin/` for backward compatibility.

---

## 2. Plugin Manifest (`plugin.json`)

Every plugin directory must contain a valid `plugin.json` adhering to `docs/plugin-schema.json`.

```json
{
  "id": "com.example.myplugin",
  "name": "My Custom Plugin",
  "version": "1.0.0",
  "author": "Jane Developer",
  "description": "Custom visualizer and scrobbler for PicAudio.",
  "homepage": "https://example.com/myplugin",
  "targetApiVersion": "1.0.0",
  "capabilities": [
    "visualizer",
    "scrobbler"
  ]
}
```

### Manifest Fields

| Field | Type | Required | Description |
|---|---|---|---|
| `id` | string | Yes | Unique reverse-DNS identifier (e.g. `com.example.myplugin`). |
| `name` | string | Yes | Display name shown in preferences. |
| `version` | string | Yes | Plugin semantic version string (e.g. `1.0.0`). |
| `author` | string | Yes | Author or organization name. |
| `description` | string | Yes | Brief description of functionality. |
| `homepage` | string | No | Website or repository URL. |
| `targetApiVersion` | string | Yes | Targeted PicAudio SDK version (e.g. `1.0.0`). |
| `capabilities` | array | Yes | List of implemented capabilities: `"visualizer"`, `"lyrics"`, `"scrobbler"`, `"dsp"`, `"library"`, `"theme"`, `"ui"`. |
| `actions` | array | No | Custom action buttons rendered on the plugin's card in Preferences (`id`, `label`, `tooltip`, `iconName`, `isPrimary`). |
| `entryPoint` | string | No | Custom shared library file name. If omitted, PicAudio auto-detects libraries in the directory. |
| `minHostVersion` | string | No | Minimum supported PicAudio player version. |

---

## 3. Plugin Lifecycle & Entry Points

Each shared library must export two `extern "C"` functions:

```cpp
extern "C" {
    PICAUDIO_PLUGIN_EXPORT picaudio::plugin::IPicAudioPlugin* create_plugin();
    PICAUDIO_PLUGIN_EXPORT void destroy_plugin(picaudio::plugin::IPicAudioPlugin* plugin);
}
```

### Lifecycle Sequence

1. **Load**: Host loads dynamic library and resolves `create_plugin()`.
2. **`initialize()`**: Invoked once to allocate internal buffers and state. Return `true` on success.
3. **`start()`**: Invoked when the plugin is activated/enabled.
4. **Capability Calls**: Host calls interface methods (`processAudioBuffer`, `fetchLyrics`, etc.).
5. **`stop()`**: Invoked when the plugin is disabled or prior to shutdown.
6. **`shutdown()`**: Releases all resources and thread pools.
7. **`destroy_plugin()`**: Destroys the plugin instance via `delete`.

---

## 4. Capability Interfaces

### IVisualizerPlugin

Implemented for spectrum analyzers, VU meters, and waveforms.

```cpp
class IVisualizerPlugin {
public:
    virtual ~IVisualizerPlugin() = default;

    virtual void processAudioBuffer(const float* buffer, size_t frames,
                                   uint32_t sampleRate, uint8_t channels) = 0;

    virtual size_t getFrequencyBands(float* outBands, size_t maxBands) const = 0;

    virtual bool isEnabled() const noexcept = 0;
    virtual void setEnabled(bool enabled) = 0;
};
```

### ILibraryPlugin

Implemented for metadata providers, synchronized lyrics, and scrobblers.

```cpp
class ILibraryPlugin {
public:
    virtual ~ILibraryPlugin() = default;

    virtual bool enrichMetadata(const PicAudioTrackInfo* inTrack,
                                PicAudioTrackInfo* outEnriched);

    virtual size_t fetchLyrics(const char* artist, const char* title,
                               char* outBuffer, size_t maxLen);

    virtual void onTrackScrobbled(const PicAudioTrackInfo* track, uint64_t timestamp);

    virtual void onTrackIndexed(const PicAudioTrackInfo* track);
};
```

### IPlayerPlugin

Implemented for real-time DSP effects (equalizers, compressors) and transport listeners.

```cpp
class IPlayerPlugin {
public:
    virtual ~IPlayerPlugin() = default;

    virtual void onPreDsp(float* buffer, size_t frames, uint32_t sampleRate, uint8_t channels);
    virtual void onPostDsp(float* buffer, size_t frames, uint32_t sampleRate, uint8_t channels);
    virtual void onTrackStarted(const PicAudioTrackInfo* track);
    virtual void onPlaybackStateChanged(PicAudioPlaybackState state);
};
```

### IThemePlugin

Implemented for dynamic theming, runtime palette customization, and desktop environment color synchronization (e.g., Pywal, desktop wallpaper accent extraction, platform accent sync).

```cpp
class IThemePlugin {
public:
    virtual ~IThemePlugin() = default;

    /**
     * @brief Registers the host callback allowing the plugin to push dynamic color updates at runtime.
     * @param hostContext Opaque pointer passed back in callback invocations.
     * @param callback Function to call when theme colors change.
     */
    virtual void setHostThemeCallback(void* hostContext, PicAudioThemeUpdateCallback callback);

    /**
     * @brief Query active theme color overrides provided by this plugin.
     * @param outTokens Caller buffer to populate with key-value color pairs.
     * @param maxTokens Maximum capacity of outTokens.
     * @return Number of tokens written.
     */
    virtual size_t getThemeOverrides(PicAudioColorToken* outTokens, size_t maxTokens);

    /**
     * @brief Called when the host switches base themes.
     * @param themeId Base theme identifier.
     */
    virtual void onBaseThemeChanged(const char* themeId);
};
```

#### Modifiable Color Tokens
Plugins can dynamically override any standard UI color token recognized by PicAudio:

| Token Key | Description |
|---|---|
| `background` | Primary application window and panel background. |
| `surface` | Surface background for lists, cards, and views. |
| `surfaceElevated` | Elevated card surfaces, popups, and floating decks. |
| `panelBorder` | Border separators between sidebars, viewport, and decks. |
| `accent` | Main accent color (playback progress, active buttons, focus). |
| `accentHover` | Hover/pressed accent tint. |
| `textPrimary` | High-contrast body and heading text. |
| `textSecondary` | Subdued secondary metadata (artist, album, duration). |
| `textMuted` | Low-contrast tertiary text (track numbers, file formats). |
| `selection` | Item selection highlight background. |
| `error` | Error badges and failure alerts. |
| `warning` | Warning notifications and caution chips. |
| `success` | Success banners and confirmations. |

### IUiPlugin

Implemented for plugins that provide custom popup windows, standalone dialogs, or customizable action buttons rendered directly on their Preferences card.

Third-party developers packaging plugins for pre-built binaries (AppImage, Portable) use `IUiPlugin` to expose custom controls without modifying the host C++ or QML code.

```cpp
class IUiPlugin {
public:
    virtual ~IUiPlugin() = default;

    /**
     * @brief Registers host-provided UI services (navigation, toasts) with the plugin.
     * @param services Pointer to host UI services table.
     */
    virtual void setHostUiServices(const PicAudioHostUiServices* services);

    /**
     * @brief Query custom action buttons that should appear on the plugin's preferences card.
     * @param outActions Caller-allocated buffer to populate with action descriptors.
     * @param maxActions Maximum capacity of outActions buffer.
     * @return Number of custom actions populated.
     */
    virtual size_t getCustomActions(PicAudioCustomAction* outActions, size_t maxActions);

    /**
     * @brief Invoked when a user clicks one of the plugin's custom action buttons.
     * @param actionId Action identifier matching PicAudioCustomAction::id.
     * @return true if action was successfully handled, false otherwise.
     */
    virtual bool triggerAction(const char* actionId);

    /**
     * @brief Requests the plugin to display its standalone popup window or dialog.
     * @return true if window was displayed, false if unsupported or failed.
     */
    virtual bool showWindow();

    /**
     * @brief Requests the plugin to hide its standalone popup window.
     * @return true if window was hidden, false if unsupported.
     */
    virtual bool hideWindow();

    /**
     * @brief Queries whether the plugin's popup window is currently visible.
     */
    [[nodiscard]] virtual bool isWindowVisible() const;
};
```

#### Host UI Services (`PicAudioHostUiServices`)

The host provides C-ABI callbacks to the plugin via `setHostUiServices()`:
- `navigateToView(void* hostContext, int viewMode)`: Request the player workspace to navigate to a specific view (e.g., Now Playing / Visualizer).
- `showToast(void* hostContext, const char* message, const char* level)`: Display an in-app non-blocking toast notification banner (`"info"`, `"success"`, `"warning"`, `"error"`).

---

## 5. Minimal Worked Example

### `MyVisualizerPlugin.cpp`

```cpp
#include <picaudio/plugin/Defines.hpp>
#include <picaudio/plugin/IPlugin.hpp>
#include <picaudio/plugin/IVisualizerPlugin.hpp>
#include <algorithm>
#include <vector>

namespace {

class MyVisualizerPlugin final : public picaudio::plugin::IPicAudioPlugin,
                                 public picaudio::plugin::IVisualizerPlugin {
public:
    [[nodiscard]] const char* getId() const noexcept override { return "com.example.visualizer"; }
    [[nodiscard]] const char* getName() const noexcept override { return "Simple Visualizer"; }
    [[nodiscard]] const char* getVersion() const noexcept override { return "1.0.0"; }
    [[nodiscard]] const char* getAuthor() const noexcept override { return "Community"; }
    [[nodiscard]] const char* getDescription() const noexcept override {
        return "Example audio visualizer plugin.";
    }

    [[nodiscard]] uint32_t getCapabilities() const noexcept override {
        return picaudio::plugin::PICAUDIO_CAP_VISUALIZER;
    }

    bool initialize() override {
        m_bands.assign(32, 0.0f);
        m_enabled = true;
        return true;
    }

    void shutdown() override {}

    void* getCapabilityInterface(uint32_t capability) override {
        if (capability == picaudio::plugin::PICAUDIO_CAP_VISUALIZER) {
            return static_cast<picaudio::plugin::IVisualizerPlugin*>(this);
        }
        return nullptr;
    }

    void processAudioBuffer(const float* buffer, size_t frames,
                            uint32_t sampleRate, uint8_t channels) override {
        (void)sampleRate;
        if (!buffer || frames == 0 || channels == 0) return;

        // Simple RMS band calculation
        float energy = 0.0f;
        for (size_t i = 0; i < frames * channels; ++i) {
            energy += std::abs(buffer[i]);
        }
        energy /= static_cast<float>(frames * channels);

        for (size_t b = 0; b < m_bands.size(); ++b) {
            m_bands[b] = std::clamp(energy * (1.0f + static_cast<float>(b) * 0.1f), 0.0f, 1.0f);
        }
    }

    size_t getFrequencyBands(float* outBands, size_t maxBands) const override {
        const size_t count = std::min(maxBands, m_bands.size());
        for (size_t i = 0; i < count; ++i) {
            outBands[i] = m_bands[i];
        }
        return count;
    }

    [[nodiscard]] bool isEnabled() const noexcept override { return m_enabled; }
    void setEnabled(bool enabled) override { m_enabled = enabled; }

private:
    bool m_enabled{true};
    std::vector<float> m_bands;
};

} // namespace

extern "C" {

PICAUDIO_PLUGIN_EXPORT picaudio::plugin::IPicAudioPlugin* create_plugin() {
    return new (std::nothrow) MyVisualizerPlugin();
}

PICAUDIO_PLUGIN_EXPORT void destroy_plugin(picaudio::plugin::IPicAudioPlugin* plugin) {
    delete plugin;
}

}
```

### `CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.20)
project(MyVisualizerPlugin VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find PicAudio SDK or link if part of tree
if(NOT TARGET picaudio_sdk)
    find_package(PicAudioSdk REQUIRED)
endif()

add_library(my_visualizer SHARED
    MyVisualizerPlugin.cpp
)

target_compile_definitions(my_visualizer PRIVATE PICAUDIO_PLUGIN_BUILD)
target_link_libraries(my_visualizer PRIVATE picaudio_sdk)

set_target_properties(my_visualizer PROPERTIES
    PREFIX ""
    OUTPUT_NAME "my_visualizer"
)
```

---

## 6. Installation & Search Paths

PicAudio automatically scans the following locations for plugin directories:

| OS | Discovery Directory Paths |
|---|---|
| **Linux** | `~/.local/share/PicAudio/plugins/`<br>`~/.local/share/ParakeetAudio/Parakeet/plugins/`<br>`<app_dir>/plugins/` |
| **Windows** | `%APPDATA%\PicAudio\plugins\`<br>`<app_dir>\plugins\` |
| **macOS** | `~/Library/Application Support/PicAudio/plugins/`<br>`<app_dir>/plugins/` |
| **Custom** | Directory path specified in `PICAUDIO_PLUGIN_DIR` environment variable |

---

## 7. User Guide: Installing & Attaching Plugins

End users can easily attach third-party plugins directly through the PicAudio interface without editing configuration files:

1. Open **Preferences** (`Ctrl+,` or **Edit → Preferences** from the menu bar).
2. Select the **Plugins & Extensions** tab on the left.
3. Click the **"Open Plugins Directory"** button. This automatically opens the correct operating system plugin folder in your native file manager (Nautilus, Dolphin, Windows Explorer, or macOS Finder).
4. Extract or copy the plugin folder (which contains `plugin.json` and the `.so` / `.dll` / `.dylib` library) into this directory.
5. Return to PicAudio and click **"Rescan Plugins"**.
6. The new plugin will appear in the list with its author, description, and API compatibility status. Use the **Enabled / Disabled** toggle to activate or deactivate it instantly.
7. Click **"Configure..."** on any plugin card to view detailed manifest information, active capabilities, and load diagnostics.

---

## 8. Dynamic Theming Integration (`IThemePlugin`)

PicAudio allows plugins to dynamically control and override application theme tokens at runtime. This enables integrations such as:
- **Pywal & Matugen Palette Sync**: Reading generated terminal/desktop color schemes and updating PicAudio instantly without restart.
- **Dynamic Accent Extraction**: Synchronizing player colors with the OS desktop accent or current album artwork.
- **Custom Aesthetic Schemes**: Providing external dynamic shaders, high-contrast accessibility profiles, or time-of-day tints.

### How Theme Overrides Work

1. **Advertise Capability**: Declare `"theme"` in `plugin.json` under `capabilities` and return `picaudio::plugin::PICAUDIO_CAP_THEME` from `getCapabilities()`.
2. **Initial Query**: When enabled, the host calls `getThemeOverrides()` to retrieve initial color key-value pairs and applies them to the active theme palette.
3. **Live Push Callback**: The host invokes `setHostThemeCallback(hostContext, callback)`. The plugin can call this callback at any time (e.g., when a file watcher detects that `~/.cache/wal/colors.json` has updated) to push fresh color tokens immediately.
4. **Base Theme Change**: If the user switches base themes in preferences (e.g., Dark Studio to Light), the host invokes `onBaseThemeChanged(themeId)`, giving the plugin an opportunity to re-adjust its palette.

### Minimal Theme Plugin Example

```cpp
#include <picaudio/plugin/Defines.hpp>
#include <picaudio/plugin/IPlugin.hpp>
#include <picaudio/plugin/IThemePlugin.hpp>
#include <array>

namespace {

class DynamicThemePlugin final : public picaudio::plugin::IPicAudioPlugin,
                                 public picaudio::plugin::IThemePlugin {
public:
    [[nodiscard]] const char* getId() const noexcept override { return "com.example.waltheme"; }
    [[nodiscard]] const char* getName() const noexcept override { return "Dynamic Wal Theme"; }
    [[nodiscard]] const char* getVersion() const noexcept override { return "1.0.0"; }
    [[nodiscard]] const char* getAuthor() const noexcept override { return "Community"; }
    [[nodiscard]] const char* getDescription() const noexcept override {
        return "Syncs player colors with external Pywal palette.";
    }

    [[nodiscard]] uint32_t getCapabilities() const noexcept override {
        return picaudio::plugin::PICAUDIO_CAP_THEME;
    }

    bool initialize() override { return true; }
    void shutdown() override { m_callback = nullptr; }

    void* getCapabilityInterface(uint32_t capability) override {
        if (capability == picaudio::plugin::PICAUDIO_CAP_THEME) {
            return static_cast<picaudio::plugin::IThemePlugin*>(this);
        }
        return nullptr;
    }

    void setHostThemeCallback(void* hostContext,
                              picaudio::plugin::PicAudioThemeUpdateCallback callback) override {
        m_hostContext = hostContext;
        m_callback = callback;
    }

    size_t getThemeOverrides(picaudio::plugin::PicAudioColorToken* outTokens,
                             size_t maxTokens) override {
        if (!outTokens || maxTokens < 3) return 0;

        outTokens[0] = {"background", "#0f141c"};
        outTokens[1] = {"surface", "#151b26"};
        outTokens[2] = {"accent", "#e06c75"};
        return 3;
    }

    void onBaseThemeChanged(const char* themeId) override {
        // Optionally adjust palette depending on whether themeId is light or dark
        (void)themeId;
    }

    // Called when external palette file changes
    void pushNewPalette(const char* bg, const char* surface, const char* accent) {
        if (!m_callback || !m_hostContext) return;

        std::array<picaudio::plugin::PicAudioColorToken, 3> tokens{{
            {"background", bg},
            {"surface", surface},
            {"accent", accent}
        }};
        m_callback(m_hostContext, tokens.data(), tokens.size());
    }

private:
    void* m_hostContext{nullptr};
    picaudio::plugin::PicAudioThemeUpdateCallback m_callback{nullptr};
};

} // namespace

extern "C" {
PICAUDIO_PLUGIN_EXPORT picaudio::plugin::IPicAudioPlugin* create_plugin() {
    return new (std::nothrow) DynamicThemePlugin();
}
PICAUDIO_PLUGIN_EXPORT void destroy_plugin(picaudio::plugin::IPicAudioPlugin* plugin) {
    delete plugin;
}
}
```

