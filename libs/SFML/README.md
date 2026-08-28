# SFML Setup

This project uses **SFML 3.1.0 (64-bit)** and the Visual Studio `v145`
toolset. The game itself remains a regular Visual Studio project; it does not
use CMake or vcpkg.

## Why SFML is built locally

SFML's Windows binaries must match the compiler used by the game. Build SFML
3.1.0 from source with Visual Studio 2026 instead of mixing the project with
binaries produced by another Visual C++ toolset.

## Build SFML 3.1.0

1. Download the official SFML 3.1.0 source archive from
   https://www.sfml-dev.org/download/sfml/3.1.0/ and extract it.
2. Open the source folder with the CMake integration included in Visual Studio
   2026, or use Visual Studio's bundled `cmake.exe` to generate a
   `Visual Studio 18 2026` x64 build.
3. Build both `Debug` and `Release` as shared libraries.
4. Copy the installed `include`, `lib` and `bin` directories into this
   repository:

```text
libs/
└── SFML/
    ├── include/
    ├── lib/
    └── bin/          <- the eight runtime DLLs (see below)
```

`bin/` must contain all eight DLLs, both configurations:

```text
sfml-system-3.dll     sfml-system-d-3.dll
sfml-window-3.dll     sfml-window-d-3.dll
sfml-graphics-3.dll   sfml-graphics-d-3.dll
sfml-audio-3.dll      sfml-audio-d-3.dll
```

The project already references `include` and `lib`, and a post-build step
copies the configuration's DLLs from `bin/` next to the executable, so the
game runs straight from `bin/Debug` or `bin/Release` with no manual copying.
`libs/SFML/` is git-ignored, so each clone populates it once.

The Network module is not used by the game.
