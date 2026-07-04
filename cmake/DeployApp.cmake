# copy_runtime_dependencies
#
# Copies the shared libraries necessary for the operation of executable files to
# the specified directory.
#
# Arguments:
#   TARGETS <target1> [<target2> ...]
#     The names of the targets created in the project.
#   EXECUTABLES <exe1> [<exe2> ...]
#     Absolute paths to already compiled executable files.
#   DIRECTORIES <dir1> [<dir2> ...]
#     The list of directories where the dependency search will be performed.
function(copy_runtime_dependencies)
  set(no_value_options)
  set(one_value_options DESTINATION)
  set(multi_value_options TARGETS EXECUTABLES DIRECTORIES)
  cmake_parse_arguments(
    PARSE_ARGV 0 arg
    "${no_value_options}"
    "${one_value_options}"
    "${multi_value_options}"
  )

  # Collect executable paths from TARGETS and EXECUTABLES.
  # Use generator expressions so they resolve at build time.
  set(_executables)
  foreach(target IN LISTS arg_TARGETS)
    list(APPEND _executables "$<TARGET_FILE:${target}>")
  endforeach()
  list(APPEND _executables ${arg_EXECUTABLES})

  if(NOT _executables)
    message(WARNING "No executables specified; nothing to copy")
    return()
  endif()

  # Prepare directories list
  set(_dirs)
  foreach(dir IN LISTS arg_DIRECTORIES)
    file(TO_CMAKE_PATH "${dir}" _cmake_dir)
    list(APPEND _dirs "${_cmake_dir}")
  endforeach()

  # Use file(GENERATE) to produce an install script so that generator
  # expressions ($<TARGET_FILE:...>) are expanded at build/generate time.
  # CMake variables (_executables, _dirs) are substituted here at
  # configure time via string(CONFIGURE).
  string(JOIN " " _executables ${_executables})
  string(JOIN " " _dirs ${_dirs})
  set(_script_gen_content [[
    if(POLICY CMP0207)
      cmake_policy(SET CMP0207 NEW)
    endif()
    file(GET_RUNTIME_DEPENDENCIES
      EXECUTABLES @_executables@
      RESOLVED_DEPENDENCIES_VAR _r_deps
      UNRESOLVED_DEPENDENCIES_VAR _u_deps
      POST_EXCLUDE_REGEXES
        "^C:[/\\][Ww][Ii][Nn][Dd][Oo][Ww][Ss][/\\][Ss][Yy][Ss][Tt][Ee][Mm]32[/\\]*"
        "^/lib/x86_64-linux-gnu/"
      DIRECTORIES "@_dirs@"
    )
    set(dest_dir lib)
    if(WIN32)
      set(dest_dir bin)
    endif()
    foreach(_file ${_r_deps})
      message(STATUS "INSTALL: ${_file}")
      file(INSTALL "${_file}"
        DESTINATION "${CMAKE_INSTALL_PREFIX}/${dest_dir}"
        TYPE SHARED_LIBRARY
        FOLLOW_SYMLINK_CHAIN
      )
    endforeach()
    if(_u_deps)
      message(WARNING "Unresolved dependencies: ${_u_deps}")
    endif()
  ]])
  string(CONFIGURE "${_script_gen_content}" _script_content @ONLY)
  set(_script "${CMAKE_CURRENT_BINARY_DIR}/copy_runtime_deps_$<CONFIG>.cmake")
  file(GENERATE OUTPUT "${_script}" CONTENT "${_script_content}")
  install(SCRIPT "${_script}")
endfunction()

function(_get_qt_core_location location config)
  if(NOT TARGET Qt::Core AND NOT TARGET Qt${QT_VERSION_MAJOR}::Core)
    message(FATAL_ERROR "Target Qt::Core doesn't exist!")
  endif()

  string(TOUPPER ${config} CONFIG)
  set(targets Qt::Core Qt${QT_VERSION_MAJOR}::Core)
  set(properties
    LOCATION_${config}
    LOCATION_${CONFIG}
    IMPORTED_LOCATION_${config}
    IMPORTED_LOCATION_${CONFIG}
  )

  foreach(tgt IN LISTS targets)
    foreach(prop IN LISTS properties)
      get_target_property(loc ${tgt} ${prop})
      if(loc)
        set(${location} ${loc} PARENT_SCOPE)
        return()
      endif()
    endforeach()
  endforeach()
  message(FATAL_ERROR "Target Qt::Core doesn't have any LOCATION property")
endfunction()

# Добываем путь к каталогу с плагинами qt.
# Не используем для этого qmake, потому что утилита не доступна в vcpkg для
# триплета x64-linux-dynamic, так как этот триплет не "native".
function(get_qt_plugins_dir plugins_dir config)
  _get_qt_core_location(core_location ${config})
  get_filename_component(lib_dir ${core_location} DIRECTORY)
  get_filename_component(root_dir ${lib_dir} DIRECTORY)
  set(dirs
    ${lib_dir}
    ${root_dir}
  )
  set(sub_dirs
    "plugins"
    "Qt${QT_VERSION_MAJOR}/plugins"
    "qt${QT_VERSION_MAJOR}/plugins"
  )
  set(result)
  foreach(dir IN LISTS dirs)
    foreach(sub_dir IN LISTS sub_dirs)
      if(EXISTS "${dir}/${sub_dir}")
        set(${plugins_dir} "${dir}/${sub_dir}" PARENT_SCOPE)
        return()
      endif()
    endforeach()
  endforeach()
  message(FATAL_ERROR "Qt plugins directory not found")
