set(SOURCE_FILES
  main.cpp
  mainwindow.cpp      mainwindow.h
  openglwidget.cpp    openglwidget.h
  logo.h              logo.cpp
  triangle.vert.spv.h
  triangle.frag.spv.h
)

add_executable(app ${SOURCE_FILES})

target_compile_definitions(app PUBLIC VK_USE_PLATFORM_WAYLAND_KHR)

target_link_libraries(app
  numgeom::render
  Vulkan::Vulkan
  glm::glm
  Qt5::Core
  Qt5::Gui
  Qt5::GuiPrivate
  Qt5::Widgets
  Qt5::OpenGL
)

set(SHADER_FILES triangle.vert triangle.frag)

foreach(file IN LISTS SHADER_FILES)
  add_custom_command(
    OUTPUT ${file}.spv.h
    COMMAND
      "$<TARGET_FILE:glslang::glslang-standalone>"
        -V ${CMAKE_CURRENT_SOURCE_DIR}/${file}
        -o ${file}.spv
    COMMAND
      bash -c "${CMAKE_CURRENT_SOURCE_DIR}/binary2text.sh ${CMAKE_CURRENT_BINARY_DIR}/${file}.spv"
    DEPENDS ${file}
  )
endforeach()

target_include_directories(app PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
