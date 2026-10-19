# PicAudio Theming Guide & Design Token Specification

PicAudio features a powerful, reactive, JSON-driven design token theming engine.
Themes customize every visual aspect of the studio interface—from panel backgrounds and accent colors to typography scales, border radii, and padding.

Themes can be hot-reloaded at runtime: modifying a theme's JSON file immediately updates the live UI without requiring an application restart.

---

## 1. Theme Storage Locations

Themes are discovered automatically on application launch and dynamically monitored for file changes:

- **Built-in System Themes**:
  - `dark-studio` (Default Dark Studio Theme)
  - `nord-audiophile` (Arctic Nord Theme)
  - `solarized-dark` (Warm Low-Contrast Theme)
- **User Installed Themes**:
  - Linux: `~/.local/share/PicAudio/themes/*.json` (or `~/.local/share/PicAudio/PicAudio/themes/*.json`)
  - Windows: `%APPDATA%/PicAudio/PicAudio/themes/*.json`
  - macOS: `~/Library/Application Support/PicAudio/PicAudio/themes/*.json`

You can open this folder directly inside PicAudio from **Preferences > Themes & Appearance > Open Themes Folder**.

---

## 2. Complete Worked Example: `nord-audiophile.json`

```json
{
  "meta": {
    "id": "nord-audiophile",
    "name": "Nord Audiophile",
    "author": "PicAudio Community",
    "version": "1.0.0",
    "apiVersion": "0.1-unstable",
    "description": "An arctic, north-bluish clean palette tailored for nighttime listening"
  },
  "colors": {
    "background": "#2e3440",
    "surface": "#3b4252",
    "surfaceElevated": "#434c5e",
    "panelBorder": "#4c566a",
    "accent": "#88c0d0",
    "accentHover": "#8fbcbb",
    "textPrimary": "#eceff4",
    "textSecondary": "#d8dee9",
    "textMuted": "#7e889b",
    "selection": "#4c566a",
    "error": "#bf616a",
    "warning": "#ebcb8b",
    "success": "#a3be8c"
  },
  "typography": {
    "fontFamily": "Inter, -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif",
    "fontSizeSmall": 10,
    "fontSizeBase": 12,
    "fontSizeLarge": 14,
    "fontSizeTitle": 18,
    "fontWeightScale": 1.0,
    "lineHeightScale": 1.2
  },
  "metrics": {
    "spacingSmall": 4,
    "spacingMedium": 8,
    "spacingLarge": 16,
    "cornerRadiusSmall": 3,
    "cornerRadiusMedium": 6,
    "panelPadding": 12
  }
}
```

---

## 3. Token Glossary

### `meta`
- `id`: Unique lowercase identifier used by the preferences engine (e.g. `nord-audiophile`).
- `name`: User-facing title rendered in theme selector cards.
- `author`: Name or GitHub username of author.
- `version`: SemVer string (e.g. `1.0.0`).
- `apiVersion`: Version of PicAudio theming engine (currently `0.1-unstable`).
- `description`: Short summary describing the visual aesthetic.

### `colors`
- `background`: Darkest base background color for the application canvas.
- `surface`: Color of primary content panels (tables, grids, sidebars).
- `surfaceElevated`: Color of floating cards, active row selection, and transport deck.
- `panelBorder`: Subtle 1px structural dividing lines between multi-pane studio layouts.
- `accent`: Primary brand highlight color used for active playing tracks, sliders, and focused tabs.
- `accentHover`: Hover state color when pointing at buttons and interactive controls.
- `textPrimary`: High-contrast foreground color for track titles and prominent headings.
- `textSecondary`: Medium-contrast color for artist names and metadata details.
- `textMuted`: Low-contrast color for timestamps, disabled states, and technical labels.
- `selection`: Highlight background color for selected table rows and item multi-selection.
- `error`: Color for missing audio files, corrupt tags, or I/O errors.
- `warning`: Color for clipping indicators, non-bit-perfect resampling alerts.
- `success`: Color for bit-perfect output lock and successful library index scans.

### `typography`
- `fontFamily`: CSS/Qt font family stack.
- `fontSizeSmall`: Technical specs, timestamps, and column header labels (default 9-10pt).
- `fontSizeBase`: Standard track table rows, artist text, menu items (default 11-12pt).
- `fontSizeLarge`: Section headings, inspector titles (default 14-16pt).
- `fontSizeTitle`: Album expanded title, hero headers (default 18-22pt).
- `fontWeightScale`: Global multiplier applied to font weights.
- `lineHeightScale`: Global multiplier for vertical text rhythm.

### `metrics`
- `spacingSmall`: Compact margins between tightly packed widgets (default 4px).
- `spacingMedium`: Standard layout padding between elements (default 8-10px).
- `spacingLarge`: Panel gaps and large dialog section separations (default 16-24px).
- `cornerRadiusSmall`: Button and tag badge corner radius (default 2-3px).
- `cornerRadiusMedium`: Dialog window, card, and cover art corner radius (default 4-8px).
- `panelPadding`: Internal margins for docked inspector panels and sidebars (default 10-14px).

---

## 4. Troubleshooting: Why Didn't Your Theme Load?

If a third-party theme fails to load, PicAudio **never crashes**. Instead, it logs an informative diagnostic warning and falls back safely to the built-in default **Dark Studio** theme.

Common failure causes:
1. **Malformed JSON Syntax**:
   - Trailing commas are invalid in standard JSON: `{"color": "#fff",}` will fail parsing.
   - Run `jq . your-theme.json` to verify JSON syntax.
2. **Missing Required Color Keys**:
   - Every key listed in the schema under `required` must be present. If any color is omitted, validation fails.
3. **Invalid Hex Color Format**:
   - Hex colors must be valid `#RRGGBB` or `#RRGGBBAA` strings (e.g. `#121214`).
4. **Invalid Value Bounds**:
   - Font sizes and spacing values must conform to the minimum and maximum boundaries defined in `docs/theme-schema.json`.
