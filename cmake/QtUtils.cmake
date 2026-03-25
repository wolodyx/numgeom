
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

  file(COPY
    ${plugin_location}
    DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/plugins/${DEST_SUBDIR}
  )
endfunction()

function(get_qt_binary_dir qt_binary_dir)
  get_target_property(qt_core_location Qt${NUMGEOM_QT_MAJOR_VERSION}::Core LOCATION)
  get_filename_component(dir ${qt_core_location} DIRECTORY)
  set(${qt_binary_dir} ${dir} PARENT_SCOPE)
endfunction()
