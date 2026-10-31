<p align="center">
  <img src="logo/PicAudio-black.png" alt="PicAudio Logo" width="160" height="160" />
</p>

<h1 align="center">PicAudio</h1>

<p align="center">
  <strong>A modern, high-fidelity desktop audio player and music library manager inspired by magpies.</strong>
</p>

<p align="center">
  <em>Built on modern C++20 and Qt 6 / QML. Designed for audiophiles and power users.</em>
</p>

---

> Still under development...

## Plugins & Public SDK

PicAudio features a compiler-agnostic C-ABI plugin architecture allowing third-party developers to extend the player without recompiling the host application:
- **Visualizers**: Real-time FFT spectrum analyzers, peak meters, and fluid waveforms.
- **Lyrics & Scrobbling**: Synchronized `.lrc` parsers, Last.fm and ListenBrainz scrobbling.
- **DSP & Effects**: Pre/post hardware buffer audio processing and filter chains.
- **Metadata**: Custom tag enrichers and online metadata scrapers.

Developers can explore:
- [**Plugin Developer Guide & SDK API Reference**](docs/PLUGIN_API.md)
- [**Plugin Manifest Schema**](docs/plugin-schema.json)
- [**Sample Plugin Boilerplate**](sdk/templates/)

## Acknowledgements & Attributions

PicAudio stands on the shoulders of outstanding open-source libraries, audio standards, and pioneer desktop audio players. See [ATTRIBUTIONS.md](ATTRIBUTIONS.md) for the complete list of credits and references.

---

## License

PicAudio is licensed under the [GNU Affero General Public License v3.0 (AGPL-3.0)](LICENSE).
