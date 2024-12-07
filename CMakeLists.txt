cmake_minimum_required(VERSION 3.22 FATAL_ERROR)

project(NumGeom
  LANGUAGES CXX C
  VERSION 0.0.1
  DESCRIPTION "Base components for numerical geometry processing"
)

set(CMAKE_CXX_STANDARD 17)

find_package(FLEX 2.6.4)
find_package(BISON 3.8.2)
find_package(OpenCASCADE CONFIG REQUIRED)
find_package(GTest)
find_package(nlohmann_json REQUIRED)

include(GenerateExportHeader)

add_subdirectory(extern)
add_subdirectory(src)

include(CTest)
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
