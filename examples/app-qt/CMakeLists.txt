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

# List of env variables to run application from vscode.
if(UNIX)
  file(GENERATE
    OUTPUT "${CMAKE_BINARY_DIR}/app-qt.env"
    CONTENT
      [===[
VCPKG_ROOT=${env:HOME}/vcpkg
VK_LAYER_PATH=${env:VCPKG_ROOT}/packages/vulkan-validationlayers_x64-linux/share/vulkan/explicit_layer.d
LD_LIBRARY_PATH=${env:VCPKG_ROOT}/installed/x64-linux/debug/lib
QT_QPA_PLATFORM=xcb
      ]===]
  )
endif()

include(Qt5Utils)
copy_qt_plugin(Qt5::QJpegPlugin "imageformats")

install(TARGETS app-qt)

# Копируем плагины Qt.
install(DIRECTORY
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<$<PLATFORM_ID:Windows>:$<CONFIG>/>plugins
  DESTINATION bin
)

install(FILES
  ${PROJECT_SOURCE_DIR}/tests/data/one-side-rounded-cube.step
  ${PROJECT_SOURCE_DIR}/tests/data/polydata-cube.vtk
  ${PROJECT_SOURCE_DIR}/tests/data/rounded-cube.step
  DESTINATION doc
)

install(CODE [[
  set(DEPENDENCY_PATHS $ENV{VCPKG_ROOT})
  file(TO_CMAKE_PATH "${DEPENDENCY_PATHS}" DEPENDENCY_PATHS)

  file(GET_RUNTIME_DEPENDENCIES
    EXECUTABLES $<TARGET_FILE:app-qt>
    RESOLVED_DEPENDENCIES_VAR _r_deps
    UNRESOLVED_DEPENDENCIES_VAR _u_deps
    POST_EXCLUDE_REGEXES
      "^C:[/\\][Ww][Ii][Nn][Dd][Oo][Ww][Ss][/\\][Ss][Yy][Ss][Tt][Ee][Mm]32[/\\]*"
      "^/lib/"
    DIRECTORIES "${DEPENDENCY_PATHS}"
  )

  foreach(_file ${_r_deps})
    file(INSTALL
      DESTINATION "${CMAKE_INSTALL_PREFIX}/bin"
      TYPE SHARED_LIBRARY
      FOLLOW_SYMLINK_CHAIN
      FILES "${_file}"
    )
  endforeach()
]])
