if(NOT geometrictools_FOUND)
  FetchContent_Declare(
    geometrictools
    GIT_REPOSITORY https://github.com/davideberly/GeometricTools.git
    GIT_TAG GTE-version-8.2
  )

  FetchContent_MakeAvailable(geometrictools)

  # The GeometricTools repository keeps its CMakeLists.txt in the GTE/
  # subdirectory, not in the repository root. As a result,
  # FetchContent_MakeAvailable() does not add any subdirectory and no targets
  # are created. The gtmathematics library is header-only, so we create the
  # INTERFACE target manually and point it at the Mathematics headers.
  if(NOT TARGET gtmathematics)
    add_library(gtmathematics INTERFACE)
    target_include_directories(gtmathematics
      INTERFACE
        ${geometrictools_SOURCE_DIR}/GTE
    )
  endif()
endif()
