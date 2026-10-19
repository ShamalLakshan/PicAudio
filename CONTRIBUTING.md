# Contributing to PicAudio

This guide covers setting up the development environment, building the project from source, running the test suite, and contributing changes.

## Prerequisites

Building PicAudio requires:
- C++20 compatible compiler (GCC 11+, Clang 13+)
- CMake 3.20+
- Qt 6 (Core, Gui, Qml, Quick, QuickControls2)
- TagLib
- SQLite3

### Installing Dependencies

**Arch Linux / Manjaro:**
```bash
sudo pacman -S base-devel cmake qt6-base qt6-declarative taglib sqlite
```

**Ubuntu / Debian (22.04+):**
```bash
sudo apt update
sudo apt install build-essential cmake qt6-base-dev qt6-declarative-dev libtag1-dev libsqlite3-dev
```

**Fedora:**
```bash
sudo dnf install gcc-c++ cmake qt6-qtbase-devel qt6-qtdeclarative-devel taglib-devel sqlite-devel
```

## Building from Source

Clone the repository:
```bash
git clone https://github.com/ShamalLakshan/PicAudio.git
cd PicAudio
```

Configure and build:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
```

Run the application:
```bash
./build/app/PicAudio
```

## Running Tests

Unit tests use GoogleTest and can be run through `ctest` or executed directly:

```bash
# Run tests with ctest
ctest --test-dir build --output-on-failure

# Or run test binary directly
./build/tests/picaudio_tests
```

## Architecture Overview

PicAudio uses a clean architecture structure:

```
PicAudio/
├── core/            # Domain logic, entities, ports, and services (pure C++20, no Qt)
├── adapters/        # Concrete implementations:
│   ├── audio/       # miniaudio playback backend
│   ├── metadata/    # TagLib metadata extraction
│   ├── storage/     # SQLite database adapter
│   └── ui_qt/       # Qt/QML bridge, models, and UI components
├── app/             # Application entry point wiring adapters to services
├── sdk/             # Public plugin interfaces
├── plugins/         # First-party reference plugins
├── tests/           # Unit tests and mocks
└── docs/            # Architecture notes and theme schema
```

## Coding & Style Guidelines

- **Standard**: C++20.
- **Tone**: Keep code comments simple, humble, and practical. Avoid marketing buzzwords, fluff, or phase milestone markers.
- **Doxygen**: Use standard Doxygen docblocks (`/** @brief ... */`) for class, method, and function declarations in headers.
- **Independence**: Keep `core/` completely free of Qt or UI-specific dependencies.

## Submitting Changes

1. Create a feature branch from `dev`:
   ```bash
   git checkout -b feature/your-feature-name
   ```
2. Implement your changes and add tests where appropriate.
3. Verify that all tests pass:
   ```bash
   ctest --test-dir build --output-on-failure
   ```
4. Commit your changes with concise, descriptive commit messages.
5. Push to your fork and submit a Pull Request against `dev`.

## Reporting Issues

If you encounter any problems, please open an issue with:
- Operating system and desktop environment
- Qt version
- Steps to reproduce and any relevant terminal/log output
