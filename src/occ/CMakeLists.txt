set(PUBLIC_HEADERS
  classificate.h
  drawable_occshape.h
  drawable_polytriangulation.h
  enums.h
  ijk.h
  loadusingocc.h
  octree.h
  quadtree.h
  removefaces.h
  sceneobject_tdocstd_document.h
  tesselate.h
  utilities.h
)
list(TRANSFORM PUBLIC_HEADERS PREPEND "include/numgeom/")

set(SOURCE_FILES
  ${PUBLIC_HEADERS}
  classificate.cc
  command_changevisibility.cc   command_changevisibility.h
  command_occ.cc                command_occ.h
  curvemetriclength.cc          curvemetriclength.h
  drawable_occshape.cc
  drawable_polytriangulation.cc
  ffextintersections.cc         ffextintersections.h
  intersections.cc              intersections.h
  iterator_ijk.cc               iterator_ijk.h
  loadusingocc.cc
  octree.cc
  quadtree.cc
  removefaces.cc
  sceneobject_tdocstd_document.cc
  tesselate.cc
  utilities.cc
  workbench_occ.cc              workbench_occ.h
  writevtk.cc                   writevtk.h
)

add_library(occ SHARED ${SOURCE_FILES})
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
  target_link_libraries(occ
    PRIVATE
      TKIGES
      TKRWMesh
      TKSTEP
      TKSTL
      TKXDEIGES
      TKXDESTEP
  )
  target_include_directories(occ PUBLIC ${OpenCASCADE_INCLUDE_DIR})
else()
  target_link_libraries(occ
    PRIVATE
      TKDEGLTF
      TKDEIGES
      TKDEOBJ
      TKDESTEP
      TKDESTL
      TKDEVRML
  )
  target_compile_definitions(occ PUBLIC NUMGEOM_OCC_LOAD_VRML)
endif()

target_include_directories(occ
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}/include
)

install(TARGETS occ)
