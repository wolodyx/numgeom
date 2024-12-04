set(SOURCE_FILES
  main.cpp
)

add_executable(app ${SOURCE_FILES})

include_directories("${GLFW_SOURCE_DIR}/deps")
target_link_libraries(app glfw OpenGL::GL)
