# Library, Playlists & Metadata Management

PicAudio provides a robust, high-performance music library system built on hexagonal architecture principles, backed by SQLite with full-text search indices, real-time filesystem monitoring, and TagLib metadata synchronization.

---

## Static Playlists (M3U, M3U8, PLS, XSPF)

The static playlist engine (`PlaylistService`) supports standard open playlist formats:
- **M3U / M3U8**: Standard and extended `#EXTM3U` playlists with UTF-8 character encoding. Includes `#EXTINF:<duration>,<artist> - <title>` directives.
- **PLS**: Standard INI-style playlist format with `[playlist]`, `FileN`, `TitleN`, and `LengthN` entries.
- **XSPF**: XML Shareable Playlist Format (`<playlist><trackList><track><location>...</location></track></trackList></playlist>`).

### Path Handling
- Paths are normalized across operating systems (converting `file://` URIs and relative paths relative to the playlist container directory).
- Reordering tracks preserves exact playback sequences (`moveItem`).
- Playlists can be exported directly from the UI or imported via standard file dialogs or drag-and-drop.

---

## Smart Auto-Playlists

Smart Auto-Playlists dynamically filter the database without manual curation. Rules are saved in JSON format and executed as parameterized SQLite queries:

### Rule Schema
```json
{
  "match": "all",
  "rules": [
    {
      "field": "rating",
      "operator": "gte",
      "value": "4"
    },
    {
      "field": "play_count",
      "operator": "gt",
      "value": "5"
    }
  ]
}
```

### Supported Fields
- `title`, `artist`, `album`, `genre`, `comment`: Text matching (`contains`, `is`, `starts_with`, `ends_with`).
- `year`, `rating`, `play_count`, `bit_depth`, `sample_rate`: Numeric matching (`eq`, `neq`, `gt`, `gte`, `lt`, `lte`).
- `is_favorite`: Boolean matching (`is_true`, `is_false`).

### Built-in Smart Playlists
- **Recently Added**: Tracks added within the last 30 days sorted descending by `date_added`.
- **Top Rated**: Tracks with rating >= 4 stars.
- **Most Played**: Tracks with `play_count > 0` sorted descending by `play_count`.
- **Favorites**: All tracks flagged as favorite.
- **Hi-Res Audio**: Tracks with `bit_depth >= 24` or `sample_rate >= 88200` Hz.

---

## 5-Star Track Ratings & Favorites Heart

PicAudio integrates 5-star ratings and favorite status directly across all views:
- **Interactive Controls**: Clickable star widgets and heart icons in Track Table rows, Now Playing views, Player Deck, and Right Inspector panel.
- **Tag Synchronization**:
  - When enabled in **Preferences > Library**, rating changes are written back to audio file tags on disk.
  - **ID3v2 (MP3, AIFF)**: Stored in the `POPM` (Popularimeter) frame, mapped to standard ratings (1 star = 1, 2 stars = 64, 3 stars = 128, 4 stars = 196, 5 stars = 255).
  - **Vorbis Comments (FLAC, Ogg, Opus)**: Stored in `RATING` (1–5) and `POPM` compatibility fields.

---

## Listening Statistics & History Timeline

- **Telemetry**: Plays and skips are automatically recorded upon playback transitions. A play is registered when listening exceeds 50% of track duration or 30 seconds.
- **Timeline View**: The History tab in Left Sidebar presents recently played tracks grouped into chronological sections:
  - **Today**
  - **Yesterday**
  - **This Week**
  - **Older**
- Double-clicking any history track immediately starts playback. The entire listening history can be cleared on demand.

---

## Real-Time Monitored Folders & Background Watcher

- `QFileSystemWatcher` continuously monitors all library directories configured in **Preferences > Library**.
- File system changes (additions, removals, renames) trigger an incremental background rescan debounced by 1500ms to avoid thrashing during file transfers.

---

## Physical Directory Explorer

For users with directory-organized music libraries, the Left Sidebar includes an expandable tree view under **Sources > Explorer**:
- Navigates real folders on disk.
- Expanding directories shows audio files and subfolders.
- Right-clicking or double-clicking a directory adds or plays all tracks within that folder hierarchy.

---

## Multi-Track Tag Editor

Accessed via **Edit > Edit Metadata...** or `Ctrl+I`:
- Edit metadata for single or multiple selected tracks simultaneously.
- Supported fields: Title, Artist, Album, Album Artist, Genre, Year, Track #, Disc #, Comment.
- Only modified ("dirty") fields are applied to preserve unique tags on untouched fields.
- **Cover Art Management**:
  - Extract embedded artwork to an image file.
  - Replace artwork from disk image file (JPEG, PNG, WebP) with atomic file updates.
  - Remove embedded artwork.

---

## Mini-Player & Floating Deck

Toggle with `Ctrl+Shift+M`:
- Compact, distraction-free floating deck.
- **Always-on-Top Pin**: Toggle pin icon to keep the mini-player floating above all other windows.
- **Marquee Ticker**: Smooth, text-looping ticker for lengthy track titles and artist names.
- Quick transport controls, seekbar, volume slider, and favorite heart toggle.

---

## Library Health Inspector & Missing File Relocation

Accessed via **Tools > Library Health & Missing Files...** or **Preferences > Library**:
- **Diagnostics Overview**: Displays library statistics including total tracks, missing files count, untagged tracks, duplicates, and database size.
- **Missing Files Table**: Lists all tracks whose files cannot be found on disk (e.g. disconnected external drives or moved music folders).
- **Batch Folder Relocation**: Allows selecting the old directory path and new directory path to update all affected track file paths in SQLite in a single transaction without losing play counts, ratings, or playlist memberships.
- **Clean Purge**: Safely remove broken track records from the library.
