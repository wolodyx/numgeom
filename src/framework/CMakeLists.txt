set(PUBLIC_HEADERS
  include/numgeom/application.h
  include/numgeom/userinputcontroller.h
)

set(SOURCE_FILES
  ${PUBLIC_HEADERS}
  application.cpp
  camera.h                  camera.cpp
  userinputcontroller.cpp
)

add_library(framework ${SOURCE_FILES})
add_library(numgeom::framework ALIAS framework)

target_link_libraries(framework
  PUBLIC
    glm::glm
    numgeom::numgeom
)

set_target_properties(framework PROPERTIES PUBLIC_HEADER "${PUBLIC_HEADERS}")

generate_export_header(framework
  EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/include/numgeom/framework_export.h
)

target_include_directories(framework
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_BINARY_DIR}/include"
)
