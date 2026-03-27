# Directory size in kilobytes.
function(get_directory_size_kb DIR VAR)
    execute_process(
        COMMAND du -sk "${DIR}"
        OUTPUT_VARIABLE DU_OUT
        ERROR_QUIET
    )
    string(REGEX MATCH "[0-9]+" SIZE "${DU_OUT}")
    set(${VAR} ${SIZE} PARENT_SCOPE)
endfunction()

set(DEB_PACKAGE_ARCHITECTURE ${CMAKE_SYSTEM_PROCESSOR})
set(DEB_PACKAGE_VERSION ${PROJECT_VERSION})
set(DEB_PACKAGE_INSTALLED_SIZE)
get_directory_size_kb(
  DEB_PACKAGE_INSTALLED_SIZE
  ${cmake_install_prefix}
  DEB_PACKAGE_INSTALLED_SIZE
)

if(${DEB_PACKAGE_ARCHITECTURE} STREQUAL "x86_64")
  set(DEB_PACKAGE_ARCHITECTURE "amd64")
endif()

configure_file(control.in package/DEBIAN/control)

add_custom_target(deb
  COMMAND
    ${CMAKE_COMMAND} --install ${PROJECT_BINARY_DIR}
  COMMAND
    ${CMAKE_CURRENT_SOURCE_DIR}/create-deb-package.sh
      --arch ${DEB_PACKAGE_ARCHITECTURE}
      --version ${DEB_PACKAGE_VERSION}
      --path ${CMAKE_INSTALL_PREFIX}
      # --verbose
  COMMENT "Create deb package"
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
