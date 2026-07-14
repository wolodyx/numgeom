if(TARGET volk::volk)
  # System volk with full target - nothing to do
elseif(TARGET volk::volk_headers)
  # System volk only provides volk::volk_headers - create alias
  add_library(volk ALIAS volk::volk_headers)
  add_library(volk::volk ALIAS volk::volk_headers)
else()
  FetchContent_Declare(volk
    GIT_REPOSITORY https://github.com/zeux/volk.git
    GIT_TAG        1.4.304
  )
  FetchContent_MakeAvailable(volk)
endif()
