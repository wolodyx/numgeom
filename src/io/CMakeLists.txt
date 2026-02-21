set(PUBLIC_HEADERS
  include/numgeom/loadfromvtk.h
  include/numgeom/loadtotrimesh.h
  include/numgeom/writetovtk.h
)

set(SOURCE_FILES
  ${PUBLIC_HEADERS}
  loadfromvtk.cpp
  loadtotrimesh.cpp
  writetovtk.cpp
)

if(OpenCASCADE_FOUND)
  list(APPEND PUBLIC_HEADERS
    include/numgeom/loadusingocc.h
  )
  list(APPEND SOURCE_FILES
    loadusingocc.cpp
  )
endif()

add_library(io ${SOURCE_FILES})
add_library(numgeom::io ALIAS io)

set_target_properties(io PROPERTIES PUBLIC_HEADER "${PUBLIC_HEADERS}")

generate_export_header(io
  EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/include/numgeom/numgeomio_export.h
)

target_include_directories(io
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}/include
)

target_link_libraries(io
  PUBLIC
    numgeom::core
)

flex_target(vtk_scanner vtk.l ${CMAKE_CURRENT_BINARY_DIR}/vtk_scanner.c)
bison_target(vtk_parser vtk.y ${CMAKE_CURRENT_BINARY_DIR}/vtk_parser.c)
add_flex_bison_dependency(vtk_scanner vtk_parser)

target_sources(io
  PRIVATE
    ${BISON_vtk_parser_OUTPUTS}
    ${FLEX_vtk_scanner_OUTPUTS}
)

target_include_directories(io
  PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
)

if(OpenCASCADE_FOUND)
  target_link_libraries(io
    PRIVATE
      TKBRep
      TKDESTEP
      numgeom::numgeom
  )
endif()
