set(PUBLIC_HEADERS
  appcontext.h
  application.h
  command.h
  debug_framework.h
  drawable.h
  drawable_cone.h
  drawable_cylinder.h
  drawable_sphere.h
  drawable_torus.h
  drawableimplementations.h
  fgobject.h
  fgtext.h
  framework_enums.h
  gizmo.h
  gizmo_arrow.h
  gizmo_filepath.h
  gizmo_placein3d.h
  gizmo_realnumber.h
  guimediator.h
  lrupool.h
  phantom.h
  pixelbuffer.h
  scene.h
  sceneobject.h
  sceneobject_mesh.h
  scenewidget.h
  scenewidget_axisindicator.h
  trackedobject.h
  userinputcontroller.h
  vkscenerenderer.h
  workbench.h
)
list(TRANSFORM PUBLIC_HEADERS PREPEND "include/numgeom/")

set(SOURCE_FILES
  ${PUBLIC_HEADERS}
  appcontext.cc
  application.cc
  camera.cc                 camera.h
  command.cc
  debug_framework.cc
  drawable.cc
  drawable_cone.cc
  drawable_cylinder.cc
  drawable_primitives.cc    drawable_primitives.h
  drawable_sphere.cc
  drawable_torus.cc
  drawableimplementations.cc
  fgimage.cc                fgimage.h
  fgobject.cc
  fgtext.cc
  gizmo.cc
  gizmo_arrow.cc
  gizmo_filepath.cc
  gizmo_placein3d.cc
  gizmo_realnumber.cc
  gizmoobserver.cc          gizmoobserver.h
  guimediator.cc
  intersection.cc           intersection.h
  lrudescriptorsetpool.cc   lrudescriptorsetpool.h
  lrupool.cc
  phantom.cc
  pixelbuffer.cc
  scene.cc
  sceneobject.cc
  sceneobject_mesh.cc
  scenewidget.cc
  scenewidget_axisindicator.cc
  trackedobject.cc
  trackedobjectdict.cc      trackedobjectdict.h
  trackedobjectlist.cc      trackedobjectlist.h
  userinputcontroller.cc
  vkscenerenderer.cc
  vkutilities.cc            vkutilities.h
  volk_impl.cc
  workbench.cc
  workplaneattributes.cc    workplaneattributes.h
)

set(SHADER_FILES
  app.vert
  app.frag
  compositing.vert
  compositing.frag
  selection.vert
  selection.frag
  text.vert
  text.frag
  workplane.vert
  workplane.frag
  line.vert
  line.frag
  point.vert
  point.frag
)

set(FONT_FILES
  "UbuntuSans[wdth,wght].ttf"
)

add_library(framework SHARED ${SOURCE_FILES} ${SHADER_FILES})
add_library(numgeom::framework ALIAS framework)

target_link_libraries(framework
  PUBLIC
    volk::volk_headers
    Boost::log
    numgeom::core
  PRIVATE
    GPUOpen::VulkanMemoryAllocator
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
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/resources/${file}"
  )
  target_sources(framework PRIVATE ${file}.h)
endforeach()


if(ENABLE_XCB)
  target_link_libraries(framework PRIVATE xcb::xcb)
endif()

install(TARGETS framework)
