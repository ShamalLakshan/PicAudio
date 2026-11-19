# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.1.0-beta.1] - 2026-11-19

### Added
- Native bit-perfect audio engine replacing GStreamer pipeline with in-process FFmpeg decoders and TagLib readers.
- Real-time parametric equalizer with ISO 10/15/31 band modes and AutoEq headphone profile database.
- Multi-channel matrix processor (stereo, 5.1 surround, 7.1 cinema upmix/downmix) with live RMS VU meters.
- Extensible C ABI plugin system supporting runtime plugin installation via drag-and-drop and single-file `.paplugin` bundles.
- Real-time audio visualizers (fluid waveform and multi-band FFT/VU spectrum bars) with dedicated floating visualizer window.
- Custom theme engine with dynamic hot-reloading and comfortable display density presets.
- CUE sheet parsing and multi-track album splitting for single-file FLAC/WAV/APE/MP3 album images.
- Cross-platform packaging pipeline: Linux AppImage with `.zsync` delta updates, Windows portable ZIP, NSIS setup installer, and developer Plugin SDK.
