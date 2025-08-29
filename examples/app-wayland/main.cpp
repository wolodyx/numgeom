#include <linux/input.h>

#include "numgeom/common.h"
#include "numgeom/vkrender.h"
#include "numgeom/connect-wayland.h"

static uint32_t width = 1024, height = 768;
extern struct model cube_model;

int main(int argc, char* argv[])
{
    struct vkcube vc;
    vc.model = cube_model;
    vc.width = width;
    vc.height = height;
    gettimeofday(&vc.start_tv, nullptr);
    wl_connection wl;
    init_wayland_display(&vc, &wl);
    mainloop_wayland(&vc, &wl);
    return 0;
}
