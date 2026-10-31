# PicAudio Visual Themes

PicAudio features a dynamic, community-driven JSON theme engine with hot-reloading support. This directory contains bundled base themes and serves as a reference for creating and sharing custom themes.

---

## Bundled Themes

| Theme ID | Name | Style | Description |
|---|---|---|---|
| `dark-studio` | **Dark Studio** | Dark | Flagship dark theme engineered for studio listening environments |
| `nord-audiophile` | **Nord** | Dark (Cool) | Arctic, north-bluish clean palette tailored for night sessions |
| `solarized-dark` | **Solarized Dark** | Dark (Warm) | Low-contrast palette calibrated for eye comfort |
| `dracula` | **Dracula** | Dark (High Contrast) | Vibrant purple and cyan accents on deep slate background |
| `solarized-light` | **Solarized Light** | Light (Warm) | Warm daytime palette designed for bright listening rooms |

---

## Creating a Community Theme

Every theme is defined as a standalone `.json` file that complies with [`docs/theme-schema.json`](../docs/theme-schema.json).

### Theme File Structure

```json
{
  "$schema": "https://picaudio.org/schemas/theme-v1.json",
  "meta": {
    "id": "my-custom-theme",
    "name": "My Custom Theme",
    "author": "Your Name",
    "version": "1.0.0",
    "apiVersion": "0.1-unstable",
    "description": "Short summary of the aesthetic"
  },
  "colors": {
    "background": "#000000",
    "surface": "#0d0d0d",
    "surfaceElevated": "#171717",
    "panelBorder": "#262626",
    "accent": "#ffffff",
    "accentHover": "#e5e5e5",
    "textPrimary": "#ffffff",
    "textSecondary": "#a3a3a3",
    "textMuted": "#737373",
    "selection": "#262626",
    "error": "#ef4444",
    "warning": "#f59e0b",
    "success": "#22c55e"
  },
  "typography": {
    "fontFamily": "Inter, sans-serif",
    "fontSizeSmall": 9,
    "fontSizeBase": 11,
    "fontSizeLarge": 14,
    "fontSizeTitle": 18,
    "fontWeightScale": 1.0,
    "lineHeightScale": 1.2
  },
  "metrics": {
    "spacingSmall": 4,
    "spacingMedium": 8,
    "spacingLarge": 16,
    "cornerRadiusSmall": 2,
    "cornerRadiusMedium": 4,
    "panelPadding": 10
  }
}
```

---

## Theme Discovery & Search Paths

PicAudio discovers themes dynamically at startup and watches for changes in real time. Theme search paths are scanned in the following priority order (higher overrides lower):

1. **User Themes Directory** *(Highest priority; allows overriding bundled themes)*:
   - **Linux**: `~/.local/share/PicAudio/themes/`
   - **macOS**: `~/Library/Application Support/PicAudio/themes/`
   - **Windows**: `%APPDATA%/PicAudio/themes/`
2. **System & Application Directories**:
   - `<install-prefix>/share/picaudio/themes/`
   - `<binary-directory>/themes/`
   - Current working directory `./themes/`
3. **Embedded Qt Resources** *(Fail-safe bundled fallback)*:
   - `:/themes/*.json`

---

## Real-Time Hot Reloading

When developing themes, any modification to a `.json` file in a watched directory immediately reloads the theme tokens and live-updates PicAudio's UI without restarting the application.
