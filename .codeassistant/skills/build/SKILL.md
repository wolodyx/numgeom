---
name: build
description: Build the project or target
---

* Follow the instructions below without looking at the files.
* **Configure project only if `/build/windows-vcpkg` directory is missing**: `cmake --preset windows-vcpkg`
* **Build project**: `cmake --build build/windows-vcpkg`
* **Build target `framework`**: `cmake --build build/windows-vcpkg --target framework`
* **CMake targets**: `core`, `app-qt`, `framework`, `io`, `unittests`, `occ`
* **Run unit tests**: `ctest --test-dir bld/windows-vcpkg -C Release`
