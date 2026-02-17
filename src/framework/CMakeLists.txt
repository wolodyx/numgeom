set(PUBLIC_HEADERS
  include/numgeom/application.h
  include/numgeom/gpumanager.h
  include/numgeom/gpumemory.h
  include/numgeom/userinputcontroller.h
)

set(SOURCE_FILES
  ${PUBLIC_HEADERS}
  application.cpp
  camera.cpp                camera.h
  gpumanager.cpp
  gpumemory.cpp
  meshchunk.cpp             meshchunk.h
  userinputcontroller.cpp
  vkutilities.cpp           vkutilities.h
)

set(SHADER_FILES
  app.vert
  app.frag
)

add_library(framework ${SOURCE_FILES})
add_library(numgeom::framework ALIAS framework)

target_link_libraries(framework
  PUBLIC
    Vulkan::Vulkan
    xcb::xcb
    Boost::log
    numgeom::core
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
    target_sources(framework PRIVATE ${file}.spv.h)
  endforeach()

  target_include_directories(framework PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
endif()

