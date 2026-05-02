include(FetchContent)

FetchContent_Declare(stb
  GIT_REPOSITORY https://github.com/nothings/stb.git
  GIT_TAG master
)

FetchContent_MakeAvailable(stb)

# Get the source directory from the populated content
if(NOT STB_SOURCE_DIR)
  # Try to get the source directory from the fetched content
  get_property(STB_SOURCE_DIR GLOBAL PROPERTY _FetchContent_stb_SOURCE_DIR)
  if(NOT STB_SOURCE_DIR)
    set(STB_SOURCE_DIR ${stb_SOURCE_DIR})
  endif()
endif()

message(STATUS "stb source directory: ${STB_SOURCE_DIR}")

if(NOT TARGET stb)
  add_library(stb INTERFACE)
  target_include_directories(stb INTERFACE ${STB_SOURCE_DIR})
  
  add_library(stb_image INTERFACE)
  target_link_libraries(stb_image INTERFACE stb)
  
  add_library(stb_image_write INTERFACE)
  target_link_libraries(stb_image_write INTERFACE stb)
  
  add_library(stb_truetype INTERFACE)
  target_link_libraries(stb_truetype INTERFACE stb)
  
  set_target_properties(stb stb_image stb_image_write stb_truetype
    PROPERTIES
      EXPORT_NAME stb
  )
endif()
