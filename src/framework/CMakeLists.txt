set(PUBLIC_HEADERS
  include/numgeom/application.h
  include/numgeom/debug_framework.h
  include/numgeom/drawable.h
  include/numgeom/drawable_cone.h
  include/numgeom/drawable_cylinder.h
  include/numgeom/drawable_sphere.h
  include/numgeom/gpumanager.h
  include/numgeom/scene.h
  include/numgeom/sceneobject.h
  include/numgeom/sceneobject_mesh.h
  include/numgeom/scenewidget.h
  include/numgeom/scenewidget_axisindicator.h
  include/numgeom/screentext.h
  include/numgeom/userinputcontroller.h
)

set(SOURCE_FILES
  ${PUBLIC_HEADERS}
  application.cc
  applicationinner.cc       applicationinner.h
  applicationstate.cc       applicationstate.h
  camera.cc                 camera.h
  debug_framework.cc
  drawable.cc
  drawable_cone.cc
  drawable_cylinder.cc
  drawable_sphere.cc
  gpumanager.cc
  logo.cc                   logo.h
  scene.cc
  sceneiterators.cc         sceneiterators.h
  sceneobject.cc
  sceneobject_mesh.cc
  scenewidget.cc
  scenewidget_axisindicator.cc
  screentext.cc
  screentextds.h
  screentextinner.cc        screentextinner.h
  screentextstate.cc        screentextstate.h
  userinputcontroller.cc
  vkutilities.cc            vkutilities.h
)

set(SHADER_FILES
  app.vert
  app.frag
  logo.vert
  logo.frag
  text.vert
  text.frag
)

set(FONT_FILES
  "UbuntuSans[wdth,wght].ttf"
)

add_library(framework ${SOURCE_FILES} ${SHADER_FILES})
add_library(numgeom::framework ALIAS framework)

target_link_libraries(framework
  PUBLIC
    volk
    Boost::log
    numgeom::core
  PRIVATE
    VulkanMemoryAllocator
    stb_image
    stb_truetype
)

# Use VK_NO_PROTOTYPES to disable the use of prototypes in the Vulkan header.
target_compile_definitions(framework PUBLIC VK_NO_PROTOTYPES)
if(WIN32)
  target_compile_definitions(framework PUBLIC VK_USE_PLATFORM_WIN32_KHR)
elseif(LINUX AND ENABLE_XCB)
  target_compile_definitions(framework PUBLIC VK_USE_PLATFORM_XCB_KHR)
endif()

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
        --chunk-size=4
        --output "${CMAKE_CURRENT_BINARY_DIR}/${file}.spv.h"
        "${CMAKE_CURRENT_BINARY_DIR}/${file}.spv"
      DEPENDS ${file}
    )
    target_sources(framework PRIVATE ${file}.spv.h)
  endforeach()

  target_include_directories(framework PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
endif()

# Добавляем файлы шрифтов в проект.
foreach(file IN LISTS FONT_FILES)
  add_custom_command(
    OUTPUT ${file}.h
    COMMAND
      ${Python3_EXECUTABLE}
    ARGS
      "${CMAKE_CURRENT_SOURCE_DIR}/binary_to_hex_array.py"
      --chunk-size=1
      --output "${CMAKE_CURRENT_BINARY_DIR}/${file}.h"
      "${CMAKE_CURRENT_SOURCE_DIR}/resources/${file}"
    DEPENDS ${file}
  )
  target_sources(framework PRIVATE ${file}.h)
endforeach()


if(ENABLE_XCB)
  target_link_libraries(framework PRIVATE xcb::xcb)
endif()

install(TARGETS framework)
