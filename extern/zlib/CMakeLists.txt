if(NOT ZLIB_FOUND)
  FetchContent_Declare(zlib
    GIT_REPOSITORY https://github.com/madler/zlib
    GIT_TAG        v1.3.2
  )
  FetchContent_MakeAvailable(zlib)
endif()
