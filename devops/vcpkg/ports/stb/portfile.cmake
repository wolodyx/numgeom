vcpkg_from_github(
  OUT_SOURCE_PATH SOURCE_PATH
  REPO nothings/stb
  REF 2c980bb59875b0d32144a71867fbdebb2f77cd20
  SHA512 93db78589d836b5a8924ed0e980c6fd8d1dbc7e2d0489bc5a0fa4dedd73ed94332e5c814adc2976e89fab3f4cd96b5c46beae7b5dd55bc8374a91390d774b2b0
  HEAD_REF master
)

# stb ships no upstream CMake build. It is header-only, so we install the
# public headers directly and provide CMake interface targets matching the
# INTERFACE targets.

file(INSTALL "${SOURCE_PATH}/"
  DESTINATION "${CURRENT_PACKAGES_DIR}/include/stb"
  FILES_MATCHING
  PATTERN "*.h"
)

# Generate a CMake package config exposing the interface targets the project
# expects: stb, stb_image, stb_image_write, stb_truetype.
# The file is fully self-contained, resolving the install prefix relative to its
# own location.
file(WRITE "${CURRENT_PACKAGES_DIR}/share/${PORT}/stb-config.cmake" [[
get_filename_component(_stb_prefix "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)

if(NOT TARGET stb)
  add_library(stb INTERFACE IMPORTED)
  set_target_properties(stb PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${_stb_prefix}/include/stb"
  )
endif()

foreach(comp stb_image stb_image_write stb_truetype)
  if(NOT TARGET ${comp})
    add_library(${comp} INTERFACE IMPORTED)
    set_target_properties(${comp} PROPERTIES
      INTERFACE_LINK_LIBRARIES stb
    )
  endif()
endforeach()

set(stb_FOUND TRUE)
]])

file(REMOVE_RECURSE
  "${CURRENT_PACKAGES_DIR}/debug/include"
  "${CURRENT_PACKAGES_DIR}/debug/share"
)
