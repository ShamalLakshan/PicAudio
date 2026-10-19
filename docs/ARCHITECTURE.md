# PicAudio — Hexagonal Architecture & Technical Design Specification

## 1. Architectural Philosophy: Ports & Adapters (Hexagonal)

PicAudio is designed around **Alistair Cockburn's Hexagonal Architecture** (Ports & Adapters).
The goal is complete domain independence:
- The **Core Domain** (`core/`) has **zero knowledge** of Qt, SQLite, TagLib, ALSA, WASAPI, CoreAudio, or any platform SDK.
- The Core Domain defines **Pure Port Interfaces** (`core/include/core/ports/`).
- External frameworks and hardware drivers reside entirely within **Adapters** (`adapters/`).
- The **App composition root** (`app/`) is responsible for instantiating adapters and injecting them into core services.

```
       +--------------------------------------------------------------+
       |                       PicAudio App                           |
       |                        (app/main.cpp)                        |
       +--------------------------------------------------------------+
                |                                            |
                v                                            v
     [ Adapters: UI & Storage ]                   [ Adapters: Audio & Hardware ]
     - adapters/ui_qt/                            - adapters/audio/
       * QtQuick / QML GUI                          * BitPerfectAudioAdapter (Qt/ALSA)
       * ThemeLoader & Hot-Reloader                 * alsa/ (Direct hw: / PipeWire)
       * List & Detail ViewModels                   * wasapi/ (Exclusive Windows)
     - adapters/storage/                          - adapters/metadata/
       * SqliteDatabaseAdapter                      * TagLibMetadataAdapter
                |                                            |
                +--------------------+  +--------------------+
                                     |  |
                                     v  v
                       +-------------------------------+
                       |      Ports (Interfaces)       |
                       |  - IAudioEnginePort           |
                       |  - IMusicDatabasePort         |
                       |  - IMetadataExtractor         |
                       |  - IPlayerPlugin              |
                       |  - ILibraryPlugin             |
                       |  - IVisualizerPlugin          |
                       +-------------------------------+
                                     |
                                     v
                       +-------------------------------+
                       |       Core Pure Domain        |
                       |  - Entities: Track, Album     |
                       |  - Services: QueueService     |
                       |              PlayerService    |
                       |              LibraryService   |
                       +-------------------------------+
```

---

## 2. Directory Boundaries & Code Organization

```
PicAudio/
├── core/                                # Pure domain — C++20 standard library only
│   ├── include/core/
│   │   ├── entities/                    # Value objects & domain models (Track, Album, etc.)
│   │   ├── ports/                       # Hexagonal abstract port interfaces
│   │   └── services/                    # Business rules (PlayerService, QueueService, LibraryService)
│   └── src/services/                    # Core service implementations
├── adapters/                            # Hexagonal port implementations
│   ├── audio/                           # Audio output drivers
│   │   ├── alsa/                        # Linux ALSA direct hw / PipeWire lock-free backend
│   │   ├── wasapi/                      # Windows WASAPI Exclusive backend
│   │   └── coreaudio/                   # macOS CoreAudio integer backend
│   ├── metadata/                        # TagLib 2.x audio metadata & cover extractor
│   ├── storage/                         # SQLite3 database with indexed & FTS5 search
│   └── ui_qt/                           # Qt 6 Quick / QML MusicBee studio UI
│       ├── theming/                     # ThemeLoader, schema validator, QFileSystemWatcher
│       ├── models/                      # QAbstractListModel implementations
│       ├── providers/                   # QQuickImageProvider for cached cover art
│       └── qml/                         # QML desktop interface
│           ├── components/              # ContextMenu, VectorIcon, JumpBar, Rating
│           ├── dialogs/                 # PreferencesDialog, TagEditorDialog
│           └── views/                   # TrackTable, AlbumGrid, QueueView
├── sdk/                                 # Public plugin API (stable C ABI)
│   └── include/picaudio/plugin/
├── plugins/                             # First-party dogfooding plugins
├── app/                                 # Composition root (main entry point)
├── tests/                               # Unit & integration tests against mock ports
└── docs/                                # Technical documentation & JSON schemas
```

---

## 3. Port Contracts & Data Flow

### 3.1 `IAudioEnginePort`
- **Responsibility**: Hardware audio output abstraction.
- **Contract**: Accepts PCM/file path requests, provides synchronous playback control (`play`, `pause`, `stop`, `seek`), and notifies on track completion via `setEndOfTrackCallback`.
- **Implementations**:
  - `adapters::BitPerfectAudioAdapter` (Qt 6 Multimedia / PipeWire / ALSA).
  - Dedicated hardware backends (`alsa/`, `wasapi/`, `coreaudio/`).
  - `MockAudioEnginePort` for automated headless testing in `tests/`.

### 3.2 `IMusicDatabasePort`
- **Responsibility**: Persistence and retrieval of indexed audio tracks and album groupings.
- **Contract**: Thread-safe CRUD operations, search queries, and atomic transactions.
- **Implementations**:
  - `adapters::SqliteDatabaseAdapter`.
  - `MockMusicDatabasePort` for pure in-memory test fixtures.

### 3.3 `IMetadataExtractor`
- **Responsibility**: Reading audio tags, stream properties (sample rate, bit depth, bitrate, channels, codec), and embedded image extraction.
- **Contract**: Thread-safe extraction given a filesystem path.
- **Implementations**:
  - `adapters::TagLibMetadataAdapter`.

### 3.4 Capability-Scoped Plugin Ports
- `IPlayerPlugin`: Hook into real-time audio buffers (`onPreDsp`, `onPostDsp`) and playback lifecycle events.
- `ILibraryPlugin`: Scrobblers, lyrics scrapers, and external metadata enrichers.
- `IVisualizerPlugin`: High-speed lock-free PCM buffer consumer for real-time FFT spectrum visualizers.

---

## 4. Threading Model & Concurrency

1. **GUI / Main Thread (Qt Event Loop)**:
   - Manages QML rendering, user input, animations, and model notifications (`beginResetModel()`, `endResetModel()`).
   - Receives events from background threads via `QMetaObject::invokeMethod(..., Qt::QueuedConnection)`.

2. **Library Scanner Thread**:
   - Worker threads spawned by `LibraryService` or `QtBridge` execute recursive filesystem walks and TagLib parsing without blocking the UI.
   - Batch database insertion utilizes SQLite transactions (`BEGIN TRANSACTION` / `COMMIT`) for maximum throughput (>500 tracks/sec).

3. **Audio Output / Real-time Audio Callback**:
   - Runs in high-priority operating system audio threads.
   - Thread isolation ensures the real-time audio pipeline is **never blocked** by filesystem I/O, database queries, or GUI re-renders.

---

## 5. Theming System Architecture

- Themes are specified using **Design Tokens** structured according to `docs/theme-schema.json`.
- `ThemeLoader` loads themes from user disk (`~/.local/share/PicAudio/themes/` or `~/.local/share/PicAudio/PicAudio/themes/`) or embedded defaults.
- Fallback strategy: In case of missing fields, malformed JSON, or non-existent files, `ThemeLoader` **guarantees non-crashing fallback** to the built-in dark studio theme.
- **Hot-Reloading**: Powered by `QFileSystemWatcher`. Modifying the active JSON on disk triggers an instantaneous reload and notification across QML properties without restarting the player.
