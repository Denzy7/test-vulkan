/* initialization to confirm vulkan is setup
 * properly 
 */
#include "vkapp.h"

#include <string.h>

int main(void)
{
    struct vkapp vkapp;
    memset(&vkapp, 0, sizeof(vkapp));
    vkapp_init(&vkapp);

    /* causes segv since it destroy device */
    /*vkapp_destroy(&vkapp);*/
    return 0;
}
