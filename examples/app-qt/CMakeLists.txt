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
  $<$<TARGET_EXISTS:numgeom::occ>:numgeom::occ>
)

# Добавляем перед запуском app-qt путь к библиотекам Qt.
include(QtUtils)
get_qt_binary_dir(NUMGEOM_QT_BIN_DIR)
set_target_properties(app-qt PROPERTIES
  VS_DEBUGGER_ENVIRONMENT "PATH=${NUMGEOM_QT_BIN_DIR};$ENV{PATH}\nQT_PLUGIN_PATH=d:/projects/numgeom/bld/windows-vcpkg/vcpkg_installed/x64-windows/$<$<CONFIG:Debug>:debug/>Qt6/plugins"
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

if(UNIX)
  add_custom_target(appimage-app-qt
    COMMAND
      ${CMAKE_CURRENT_SOURCE_DIR}/appimage/make-appimage.sh
        ${CMAKE_CURRENT_SOURCE_DIR}
    WORKING_DIRECTORY
      ${CMAKE_CURRENT_BINARY_DIR}
  )
  add_dependencies(appimage-app-qt app-qt)
endif()

install(TARGETS app-qt)

install(FILES
  ${PROJECT_SOURCE_DIR}/tests/data/one-side-rounded-cube.step
  ${PROJECT_SOURCE_DIR}/tests/data/polydata-cube.vtk
  ${PROJECT_SOURCE_DIR}/tests/data/rounded-cube.step
  DESTINATION doc
)

set(used_qt_plugins
  Qt${NUMGEOM_QT_MAJOR_VERSION}::QJpegPlugin
)
if(LINUX)
  list(APPEND used_qt_plugins Qt${NUMGEOM_QT_MAJOR_VERSION}::QXcbIntegrationPlugin)
elseif(WINDOWS)
  list(APPEND used_qt_plugins Qt${NUMGEOM_QT_MAJOR_VERSION}::QWindowsIntegrationPlugin)
endif()

install(
  FILES appimage/app-qt.sh
  DESTINATION bin
  PERMISSIONS
    OWNER_EXECUTE OWNER_WRITE OWNER_READ
    GROUP_EXECUTE GROUP_READ
    WORLD_EXECUTE WORLD_READ
)
install(
  FILES appimage/qt.conf
  DESTINATION bin
)
set(executable_files $<TARGET_FILE:app-qt>)
foreach(plugin ${used_qt_plugins})
  get_target_property(loc ${plugin} LOCATION)
  list(APPEND executable_files ${loc})
endforeach()

install(
  CODE "set(executable_files ${executable_files})"
  CODE [[
  if(DEFINED VCPKG_INSTALLED_DIR AND NOT ${VCPKG_INSTALLED_DIR} STREQUAL "")
    set(DEPENDENCY_PATHS "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib")
  endif()
  file(GET_RUNTIME_DEPENDENCIES
    EXECUTABLES ${executable_files}
    RESOLVED_DEPENDENCIES_VAR _r_deps
    UNRESOLVED_DEPENDENCIES_VAR _u_deps
    POST_EXCLUDE_REGEXES
      "^C:[/\\]Windows[/\\]system32[/\\]*"
      "^/lib/x86_64-linux-gnu/"
    DIRECTORIES "${DEPENDENCY_PATHS}"
  )
  foreach(_file ${_r_deps})
    file(INSTALL
      DESTINATION "${CMAKE_INSTALL_PREFIX}/lib"
      TYPE SHARED_LIBRARY
      FOLLOW_SYMLINK_CHAIN
      FILES "${_file}"
    )
  endforeach()
  # Creating the missing `libvulkan.so ` -- symbolic link to `libvulkan.so.1'.
  if(NOT EXISTS ${CMAKE_INSTALL_PREFIX}/lib/libvulkan.so)
    if(EXISTS ${CMAKE_INSTALL_PREFIX}/lib/libvulkan.so.1)
      file(CREATE_LINK
        libvulkan.so.1
        ${CMAKE_INSTALL_PREFIX}/lib/libvulkan.so
        SYMBOLIC
      )
    endif()
  endif()
]])

foreach(plugin IN LISTS used_qt_plugins)
  get_target_property(type ${plugin} QT_PLUGIN_TYPE)
  install(FILES $<TARGET_FILE:${plugin}>
    DESTINATION plugins/${type}
  )
endforeach()
