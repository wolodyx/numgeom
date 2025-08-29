set(SOURCE_FILES
  main.cpp
)

add_executable(app-wayland ${SOURCE_FILES})

target_link_libraries(app-wayland
  numgeom::render
)
