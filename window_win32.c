#include "window.h"

#include <windows.h>
#include <string.h>
#include <stdio.h>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>

WNDCLASSW wc;
static const wchar_t* wc_class = L"WINDOW_WIN32";
static const char* exts[] =
{
    VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
    VK_KHR_SURFACE_EXTENSION_NAME
};
struct winapi
{
    HWND hwnd;    
    MSG msg;
};

LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    struct window* window;
    CREATESTRUCTW* createstruct;

    if(uMsg == WM_CREATE)
    {
        createstruct = (CREATESTRUCTW*)lParam;
        window = createstruct->lpCreateParams;
    }else
    {
        window = (struct window*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }

    switch (uMsg) {
        case WM_CLOSE:
            window->RUNNING = 0;
            return 1;
        default: 
            return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
}
int window_init()
{
    ZeroMemory(&wc, sizeof(wc));
    wc.hInstance = GetModuleHandle(NULL);;
    wc.lpszClassName = wc_class;
    wc.lpfnWndProc = WndProc;
    wc.style = CS_OWNDC;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    if(!RegisterClassW(&wc))
    {
        perror("RegisterClassExW");
        return 0;
    }

    return 1;
}
void window_terminate()
{
    UnregisterClassW(wc_class, wc.hInstance);
}
const char** window_extensions_get(uint32_t* extension_count)
{
    *extension_count = sizeof(exts) / sizeof(exts[0]);
    return exts;
}

int window_create(struct window* window)
{
    struct winapi winapi;
    wchar_t* wcs;
    size_t wcs_len = 0;

    if(window->title != NULL)   
        wcs_len = mbstowcs(NULL, window->title, 0);

    wcs = calloc(wcs_len + 1, sizeof(wchar_t));

    if(mbstowcs(wcs, window->title, wcs_len + 1) == sizeof(size_t) - 1)
    {
        perror("mbstowcs");
        return 0;
    }
    winapi.hwnd = CreateWindowExW(
            0, 
            wc.lpszClassName,
            wcs, 
            WS_OVERLAPPEDWINDOW, 
            0, 0, 
            window->width, window->height, 
            NULL, 
            NULL, 
            wc.hInstance,
            NULL);
    if(winapi.hwnd == NULL)
    {
        perror("CreateWindowExW");
        return 0;
    }
    ShowWindow(winapi.hwnd, SW_NORMAL);
    UpdateWindow(winapi.hwnd);
    SetWindowLongPtr(winapi.hwnd, GWLP_USERDATA, (LONG_PTR)window);
    window->RUNNING = 1;
    window->__winapi = malloc(sizeof(struct winapi));
    memcpy(window->__winapi, &winapi, sizeof(struct winapi));

    return 1;

}
int window_poll(struct window* window)
{
    struct winapi* winapi = window->__winapi;

    PeekMessageW(&winapi->msg, winapi->hwnd, 0, 0, PM_REMOVE);
    DispatchMessageW(&winapi->msg);
    return 1;
}
void window_destroy(struct window* window)
{
    struct winapi* winapi = window->__winapi;

    DestroyWindow(winapi->hwnd);
}

int window_vk(const VkInstance instance, VkSurfaceKHR* surface, struct window* window)
{
    struct winapi* winapi = window->__winapi;

    VkWin32SurfaceCreateInfoKHR createinfo;

    memset(&createinfo, 0, sizeof(VkWin32SurfaceCreateInfoKHR));
    createinfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createinfo.hwnd = winapi->hwnd;
    createinfo.hinstance = wc.hInstance;

    if(vkCreateWin32SurfaceKHR(instance, &createinfo, NULL, surface) != VK_SUCCESS)
    {
        perror("vkCreateWin32SurfaceKHR");
        return 1;
    }
    return 1;
}
