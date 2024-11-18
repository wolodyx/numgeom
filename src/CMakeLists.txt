set(PUBLIC_HEADERS
  include/numgeom/circularlist.h
  include/numgeom/classificate.h
  include/numgeom/enums.h
  include/numgeom/ijk.h
  include/numgeom/iterator.h
  include/numgeom/octree.h
  include/numgeom/quadtree.h
  include/numgeom/removefaces.h
  include/numgeom/staticjaggedarray.h
  include/numgeom/tesselate.h
  include/numgeom/trimesh.h
  include/numgeom/trimeshconnectivity.h
  include/numgeom/utilities.h
)

set(SOURCE_FILES
  ${PUBLIC_HEADERS}
  classificate.cpp
  curvemetriclength.cpp     curvemetriclength.h
  ffextintersections.cpp    ffextintersections.h
  intersections.h           intersections.cpp
  iterator_ijk.cpp          iterator_ijk.h
  octree.cpp
  quadtree.cpp
  removefaces.cpp
  staticjaggedarray.cpp
  tesselate.cpp
  trimesh.cpp
  trimeshconnectivity.cpp
  utilities.cpp
  writevtk.cpp              writevtk.h
)

add_library(numgeom ${SOURCE_FILES})

set_target_properties(numgeom PROPERTIES PUBLIC_HEADER "${PUBLIC_HEADERS}")

generate_export_header(numgeom
  EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/include/numgeom/numgeom_export.h
)

target_link_libraries(numgeom
  PUBLIC
    TKMath
    TKG3d
    TKernel
  PRIVATE
    TKBO
    TKBRep
    TKG2d
    TKDESTEP
    TKGeomAlgo
    TKGeomBase
    TKMesh
    TKShHealing
    TKTopAlgo
    TKXSBase
)

target_include_directories(numgeom
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}/include
)
