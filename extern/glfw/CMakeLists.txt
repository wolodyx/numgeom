FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.4
)

set(GLFW_BUILD_WAYLAND ${ENABLE_WAYLAND})
set(GLFW_BUILD_X11 ${ENABLE_XCB})
FetchContent_MakeAvailable(glfw)
