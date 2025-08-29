set(SOURCE_FILES
  main.cpp
  #mainwindow.cpp            mainwindow.h
  #vulkanwidget.cpp          vulkanwidget.h
  vulkanwindow.cpp          vulkanwindow.h
  vulkanwindowrenderer.cpp  vulkanwindowrenderer.h
)

add_executable(app-qt ${SOURCE_FILES})

target_link_libraries(app-qt
  Qt::Core
  Qt::Widgets
  Qt::Gui
  numgeom::render
)
