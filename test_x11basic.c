#include "window.h"

#include <stdio.h>
#include <string.h>

int main()
{
    struct window window;
    memset(&window, 0, sizeof(struct window));
    window.width = 854;
    window.height = 480;
    window.title = "Hello Vulkan";

    if(!window_init())
        return 1;

    if(!window_create(&window))
        return 1;

    while(window.RUNNING)
    {
        window_poll(&window);
    }

    window_terminate();

    return 0;
}

