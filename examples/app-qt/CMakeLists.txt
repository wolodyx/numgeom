set(SOURCE_FILES
  gpumemory.cpp             gpumemory.h
  main.cpp
  mainwindow.cpp            mainwindow.h
  vulkanwidget.cpp          vulkanwidget.h
  vulkanwindow.cpp          vulkanwindow.h
  vulkanwindowrenderer.cpp  vulkanwindowrenderer.h
)

set(RESOURCE_FILES
  resources/app.qrc
)

add_executable(app-qt
  ${SOURCE_FILES}
  ${RESOURCE_FILES}
)

target_link_libraries(app-qt
  Qt::Core
  Qt::Widgets
  Qt::Gui
  numgeom::render
  numgeom::numgeom
)

set(SHADER_FILES
  color.vert
  color.frag
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
    target_sources(app-qt PRIVATE ${file}.spv.h)
  endforeach()

  target_include_directories(app-qt PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
endif()
