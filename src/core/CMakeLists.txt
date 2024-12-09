set(PUBLIC_HEADERS
  include/numgeom/iterator.h
  include/numgeom/outcome.h
)

set(SOURCE_FILES
  ${PUBLIC_HEADERS}
  outcome.cpp
)

add_library(core ${SOURCE_FILES})
add_library(numgeom::core ALIAS core)

set_target_properties(core PROPERTIES PUBLIC_HEADER "${PUBLIC_HEADERS}")

generate_export_header(core
  EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/include/numgeom/numgeomcore_export.h
)

target_include_directories(core
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}/include
)
