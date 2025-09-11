set(PUBLIC_HEADERS
  include/numgeom/common.h
  include/numgeom/sceneitem.h
  include/numgeom/scenemanager.h
  include/numgeom/vkrender.h
)

set(SHADER_FILES
  app.vert
  app.frag
)

set(SOURCE_FILES
  ${PUBLIC_HEADERS}
  cube.cpp
  sceneitem.cpp
  scenemanager.cpp
  vkrender.cpp
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
    Vulkan::Vulkan
    glm::glm
    Boost::log
)

if(ENABLE_WAYLAND)
  target_link_libraries(render
    PUBLIC
      SDL2::SDL2
      Wayland::Client
      xdg-shell-client
  )
  target_compile_definitions(render PUBLIC ENABLE_WAYLAND)
  target_sources(render PRIVATE connect-wayland.cpp)
endif()

if(ENABLE_XCB)
  target_link_libraries(render
    PUBLIC
      xcb::xcb
  )
  target_compile_definitions(render PUBLIC ENABLE_XCB)
  target_sources(render PRIVATE connect-xcb.cpp)
endif()

# Добавляем шейдерные файлы в проект.
if(DEFINED SHADER_FILES)
  foreach(file IN LISTS SHADER_FILES)
    add_custom_command(
      OUTPUT ${file}.spv.h
      COMMAND
        "$<TARGET_FILE:glslang::glslang-standalone>"
          -V ${CMAKE_CURRENT_SOURCE_DIR}/${file}
          -o ${file}.spv
      COMMAND
        "${Python3_EXECUTABLE}"
      ARGS "${CMAKE_CURRENT_SOURCE_DIR}/binary_to_hex_array.py"
           "${CMAKE_CURRENT_BINARY_DIR}/${file}.spv"
           "${CMAKE_CURRENT_BINARY_DIR}/${file}.spv.h"
      DEPENDS ${file}
    )
    target_sources(render PRIVATE ${file}.spv.h)
  endforeach()

  target_include_directories(render PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
endif()

