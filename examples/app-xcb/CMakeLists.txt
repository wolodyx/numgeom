set(SOURCE_FILES
  main.cpp
)

add_executable(app-xcb ${SOURCE_FILES})

target_link_libraries(app-xcb
  numgeom::render
)

