set(SOURCE_FILES
  main.cc
  loadtotrimesh.cc          loadtotrimesh.h
  mainwindow.cc             mainwindow.h
  scenewindow.cc            scenewindow.h
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

# Добавляем перед запуском app-qt путь к библиотекам Qt.
include(QtUtils)
get_qt_binary_dir(NUMGEOM_QT_BIN_DIR)
set_target_properties(app-qt PROPERTIES
  VS_DEBUGGER_ENVIRONMENT "PATH=${NUMGEOM_QT_BIN_DIR};$ENV{PATH}"
)

# List of env variables to run application from vscode.
if(UNIX)
  file(GENERATE
    OUTPUT "${CMAKE_BINARY_DIR}/app-qt.env"
    CONTENT
[===[
LD_LIBRARY_PATH=${env:VCPKG_ROOT}/installed/x64-linux/debug/lib
QT_QPA_PLATFORM=xcb
]===]
  )
endif()

install(TARGETS app-qt)

install(FILES
  ${PROJECT_SOURCE_DIR}/tests/data/one-side-rounded-cube.step
  ${PROJECT_SOURCE_DIR}/tests/data/polydata-cube.vtk
  ${PROJECT_SOURCE_DIR}/tests/data/rounded-cube.step
  DESTINATION doc
)

deploy_qt_app(TARGET app-qt)
