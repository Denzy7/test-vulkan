#include <stdint.h>
#include <vulkan/vulkan_core.h>

struct window
{
    int width;
    int height;
    const char* title;
    int running;

    /*RESERVED*/
    void* __winapi;
};

int window_init();
void window_terminate();

int window_create(const char* title, const VkInstance instance, VkSurfaceKHR* surface, struct window* window);
const char** window_extensions_get(uint32_t* extension_count); 
int window_poll(struct window* window);
void window_destroy(struct window* window);
