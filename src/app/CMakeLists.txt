set(SOURCE_FILES
  main.cpp
  mainwindow.cpp      mainwindow.h
  openglwidget.cpp    openglwidget.h
  logo.h              logo.cpp
)

add_executable(app ${SOURCE_FILES})

target_link_libraries(app
  numgeom::render
  glm::glm
  Qt5::Core
  Qt5::Gui
  Qt5::Widgets
  Qt5::OpenGL
)
