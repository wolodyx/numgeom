set(PUBLIC_HEADERS
  include/numgeom/sceneitem.h
  include/numgeom/scenemanager.h
  include/numgeom/shaderprogram.h
)

set(SOURCE_FILES
  ${PUBLIC_HEADERS}
  sceneitem.cpp
  scenemanager.cpp
  shaderprogram.cpp
)

add_library(render ${SOURCE_FILES})
add_library(numgeom::render ALIAS render)

set_target_properties(render PROPERTIES PUBLIC_HEADER "${PUBLIC_HEADERS}")

generate_export_header(render
  EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/include/numgeom/render_export.h
)

target_include_directories(render
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_BINARY_DIR}/include"
  PRIVATE
    "${GLFW_SOURCE_DIR}/deps"
)

target_link_libraries(render
  PUBLIC
    numgeom::core
    TKMath
  PRIVATE
    GLEW::glew
    OpenGL::GL
    glm::glm
)
