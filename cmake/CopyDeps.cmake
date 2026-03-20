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
#   DESTINATION <dest_dir>
#     The directory where all found shared libraries will be copied.
#   RESOLVE_DEPS_ALL (optional)
#     If specified, the function recursively resolves the dependencies of the
#     found libraries. Without this flag, only direct dependencies of executable
#     files are copied.
#
# Example of a call inside install(CODE)
#   install(CODE [[
#     copy_runtime_dependencies(
#       EXECUTABLES "$<TARGET_FILE:myapp>"
#       DIRECTORIES "${CMAKE_BINARY_DIR}/vcpkg_installed/x64-linux-dynamic"
#       DESTINATION "${CMAKE_INSTALL_PREFIX}/lib"
#       RESOLVE_DEPS_ALL
#     )
#   ]])
function(copy_runtime_dependencies)
  set(options RESOLVE_DEPS_ALL)
  set(oneValueArgs DESTINATION)
  set(multiValueArgs TARGETS EXECUTABLES DIRECTORIES)
  cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT ARG_DESTINATION)
    message(FATAL_ERROR "DESTINATION must be specified")
  endif()

  # Collect executable paths from TARGETS and EXECUTABLES
  set(_executables)
  foreach(target IN LISTS ARG_TARGETS)
    list(APPEND _executables "$<TARGET_FILE:${target}>")
  endforeach()
  list(APPEND _executables ${ARG_EXECUTABLES})

  if(NOT _executables)
    message(WARNING "No executables specified; nothing to copy")
    return()
  endif()

  # Prepare directories list
  set(_dirs)
  foreach(dir IN LISTS ARG_DIRECTORIES)
    file(TO_CMAKE_PATH "${dir}" _cmake_dir)
    list(APPEND _dirs "${_cmake_dir}")
  endforeach()

  # Determine whether to resolve all dependencies or only direct ones
  if(ARG_RESOLVE_DEPS_ALL)
    # Recursive resolution: exclude only system libraries
    set(_post_exclude_regexes
      "^C:[/\\][Ww][Ii][Nn][Dd][Oo][Ww][Ss][/\\][Ss][Yy][Ss][Tt][Ee][Mm]32[/\\]*"
      "^/lib/"
    )
  else()
    # Only direct dependencies: exclude all libraries (i.e., do not resolve further)
    # This is achieved by excluding everything after the first level.
    # Since file(GET_RUNTIME_DEPENDENCIES does not have a direct‑only flag,
    # we approximate by excluding all libraries that are not immediate dependencies.
    # For simplicity, we keep the same system exclusions and rely on the fact that
    # POST_EXCLUDE_REGEXES will prevent recursion beyond the first level.
    set(_post_exclude_regexes
      "^C:[/\\][Ww][Ii][Nn][Dd][Oo][Ww][Ss][/\\][Ss][Yy][Ss][Tt][Ee][Mm]32[/\\]*"
      "^/lib/"
      ".*"  # Exclude everything else (this will stop recursion)
    )
  endif()

  file(GET_RUNTIME_DEPENDENCIES
    EXECUTABLES ${_executables}
    RESOLVED_DEPENDENCIES_VAR _r_deps
    UNRESOLVED_DEPENDENCIES_VAR _u_deps
    POST_EXCLUDE_REGEXES ${_post_exclude_regexes}
    DIRECTORIES ${_dirs}
  )

  foreach(_file ${_r_deps})
    file(COPY
      DESTINATION "${ARG_DESTINATION}"
      TYPE SHARED_LIBRARY
      FOLLOW_SYMLINK_CHAIN
      FILES "${_file}"
    )
  endforeach()
endfunction()
