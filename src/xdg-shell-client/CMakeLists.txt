add_library(xdg-shell-client xdg-shell-protocol.c)

target_include_directories(xdg-shell-client
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)


# wayland-scanner client-header \
#     /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml \
#     xdg-shell-client-protocol.h

# wayland-scanner private-code \
#     /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml \
#     xdg-shell-protocol.c