endfunction()

# Формируем пускач -- исполняемый файл для настройки окружения и запуска
# приложения qt.
function(create_qt_launcher tgt)
  if(NOT UNIX)
    return()
  endif()
  set(APP_EXECUTABLE "${tgt}")
  get_filename_component(FUNC_DIR ${CMAKE_CURRENT_FUNCTION_LIST_FILE} DIRECTORY)
  set(LAUNCH_FILE "${CMAKE_CURRENT_BINARY_DIR}/${APP_EXECUTABLE}.sh")
  configure_file(
    "${FUNC_DIR}/launcher-qtapp.sh.in"
    "${LAUNCH_FILE}"
    @ONLY
  )
  file(CHMOD "${LAUNCH_FILE}"
    FILE_PERMISSIONS
      OWNER_EXECUTE OWNER_WRITE OWNER_READ
      GROUP_EXECUTE GROUP_READ
      WORLD_EXECUTE WORLD_READ
  )
  install(PROGRAMS "${LAUNCH_FILE}" DESTINATION bin)
endfunction()

# Получаем список каталогов, в которых могут найтись внешние зависимости.
function(_get_dep_dirs dep_dirs)
  set(result)
  if (VCPKG_TOOLCHAIN)
    list(APPEND result "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/bin")
  endif()
  set(${dep_dirs} ${result} PARENT_SCOPE)
endfunction()

function(_deploy_qt_app)
  set(no_value_options)
  set(single_value_options TARGET)
  set(multi_value_options)
  cmake_parse_arguments(
    PARSE_ARGV 0 arg
    "${no_value_options}"
    "${single_value_options}"
    "${multi_value_options}"
  )
  if(NOT arg_TARGET)
    message(FATAL_ERROR "TARGET must be specified")
  endif()

  _get_dep_dirs(dep_dirs)
  copy_runtime_dependencies(
    TARGETS ${arg_TARGET}
    DIRECTORIES ${dep_dirs}
  )
endfunction()

function(create_qt_conf)
  if(WIN32)
    get_qt_plugins_dir(NUMGEOM_QT_PLUGIN_PATH Release)
    configure_file(
      "${PROJECT_SOURCE_DIR}/cmake/qt.conf.in"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release/qt.conf"
      @ONLY
    )
    get_qt_plugins_dir(NUMGEOM_QT_PLUGIN_PATH Debug)
    configure_file(
      "${PROJECT_SOURCE_DIR}/cmake/qt.conf.in"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/qt.conf"
      @ONLY
    )
  elseif(UNIX)
    get_qt_plugins_dir(NUMGEOM_QT_PLUGIN_PATH ${CMAKE_BUILD_TYPE})
    configure_file(
      "${PROJECT_SOURCE_DIR}/cmake/qt.conf.in"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/qt.conf"
      @ONLY
    )
  endif()
endfunction()

# Генерируем скрипт развертывания, который копирует runtime-зависимости
# (библиотеки, плагины) в каталог установки приложения.
function(deploy_app)
  set(no_value_options)
  set(single_value_options TARGET)
  set(multi_value_options)
  cmake_parse_arguments(
    PARSE_ARGV 0 arg
    "${no_value_options}"
    "${single_value_options}"
    "${multi_value_options}"
  )

  if(NOT arg_TARGET)
    message(FATAL_ERROR "TARGET must be specified.")
  endif()

  # Признак дополнительного поиска внешних зависимостей командой
  # `file(GET_RUNTIME_DEPENDENCIES)`, вызываемого через функцию
  # `copy_runtime_dependencies`.
  set(DEPLOY_USING_GRD 1)
  if(QT_FOUND)
    # Начиная с версии 6.5 Qt поддерживает функцию
    # `qt6_generate_deploy_app_script` в законченном виде.
    if(QT_VERSION VERSION_GREATER_EQUAL "6.5")
      qt6_generate_deploy_app_script(TARGET ${arg_TARGET}
        OUTPUT_SCRIPT deploy-app-script
        NO_TRANSLATIONS
      )
      install(SCRIPT ${deploy-app-script})
      # Под Windows `qt6_generate_deploy_app_script` копирует только
      # зависимости Qt, поэтому для остальных библиотек используем
      # `copy_runtime_dependencies`.
      if(NOT WIN32)
        set(DEPLOY_USING_GRD 0)
      endif()
    else()
      # Используем собственный деплой Qt на основе `file(GET_RUNTIME_DEPENDENCIES)`.
      # В эту ветку попадаем когда используем системный Qt в Ubuntu 24.04.
      set(DEPLOY_USING_GRD 0)
      _deploy_qt_app(${ARGV})
    endif()
    create_qt_launcher(${arg_TARGET})
  endif()

  # Копируем еще не скопированные runtime-зависимости.
  if(DEPLOY_USING_GRD EQUAL 1)
    _get_dep_dirs(dep_dirs)
    copy_runtime_dependencies(
      TARGETS ${arg_TARGET}
      DIRECTORIES ${dep_dirs}
    )
  endif()
endfunction()
