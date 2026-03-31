
function(copy_qt_plugin PLUGIN_TARGET DEST_SUBDIR)
  if(NOT TARGET ${PLUGIN_TARGET})
    message(WARNING "Plugin target ${PLUGIN_TARGET} not found, skipping")
    return()
  endif()

  get_target_property(plugin_location ${PLUGIN_TARGET} LOCATION)

  if(NOT plugin_location OR NOT EXISTS "${plugin_location}")
    message(WARNING "Plugin file for ${PLUGIN_TARGET} not found")
    return()
  endif()

  if(WIN32)
    file(COPY
      ${plugin_location}
      DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release/plugins/${DEST_SUBDIR}
    )
    file(COPY
      ${plugin_location}
      DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/plugins/${DEST_SUBDIR}
    )
  else()
    file(COPY
      ${plugin_location}
      DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/plugins/${DEST_SUBDIR}
    )
  endif()
endfunction()

function(get_qt_binary_dir qt_binary_dir)
  get_target_property(qt_core_location Qt${NUMGEOM_QT_MAJOR_VERSION}::Core LOCATION)
  get_filename_component(dir ${qt_core_location} DIRECTORY)
  set(${qt_binary_dir} ${dir} PARENT_SCOPE)
endfunction()

function(_deploy_qt6_app tgt)
  qt_generate_deploy_app_script(
    TARGET ${tgt}
    OUTPUT_SCRIPT deploy_script
    NO_TRANSLATIONS
  )
  install(SCRIPT ${deploy_script})
endfunction()

function(_deploy_qt5_app tgt)
  if(UNIX)
    copy_qt_plugin("Qt${NUMGEOM_QT_MAJOR_VERSION}::QXcbIntegrationPlugin" "platforms")
  elseif(WIN32)
    copy_qt_plugin("Qt${NUMGEOM_QT_MAJOR_VERSION}::QWindowsIntegrationPlugin" "platforms")
  endif()
endfunction()

function(deploy_qt_app)
  set(options)
  set(oneValueArgs TARGET)
  set(multiValueArgs)
  cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
  if(NOT ARG_TARGET)
    message(FATAL_ERROR "TARGET must be specified")
  endif()
  if(${NUMGEOM_QT_MAJOR_VERSION} EQUAL 6)
    _deploy_qt6_app(${ARG_TARGET})
  elseif(${NUMGEOM_QT_MAJOR_VERSION} EQUAL 5)
    _deploy_qt5_app(${ARG_TARGET})
  else()
    message(FATAL_ERROR "Unexpected Qt version (${NUMGEOM_QT_MAJOR_VERSION})")
  endif()

  # Формируем пускач -- исполняемый файл для настройки окружения и запуска
  # приложения qt.
  if(UNIX)
    set(APP_EXECUTABLE ${ARG_TARGET})
    get_filename_component(FUNC_DIR ${CMAKE_CURRENT_FUNCTION_LIST_FILE} DIRECTORY)
    set(LAUNCHER_FILE "${CMAKE_CURRENT_BINARY_DIR}/launcher-${APP_EXECUTABLE}.sh")
    configure_file(
      "${FUNC_DIR}/launcher-qtapp.sh.in"
      "${LAUNCHER_FILE}"
      @ONLY
    )
    file(CHMOD
      "${LAUNCHER_FILE}"
      FILE_PERMISSIONS
        OWNER_EXECUTE OWNER_WRITE OWNER_READ
        GROUP_EXECUTE GROUP_READ
        WORLD_EXECUTE WORLD_READ
    )
    install(FILES
      "${LAUNCHER_FILE}"
      DESTINATION bin
    )
  endif()
endfunction()
