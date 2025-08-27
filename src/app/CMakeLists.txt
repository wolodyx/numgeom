set(SOURCE_FILES
  main.cpp
  mainwindow.cpp      mainwindow.h
  openglwidget.cpp    openglwidget.h
  logo.h              logo.cpp
  cube.cpp
  vkrender.cpp        vkrender.h
)

set(SHADER_FILES
  app.vert
  app.frag
)

set(DISPLAY_MODE "xcb_mode") # wayland_mode, xcb_mode

add_executable(app ${SOURCE_FILES})

target_link_libraries(app
  Vulkan::Vulkan
  Wayland::Client
  SDL2::SDL2
  Qt5::Core
  Qt5::Widgets
  Qt5::Gui
)

if(DISPLAY_MODE STREQUAL "wayland_mode")
  target_compile_definitions(app PUBLIC ENABLE_WAYLAND)
  target_link_libraries(app
    Wayland::Client
    SDL2::SDL2
  )
elseif(DISPLAY_MODE STREQUAL "xcb_mode")
  target_compile_definitions(app PUBLIC ENABLE_XCB)
else()
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
        bash -c "${CMAKE_CURRENT_SOURCE_DIR}/binary2text.sh ${CMAKE_CURRENT_BINARY_DIR}/${file}.spv"
      DEPENDS ${file}
    )
    target_sources(app PRIVATE ${file}.spv.h)
  endforeach()

  target_include_directories(app PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
endif()
