#ifndef WINDOW_H
#define WINDOW_H
#include <stdint.h>

#ifdef WINDOW_VK
#include <vulkan/vulkan_core.h>
#endif

struct window
{
    int width;
    int height;
    const char* title;

    int RUNNING;

    /*RESERVED*/
    void* __winapi;
};
#ifdef _cplusplus
extern "C" {
#endif
int window_init(void);
void window_terminate(void);
const char** window_extensions_get(uint32_t* extension_count); 

int window_create(struct window* window);
int window_poll(struct window* window);
void window_destroy(struct window* window);

#ifdef WINDOW_VK
int window_vk(const VkInstance instance, VkSurfaceKHR* surface, struct window* window);
#endif
#ifdef _cplusplus
}
#endif
#endif
