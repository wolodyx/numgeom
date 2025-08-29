set(SOURCE_FILES
  main.cpp
  testcircularlist.cpp
  testclassificate.cpp
  testoctree.cpp
  testquadtree.cpp
  testremovefaces.cpp
  testtrimesh.cpp
  utilities.cpp             utilities.h
)

add_executable(unittests ${SOURCE_FILES})

set(TEST_DATA_DIR ${PROJECT_SOURCE_DIR}/tests/data)

configure_file(configure.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/configure.h
)

target_include_directories(unittests
  PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(unittests
  GTest::gtest_main
  numgeom
)

include(GoogleTest)
gtest_discover_tests(unittests)
