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

find_package(Vulkan REQUIRED COMPONENTS glslc)
find_package(Boost COMPONENTS log REQUIRED CONFIG)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
find_package(FLEX)
find_package(BISON)
find_package(OpenCASCADE CONFIG)
if(UNIX)
  if(ENABLE_XCB)
    find_package(XCB REQUIRED)
  endif()
  if(ENABLE_WAYLAND)
    find_package(Wayland REQUIRED)
    find_package(SDL2 REQUIRED)
  endif()
endif()

if(ENABLE_QT)
  set(QtComponents Core Gui Widgets)
  find_package(Qt6 COMPONENTS ${QtComponents} CONFIG)
  set(NUMGEOM_QT_MAJOR_VERSION 6)
  if(NOT Qt6_FOUND)
    find_package(Qt5 COMPONENTS ${QtComponents} CONFIG)
    set(NUMGEOM_QT_MAJOR_VERSION 5)
    if(NOT Qt5_FOUND)
      message(FATAL_ERROR "Qt package not found!")
    endif()
  endif()
  set(CMAKE_AUTOMOC ON)
  set(CMAKE_AUTORCC ON)
  set(CMAKE_AUTOUIC ON)
endif()

include(GitVersionDetect)
set(PROJECT_VERSION_MAJOR ${GITVERSIONDETECT_VERSION_MAJOR})
set(PROJECT_VERSION_MINOR ${GITVERSIONDETECT_VERSION_MINOR})
set(PROJECT_VERSION_PATCH ${GITVERSIONDETECT_VERSION_PATCH})
set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")

include(GenerateExportHeader)

add_subdirectory(extern)
add_subdirectory(src)
add_subdirectory(examples)
add_subdirectory(build)

include(CTest)
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()

include(CPackConfig.cmake)
include(CPack)
