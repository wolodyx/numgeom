set(PUBLIC_HEADERS
  include/numgeom/classificate.h
  include/numgeom/drawable_occshape.h
  include/numgeom/drawable_polytriangulation.h
  include/numgeom/enums.h
  include/numgeom/ijk.h
  include/numgeom/iterator.h
  include/numgeom/loadusingocc.h
  include/numgeom/octree.h
  include/numgeom/quadtree.h
  include/numgeom/removefaces.h
  include/numgeom/sceneobject_polytriangulation.h
  include/numgeom/sceneobject_tdocstd_document.h
  include/numgeom/tesselate.h
  include/numgeom/utilities.h
)

set(SOURCE_FILES
  ${PUBLIC_HEADERS}
  classificate.cc
  curvemetriclength.cc      curvemetriclength.h
  drawable_occshape.cc
  drawable_polytriangulation.cc
  ffextintersections.cc     ffextintersections.h
  intersections.cc          intersections.h
  iterator_ijk.cc           iterator_ijk.h
  loadusingocc.cc
  octree.cc
  quadtree.cc
  removefaces.cc
  sceneobject_polytriangulation.cc
  sceneobject_tdocstd_document.cc
  tesselate.cc
  utilities.cc
  writevtk.cc               writevtk.h
)

add_library(occ ${SOURCE_FILES})
add_library(numgeom::occ ALIAS occ)

#set_target_properties(occ PROPERTIES PUBLIC_HEADER "${PUBLIC_HEADERS}")

generate_export_header(occ
  EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/include/numgeom/occ_export.h
)

target_link_libraries(occ
  PUBLIC
    numgeom::core
    TKMath
    TKG3d
    TKernel
  PRIVATE
    numgeom::framework
    nlohmann_json
    TKBO
    TKBRep
    TKG2d
    TKGeomAlgo
    TKGeomBase
    TKMesh
    TKShHealing
    TKTopAlgo
    TKXSBase
)

target_compile_definitions(occ PUBLIC USE_NUMGEOM_MODULE_OCC)

if(${OpenCASCADE_VERSION} VERSION_LESS "7.8.0")
  target_link_libraries(occ PRIVATE TKSTEP TKXDESTEP TKSTL)
  target_include_directories(occ PUBLIC ${OpenCASCADE_INCLUDE_DIR})
else()
  target_link_libraries(occ PRIVATE TKDESTEP TKDESTL)
endif()

target_include_directories(occ
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}/include
)

install(TARGETS occ)
