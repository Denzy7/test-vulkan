#include "window.h"

#include <stdlib.h>
#include <string.h>

#include "teskvk_android.h"

#include <android/looper.h>
#include <android/native_app_glue/android_native_app_glue.h>
#include <android/native_window.h>
#include <android/log.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_android.h>

struct winapi
{
    ANativeWindow* window;
};

static const char* exts[] =
{
    VK_KHR_SURFACE_EXTENSION_NAME,
    VK_KHR_ANDROID_SURFACE_EXTENSION_NAME
};

#define arysz(ary) sizeof(ary) / sizeof(ary[0])

void onAppCmd(struct android_app* app, int32_t cmd);
int _android_poll(struct android_app* app);

int window_init()
{
    return 1; 
}

void window_terminate()
{
    return;
}

const char** window_extensions_get(uint32_t* extension_count)
{
    *extension_count = arysz(exts);
    return exts;
}

int window_create(struct window* window)
{
    struct android_app* app = testvk_getapp();
    app->onAppCmd = onAppCmd;
    app->userData = window;

    struct winapi* winapi = calloc(1, sizeof(struct winapi));
    window->__winapi = winapi;
   
    while(winapi->window == NULL)
    {
        _android_poll(app);
    }

    window->RUNNING = 1;

    return 1;
}

void onAppCmd(struct android_app* app, int32_t cmd)
{
    struct window* window = app->userData;
    struct winapi* winapi = window->__winapi;
    LOGI("cmd: %d", cmd);

    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            ANativeWindow_acquire(app->window);
            winapi->window = app->window;        
            break;

        case APP_CMD_TERM_WINDOW:
            ANativeWindow_release(app->window);
            window->RUNNING = 0;
            break;
    }
}

int _android_poll(struct android_app* app)
{
    int events;
    struct android_poll_source* poll;
    while(ALooper_pollAll(0, NULL, &events, (void**)&poll) >= 0)
    {
        poll->process(app, poll);
    }
    return events;
}

int window_poll(struct window* window)
{
    struct android_app* app = testvk_getapp();
    return _android_poll(app);
}

void window_destroy(struct window* window)
{
    struct android_app* app = testvk_getapp();
    free(window->__winapi);
}

#ifndef WINDOW_VK
int window_vk(const VkInstance instance, VkSurfaceKHR* surface, struct window* window)
{
    VkAndroidSurfaceCreateInfoKHR vk_sfc_createinfo;
    struct android_app* app = testvk_getapp();

    memset(&vk_sfc_createinfo, 0, sizeof(vk_sfc_createinfo));
    vk_sfc_createinfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
    vk_sfc_createinfo.window = app->window;
    if(vkCreateAndroidSurfaceKHR(instance, &vk_sfc_createinfo, NULL, surface) != VK_SUCCESS)
    {
        LOGE("cannot vkCreateAndroidSurfaceKHR");
        return 0;
    }
    return 1;
}
#endif



