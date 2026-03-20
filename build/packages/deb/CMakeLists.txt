# Package size in kilobytes.
# Result of command `du -k ./package`.
set(DEB_PACKAGE_INSTALLED_SIZE 16)

configure_file(control.in package/DEBIAN/control)

add_custom_target(deb
  COMMAND
    ${CMAKE_CURRENT_SOURCE_DIR}/create-deb-package.sh ${CMAKE_INSTALL_PREFIX}
  COMMENT
    "Create deb package"
)

add_dependencies(deb install)
