---
name: build
description: Build the project or target
---

* Follow the instructions below without looking at the files.
* **Configure project only if `/build/windows-sdk` directory is missing**: `cmake --preset windows-sdk`
* **Build project**: `cmake --build build/windows-sdk`
* **Build target `framework`**: `cmake --build build/windows-sdk --target framework`
* **CMake targets**: `core`, `app-qt`, `framework`, `io`, `unittests`, `occ`
* **Run unit tests**: `ctest --test-dir bld/windows-sdk -C Release`
