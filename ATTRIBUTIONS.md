# Acknowledgements & Attributions

PicAudio is developed as a modern, high-fidelity desktop music player and library manager that stands on the shoulders of outstanding open-source projects, audio engineering specifications, and community initiatives:

---

## Audio Engine & Playback Pipeline

Special credit and gratitude go to **[fooyin](https://www.fooyin.org/)** by **Luke Taylor** (<luket@pm.me>).

PicAudio's audio playback backend, real-time pipeline, and thread architecture are derived from and inspired by the Fooyin audio engine. This includes:
- **Multi-Tier Thread Isolation**: Clean asynchronous separation between the UI Thread $\leftrightarrow$ Engine Worker Thread $\leftrightarrow$ Real-time Audio Pipeline Thread.
- **64-bit Floating Point Mixer**: Real-time multi-stream mixing with sample-accurate track fading and volume control.
- **Asynchronous Decoding Pipeline**: In-process FFmpeg and TagLib decoders with watermark hysteresis and background candidate track pre-buffering.
- **Sample-Accurate Playback Transitions**: Gapless boundary handoff, immediate cuts, and customizable equal-power crossfading.
- **Lock-Free Visualization Bus**: High-speed PFFFT SIMD spectrum analyzer and circular PCM waveform history for responsive visualizers.
- **Direct Hardware Audio Backends**: Native PipeWire client, direct ALSA (`hw:`) streaming, and Qt Audio fallback.

The underlying audio engine components are adapted and licensed under the **GNU General Public License version 3 (GPLv3)**.

---

## Core Libraries & Toolkits

- [Qt 6](https://www.qt.io/) — Cross-platform GUI and application framework (Qt Quick / QML)
- [TagLib](https://taglib.org/) — Audio metadata extraction and tag editing library
- [SQLite](https://www.sqlite.org/) — In-process SQL database engine
- [FFmpeg](https://ffmpeg.org/) — Cross-platform multimedia solution for audio decoding and stream handling
- [PFFFT](https://github.com/martonparlaghi/PFFFT) — Pretty Fast FFT library for high-speed SIMD real-time spectrum analysis
- [GoogleTest](https://github.com/google/googletest) — C++ test framework

---

## Audio Standards & Protocols

- [ReplayGain 2.0 Specification](https://wiki.hydrogenaud.io/index.php?title=ReplayGain_2.0_specification) — Loudness normalization standard
- [ITU-R BS.1770-4](https://www.itu.int/rec/R-REC-BS.1770/en) — Audio loudness and peak level measurement algorithm
- [M3U / Extended M3U8 Playlist Format](https://en.wikipedia.org/wiki/M3U) — Industry standard playlist format
- [Audioscrobbler Protocol](https://www.last.fm/api/scrobbling) — Real-time music scrobbling protocol
- [ListenBrainz API](https://listenbrainz.readthedocs.io/en/latest/users/api/) — Open-source music listening statistics

---

## Desktop Player Ergonomics & Design References

- [foobar2000](https://www.foobar2000.org/) — Advanced audio player for Windows
- [MusicBee](https://getmusicbee.com/) — The ultimate music manager and player
- [Strawberry Music Player](https://www.strawberrymusicplayer.org/) — High-fidelity music player and library organizer
- [Clementine](https://www.clementine-player.org/) — Multiplatform music player inspired by Amarok

---

## Typography & Visual Design

- [Inter Typeface](https://rsms.me/inter/) — Variable font family crafted by Rasmus Andersson
- [Tabler Icons](https://tabler-icons.io/) — Open-source SVG icons
- [Feather Icons](https://feathericons.com/) — Open-source minimalist icon set
