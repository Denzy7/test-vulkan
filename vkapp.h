#ifndef VK_H
#define VK_H

#include <vulkan/vulkan_core.h>
#ifdef _cplusplus
extern "C" {
#endif


#define arysz(ary) sizeof(ary)/sizeof(ary[0])

struct vkapp{
    /* USER DEFINED */

    int uselayers;
    int usevsync;
    int bufferedframes; 
    VkSurfaceKHR surface;


    /* READ ONLY */

    /* instance related */
    VkInstance INSTANCE;
    VkPhysicalDevice PHYDEV;
    VkPhysicalDeviceFeatures PHYDEV_FEAT;
    VkDevice DEV;
    VkQueue QGFX;
    uint32_t QGFX_ID;
    /* debugging */
    PFN_vkCreateDebugReportCallbackEXT vkapp_CreateDebugReportCallbackEXT;
    PFN_vkDestroyDebugReportCallbackEXT vkapp_DestroyDebugReportCallbackEXT;
    VkDebugReportCallbackEXT vkapp_DebugReportCallbackEXT;
    PFN_vkCreateDebugUtilsMessengerEXT vkapp_CreateDebugUtilsMessengerEXT;
    PFN_vkDestroyDebugUtilsMessengerEXT vkapp_DestroyDebugUtilsMessengerEXT;
    VkDebugUtilsMessengerEXT vkapp_DebugUtilsMessengerEXT;

    /* swapchain releated */
    VkSurfaceFormatKHR SFCFMT;
    VkSurfaceCapabilitiesKHR SFCCAP;
    VkSwapchainKHR SWAP;
    /*VkSwapchainKHR SWAP_OLD;*/
    VkImage* SWAP_IMGS;
    VkImageView* SWAP_IMGVIEWS;
    uint32_t SWAP_IMGCOUNT;
    VkFramebuffer* FBUF;
    VkRenderPass RPASS;
};

int vkapp_init(struct vkapp* vkapp);
int vkapp_destroy(struct vkapp* vkapp);

int vkapp_dewice(struct vkapp* vkapp);
int vkapp_swapchain(struct vkapp* vkapp);

int mkbuffer(VkPhysicalDevice phydev, VkDevice dev, VkDeviceSize size, VkBufferUsageFlags usage,
        VkMemoryPropertyFlags props, VkBuffer* buffer,
        VkDeviceMemory* memory);
int bufcpy(VkDevice dev, VkCommandPool pool, VkQueue queue, VkBuffer src, VkBuffer dst, VkDeviceSize size);
int hirestime(double* t);

#ifdef _cplusplus
}
#endif
#endif
