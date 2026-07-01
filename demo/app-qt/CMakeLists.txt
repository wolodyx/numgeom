cmake_minimum_required(VERSION 3.22 FATAL_ERROR)

project(DemoQt
  LANGUAGES CXX C
  VERSION 1.0.0
  DESCRIPTION "NumGeom demo Qt application"
)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets CONFIG)
qt_standard_project_setup()
set(CMAKE_AUTORCC ON)

set(SOURCE_FILES
  main.cc
  downloaddata.cc           downloaddata.h
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

qt6_add_executable(app-qt
  ${SOURCE_FILES}
  ${DIALOG_FILES}
  ${RESOURCE_FILES}
)

configure_file(version.h.in ${CMAKE_CURRENT_BINARY_DIR}/version.h)
target_include_directories(app-qt PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(app-qt
  PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
    numgeom::framework
    numgeom::io
    $<$<TARGET_EXISTS:numgeom::occ>:numgeom::occ>
    httplib
    ZLIB::ZLIB
)

include(QtUtils)
generate_qt_conf()

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

# --- Deployment via qt_deploy_runtime_dependencies ---
# Генерируем скрипт развертывания, который копирует runtime-зависимости Qt
# (библиотеки, плагины) в каталог установки приложения.
qt_generate_deploy_app_script(TARGET app-qt
  OUTPUT_SCRIPT deploy-app-script
  NO_TRANSLATIONS
)

install(TARGETS app-qt
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(SCRIPT ${deploy-app-script})

# Копирование runtime-зависимостей сторонних библиотек в каталог установки.
if (WIN32)
  # В Windows qt_deploy_runtime_dependencies копирует только зависимости Qt.
  # Для остальных библиотек (OpenCASCADE, Boost, ZLIB и др.) используем
  # file(GET_RUNTIME_DEPENDENCIES) с указанием каталогов поиска из vcpkg.
  # Путь к исполняемому файлу берется из каталога установки, куда он был
  # скопирован на предыдущем шаге.
  set(_vcpkg_bin_dir "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/bin")
  install(CODE [[
    if(POLICY CMP0207)
      cmake_policy(SET CMP0207 NEW)
    endif()
    file(GET_RUNTIME_DEPENDENCIES
      EXECUTABLES $<TARGET_FILE:app-qt>
      RESOLVED_DEPENDENCIES_VAR _r_deps
      UNRESOLVED_DEPENDENCIES_VAR _u_deps
      POST_EXCLUDE_REGEXES
        "^C:[/\\][Ww][Ii][Nn][Dd][Oo][Ww][Ss][/\\][Ss][Yy][Ss][Tt][Ee][Mm]32[/\\]*"
        "^/lib/x86_64-linux-gnu/"
      DIRECTORIES "${_vcpkg_bin_dir}"
    )
    foreach(_file ${_r_deps})
      file(INSTALL
        DESTINATION "${CMAKE_INSTALL_PREFIX}/bin"
        TYPE SHARED_LIBRARY
        FOLLOW_SYMLINK_CHAIN
        FILES "${_file}"
      )
    endforeach()
    if(_u_deps)
      message(STATUS "Unresolved dependencies: ${_u_deps}")
    endif()
  ]])
endif()
