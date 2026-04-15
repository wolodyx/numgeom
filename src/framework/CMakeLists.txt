set(PUBLIC_HEADERS
  include/numgeom/application.h
  include/numgeom/axisindicator.h
  include/numgeom/drawable.h
  include/numgeom/gpumanager.h
  include/numgeom/scene.h
  include/numgeom/sceneobject.h
  include/numgeom/sceneobject_mesh.h
  include/numgeom/scenewidget.h
  include/numgeom/userinputcontroller.h
)

set(SOURCE_FILES
  ${PUBLIC_HEADERS}
  application.cc
  axisindicator.cc
  camera.cc                 camera.h
  drawable.cc
  gpumanager.cc
  scene.cc
  sceneiterators.cc         sceneiterators.h
  sceneobject.cc
  sceneobject_mesh.cc
  scenewidget.cc
  userinputcontroller.cc
  vkutilities.cc            vkutilities.h
)

set(SHADER_FILES
  app.vert
  app.frag
)

add_library(framework ${SOURCE_FILES} ${SHADER_FILES})
add_library(numgeom::framework ALIAS framework)

target_link_libraries(framework
  PUBLIC
    Vulkan::Vulkan
    Boost::log
    numgeom::core
  PRIVATE
    VulkanMemoryAllocator
)

#set_target_properties(framework PROPERTIES PUBLIC_HEADER "${PUBLIC_HEADERS}")

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
        $<TARGET_FILE:Vulkan::glslangValidator>
      ARGS
        -V "${CMAKE_CURRENT_SOURCE_DIR}/${file}"
        -o "${CMAKE_CURRENT_BINARY_DIR}/${file}.spv"
      COMMAND
        ${Python3_EXECUTABLE}
      ARGS
        "${CMAKE_CURRENT_SOURCE_DIR}/binary_to_hex_array.py"
        "${CMAKE_CURRENT_BINARY_DIR}/${file}.spv"
        "${CMAKE_CURRENT_BINARY_DIR}/${file}.spv.h"
      DEPENDS ${file}
    )
    target_sources(framework PRIVATE ${file}.spv.h)
  endforeach()

  target_include_directories(framework PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
endif()

if(ENABLE_XCB)
  target_link_libraries(framework PRIVATE xcb::xcb)
endif()

install(TARGETS framework)
