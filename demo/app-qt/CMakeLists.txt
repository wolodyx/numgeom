cmake_minimum_required(VERSION 3.22 FATAL_ERROR)

project(DemoQt
  LANGUAGES CXX C
  VERSION 1.0.0
  DESCRIPTION "NumGeom demo Qt application"
)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core CONFIG)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets CONFIG)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(SOURCE_FILES
  main.cc
  commandwidget.cc          commandwidget.h
  downloaddata.cc           downloaddata.h
  gizmowidget.cc            gizmowidget.h
  gizmowidget_filepath.cc   gizmowidget_filepath.h
  gizmowidget_placein3d.cc  gizmowidget_placein3d.h
  gizmowidget_realnumber.cc gizmowidget_realnumber.h
  loadtotrimesh.cc          loadtotrimesh.h
  mainwindow.cc             mainwindow.h
  scenewindow.cc            scenewindow.h
  scenemdisubwindow.cc      scenemdisubwindow.h
)

set(RESOURCE_FILES
  resources/app.qrc
)

set(DIALOG_FILES
  addforegroundtext.cc       addforegroundtext.h
  addforegroundimage.cc      addforegroundimage.h
)
list(TRANSFORM DIALOG_FILES PREPEND "dialogs/")

add_executable(app-qt
  ${SOURCE_FILES}
  ${DIALOG_FILES}
  ${RESOURCE_FILES}
)

configure_file(version.h.in ${CMAKE_CURRENT_BINARY_DIR}/version.h)
target_include_directories(app-qt PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(app-qt
  PRIVATE
    Qt::Core
    Qt::Widgets
    Qt::Gui
    numgeom::framework
    numgeom::io
    httplib::httplib
    ZLIB::ZLIB
)

if(TARGET numgeom::occ)
  target_link_libraries(app-qt PRIVATE numgeom::occ)
  target_compile_definitions(app-qt PRIVATE
    NUMGEOM_HAS_OCC=1
    NUMGEOM_OCC_MODULE="$<TARGET_FILE_NAME:occ>"
  )
endif()

include(DeployApp)
create_qt_conf()

if(UNIX)
  # List of env variables to run application from vscode.
  file(GENERATE
    OUTPUT "${CMAKE_BINARY_DIR}/app-qt.env"
    CONTENT "QT_QPA_PLATFORM=xcb"
  )
  # Add custom target to build appimage.
  add_custom_target(appimage-app-qt
    COMMAND
      ${CMAKE_CURRENT_SOURCE_DIR}/appimage/make-appimage.sh
        ${CMAKE_CURRENT_SOURCE_DIR}
    WORKING_DIRECTORY
      ${CMAKE_CURRENT_BINARY_DIR}
  )
  add_dependencies(appimage-app-qt app-qt)
endif()

install(TARGETS app-qt
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

deploy_app(TARGET app-qt)
