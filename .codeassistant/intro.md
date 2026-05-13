# Introduction to the NumGeom project

NumGeom is a platform for the rapid development of 3D interactive applications using C++, Qt, Vulkan, Vcpkg, CMake, OpenCascade.

* Code style: `Google C++ style Guide`
* Configure project: `cmake --preset windows-vcpkg`
* Build project: `cmake --build bld/windows-vcpkg`
* Test project: `ctest --test-dir bld/windows-vcpkg -C Release`
* Unit tests use `Google Testing Framework`
* CMake targets: `core`, `app-qt`, `framework`, `io`, `unittests`, `occ`.
* The name of the target matches the name of the directory where it is hosted.
* `/src` directory contains the source code of libraries.
* `/demo` directory contains the source code of demo applications.
* Include files, forming the interface of the module, are located in the `include/numgeom` directory in the library directory.
* Use `socraticode` mcp-server to analyze the code.
* Use `debugmcp` mcp-server to debug the code.
