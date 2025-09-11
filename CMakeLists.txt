cmake_minimum_required(VERSION 3.22 FATAL_ERROR)

project(NumGeom
  LANGUAGES CXX C
  VERSION 0.0.1
  DESCRIPTION "Base components for numerical geometry processing"
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

option(ENABLE_QT "Enable xcb code" ON)
if(UNIX)
  option(ENABLE_WAYLAND "Enable wayland code" ON)
  option(ENABLE_XCB "Enable xcb code" ON)
endif()

find_package(OpenCASCADE CONFIG REQUIRED)
find_package(GTest)
find_package(nlohmann_json REQUIRED)
find_package(Vulkan REQUIRED)
find_package(glslang COMPONENTS glslang-standalone REQUIRED)
find_package(Boost COMPONENTS log REQUIRED)
find_package(Python3 COMPONENTS Interpreter)
if(UNIX)
  find_package(FLEX 2.6.4)
  find_package(BISON 3.8.2)
  if(ENABLE_XCB)
    find_package(XCB REQUIRED)
  endif()
  if(ENABLE_WAYLAND)
    find_package(Wayland REQUIRED)
    find_package(SDL2 REQUIRED)
  endif()
endif()

if(ENABLE_QT)
  find_package(Qt5 COMPONENTS Core Gui Widgets OpenGL REQUIRED)
endif()

include(GenerateExportHeader)

add_subdirectory(extern)
add_subdirectory(src)
add_subdirectory(examples)

include(CTest)
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
