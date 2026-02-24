set(SOURCE_FILES
  main.cpp
  mainwindow.cpp            mainwindow.h
  scenewindow.cpp           scenewindow.h
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
  numgeom::framework
  numgeom::io
)

install(TARGETS app-qt)
