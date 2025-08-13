#include <linux/input.h>

#include "common.h"
#include "vkrender.h"

static uint32_t width = 1024, height = 768;
extern struct model cube_model;

int main(int argc, char *argv[])
{
   struct vkcube vc;

   vc.model = cube_model;
   vc.wl.surface = nullptr;
   vc.width = width;
   vc.height = height;
   gettimeofday(&vc.start_tv, nullptr);

   init_display(&vc);
   mainloop(&vc);

   return 0;
}
