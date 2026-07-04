FetchContent_Declare(
  httplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib
  GIT_TAG v0.48.0
)
option(HTTPLIB_INSTALL "" OFF)
FetchContent_MakeAvailable(httplib)
