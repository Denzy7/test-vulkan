#include "window.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <X11/Xlib.h>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_xlib.h>

Display* dpy;
Atom wm_delete;
XEvent ev;

static const char* neededexts_inst_str[]=
{
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
    VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
    VK_KHR_SURFACE_EXTENSION_NAME
};

struct winapi
{
    XSetWindowAttributes swa;
    Window win;
    XEvent win_ev;
};

int x11error(Display* _dpy, XErrorEvent* err)
{
    printf("xerr:%d", err->error_code);
    return 1;
}


int window_init()
{
    dpy = XOpenDisplay(NULL);
    if(dpy == NULL)
    {
        perror("XOpenDisplay");
        return 0;
    }

    wm_delete = XInternAtom(dpy, "WM_DELETE_WINDOW", False);

    XSetErrorHandler(x11error);
    return 1;
}

void window_terminate()
{
    XCloseDisplay(dpy);
}

const char** window_extensions_get(uint32_t* extension_count)
{
    *extension_count = sizeof(neededexts_inst_str) / sizeof(neededexts_inst_str[0]);
    return neededexts_inst_str;
}

int window_create(const char* title, const VkInstance instance, VkSurfaceKHR* surface, struct window* window)
{
    struct winapi* winapi;
    VkXlibSurfaceCreateInfoKHR xlib_sfc_createinfo;


    window->__winapi = malloc(sizeof(struct winapi));


    winapi = window->__winapi;
    window->title = title;

    /* create x11 window */
    winapi->swa.event_mask = ExposureMask |
        ButtonPressMask | ButtonReleaseMask |
        KeyPressMask | KeyReleaseMask |
        PointerMotionMask;

    winapi->win = XCreateWindow(dpy, DefaultRootWindow(dpy), 0, 0, 
            window->width, window->height, 
            0,
            CopyFromParent, InputOutput, CopyFromParent,
            CWEventMask, &winapi->swa);

    if(!winapi->win)
    {
        perror("XCreateWindow");
        return 0;
    }  
    XSetWMProtocols(dpy, winapi->win, &wm_delete, 1);
    XMapWindow(dpy,winapi->win);
    XStoreName(dpy, winapi->win, window->title);

    memset(&xlib_sfc_createinfo, 0, sizeof(VkXlibSurfaceCreateInfoKHR));
    xlib_sfc_createinfo.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
    xlib_sfc_createinfo.dpy = dpy;
    xlib_sfc_createinfo.window = winapi->win;

    if(vkCreateXlibSurfaceKHR(instance, &xlib_sfc_createinfo, NULL, surface) != VK_SUCCESS)
    {
        perror("vkCreateXlibSurfaceKHR fail");
        return 0;
    }

    return 1;
}

int window_poll(struct window* window)
{
    struct winapi* winapi = window->__winapi;
    if(!XPending(dpy))
        return 0;

    XPeekEvent(dpy, &ev);
    if(ev.type == ClientMessage && ev.xclient.data.l[0] == wm_delete && ev.xclient.window == winapi->win)
    {
        window->running = 1;
        return 0;
    }

    if(!XCheckWindowEvent(dpy, winapi->win, winapi->swa.event_mask, &winapi->win_ev))
        return 0;

    return 1;
}

void window_destroy(struct window* window)
{
    struct winapi* winapi = window->__winapi;
    XUnmapWindow(dpy, winapi->win);
    XDestroyWindow(dpy, winapi->win);

    free(window->__winapi);
}



