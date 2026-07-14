cmake_minimum_required(VERSION 3.22 FATAL_ERROR)

project(NumGeom
  LANGUAGES CXX C
  VERSION 0.0.1
  DESCRIPTION "Base components for numerical geometry processing"
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_SKIP_INSTALL_RPATH TRUE)

option(ENABLE_QT "Enable xcb code" ON)
if(UNIX)
  option(ENABLE_WAYLAND "Enable wayland code" OFF )
  option(ENABLE_XCB "Enable xcb code" ON)
endif()

find_package(Vulkan REQUIRED)
find_package(Boost COMPONENTS log REQUIRED CONFIG)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
find_package(FLEX)
find_package(BISON)
find_package(OpenCASCADE CONFIG)
message(STATUS "OpenCASCADE: Version ${OpenCASCADE_VERSION}")
if(UNIX)
  if(ENABLE_XCB)
    find_package(XCB REQUIRED)
  endif()
  if(ENABLE_WAYLAND)
    find_package(Wayland REQUIRED)
    find_package(SDL2 REQUIRED)
  endif()
endif()
# We are trying to find the system <package>. If it doesn't exist,
# then we build it from the source codes in `extern/<package>`.
find_package(glfw3 CONFIG)
find_package(glm CONFIG)
find_package(GTest CONFIG)
find_package(nlohmann_json CONFIG)
find_package(volk CONFIG)
find_package(VulkanMemoryAllocator CONFIG)
find_package(ZLIB)

include(GitVersionDetect)
set(PROJECT_VERSION_MAJOR ${GITVERSIONDETECT_VERSION_MAJOR})
set(PROJECT_VERSION_MINOR ${GITVERSIONDETECT_VERSION_MINOR})
set(PROJECT_VERSION_PATCH ${GITVERSIONDETECT_VERSION_PATCH})
set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")

include(GenerateExportHeader)

add_subdirectory(extern)
add_subdirectory(src)
add_subdirectory(demo)

include(CTest)
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()

include(CPackConfig.cmake)
include(CPack)
