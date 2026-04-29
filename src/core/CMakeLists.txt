set(PUBLIC_HEADERS
  include/numgeom/alignedboundbox.h
  include/numgeom/circularlist.h
  include/numgeom/iterator.h
  include/numgeom/iteratorimpl.h
  include/numgeom/iteratorimpl.hpp
  include/numgeom/outcome.h
  include/numgeom/shapes.h
  include/numgeom/staticjaggedarray.h
  include/numgeom/trimesh.h
  include/numgeom/trimeshconnectivity.h
)

set(SOURCE_FILES
  ${PUBLIC_HEADERS}
  outcome.cc
  shapes.cc
  staticjaggedarray.cc
  trimesh.cc
  trimeshconnectivity.cc
)

add_library(core ${SOURCE_FILES})
add_library(numgeom::core ALIAS core)

#set_target_properties(core PROPERTIES PUBLIC_HEADER "${PUBLIC_HEADERS}")

generate_export_header(core
  EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/include/numgeom/core_export.h
)

target_include_directories(core
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}/include
)

target_link_libraries(core
  PUBLIC
    glm::glm
)

if(WIN32)
  target_compile_definitions(core PRIVATE _USE_MATH_DEFINES)
endif()
target_compile_definitions(core
  PUBLIC
    GLM_FORCE_DEPTH_ZERO_TO_ONE
    GLM_ENABLE_EXPERIMENTAL
)

install(TARGETS core)
