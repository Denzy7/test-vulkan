#include "vkapp.h"
#include "window.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#endif


struct needed_ext_or_layer
{
    const char* ext_or_layer;
    int has;
};

/* instance extensions used by vkapp*/
static const char* neededexts_inst_vkapp_str[] =
{
    /* support VK_EXT_debug_report for old android vulkan. debug_utils
     * seems to be a device extension for whatever reason (on android 
     * at least), vkapp_init will switch to debug utils if availabe*/
    VK_EXT_DEBUG_REPORT_EXTENSION_NAME
};

static struct needed_ext_or_layer neededexts_inst_vkapp[arysz(neededexts_inst_vkapp_str)];

static const char* neededexts_dev_str[] =
{
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};
static struct needed_ext_or_layer neededexts_dev[arysz(neededexts_dev_str)];

static const char* neededlayers_str[] =
{
    "VK_LAYER_KHRONOS_validation",
    /*"VK_LAYER_MANGOHUD_overlay_x86_64"*/
    /*"VK_LAYER_LUNARG_api_dump"*/
};
static struct needed_ext_or_layer neededlayers [arysz(neededlayers_str)];

static VKAPI_ATTR VkBool32 VKAPI_CALL debugutilscallback(VkDebugUtilsMessageSeverityFlagsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type, const VkDebugUtilsMessengerCallbackDataEXT* cbdata, void* userdata )
{
    printf("\ndebugutilscallback:> ");
    if(severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
        printf("info: %s", cbdata->pMessage);
    if(severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        printf("warn: %s", cbdata->pMessage);
    if(severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        printf("errr: %s", cbdata->pMessage);
        exit(1);
    }
    printf("\n");
    return VK_FALSE;
}
static VKAPI_ATTR VkBool32 debugreportcallback(
    VkDebugReportFlagsEXT                       flags,
    VkDebugReportObjectTypeEXT                  objectType,
    uint64_t                                    object,
    size_t                                      location,
    int32_t                                     messageCode,
    const char*                                 pLayerPrefix,
    const char*                                 pMessage,
    void*                                       pUserData)
{
    printf("\ndebugreportcallback:> ");
    if(flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT)
        printf("info: %s", pMessage);
    if(flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
        printf("warn: %s", pMessage);
    if(flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
    {
        printf("errr: %s", pMessage);
        exit(1);
    }
    printf("\n");
    return VK_FALSE;
}

uint32_t findmem(VkPhysicalDevice phydev, uint32_t type, VkMemoryPropertyFlags flag)
{
    uint32_t i;
    VkPhysicalDeviceMemoryProperties vk_phydev_memprops;
    vkGetPhysicalDeviceMemoryProperties(phydev, &vk_phydev_memprops);
for(i = 0; i < vk_phydev_memprops.memoryTypeCount; i++)
    {
        if((type & (1 << i) &&
            (vk_phydev_memprops.memoryTypes[i].propertyFlags & flag) == flag)) 
        {
            return i;
        }
    }

    return 0;
}

int mkbuffer(VkPhysicalDevice phydev, VkDevice dev, VkDeviceSize size, VkBufferUsageFlags usage,
        VkMemoryPropertyFlags props, VkBuffer* buffer,
        VkDeviceMemory* memory)
{
    VkBufferCreateInfo createinfo;
    VkMemoryRequirements require;
    VkMemoryAllocateInfo allocinfo;

    memset(&createinfo, 0, sizeof(createinfo));
    createinfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    createinfo.size = size;
    createinfo.usage = usage;
    createinfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if(vkCreateBuffer(dev, &createinfo, NULL, buffer) 
            != VK_SUCCESS)
    {
        perror("vkCreateBuffer");
        return 0;
    }

    memset(&require, 0, sizeof(require));
    vkGetBufferMemoryRequirements(dev, *buffer, &require);
    memset(&allocinfo, 0, sizeof(allocinfo));
    allocinfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocinfo.allocationSize = require.size;
    allocinfo.memoryTypeIndex = findmem(phydev,require.memoryTypeBits, props);

    if(vkAllocateMemory(dev, &allocinfo, NULL, memory) !=
            VK_SUCCESS)
    {
        perror("vkAllocateMemory");
        return 0;
    }
    
    vkBindBufferMemory(dev, *buffer, *memory, 0);
    return 1;
}

int bufcpy(VkDevice dev, VkCommandPool pool, VkQueue queue, VkBuffer src, VkBuffer dst, VkDeviceSize size)
{
    VkCommandBufferAllocateInfo allocinfo;
    VkCommandBuffer cmdbuf;
    VkCommandBufferBeginInfo begininfo;
    VkBufferCopy copyregioninfo;
    VkSubmitInfo submitinfo;

    memset(&allocinfo, 0, sizeof(allocinfo));
    allocinfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocinfo.commandPool = pool;
    allocinfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocinfo.commandBufferCount = 1;

    if(vkAllocateCommandBuffers(dev, &allocinfo, &cmdbuf) != VK_SUCCESS)
    {
        perror("vkAllocateCommandBuffers failed");
        return 0;
    }

    begininfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begininfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmdbuf, &begininfo);
    memset(&copyregioninfo, 0, sizeof(copyregioninfo));
    /* offsets are 0 */
    copyregioninfo.size = size;
    vkCmdCopyBuffer(cmdbuf, src, dst, 1, &copyregioninfo);
    vkEndCommandBuffer(cmdbuf);

    memset(&submitinfo, 0, sizeof(submitinfo));
    submitinfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitinfo.commandBufferCount = 1;
    submitinfo.pCommandBuffers = &cmdbuf;

    vkQueueSubmit(queue, 1, &submitinfo, VK_NULL_HANDLE);
    vkDeviceWaitIdle(dev);

    vkFreeCommandBuffers(dev, pool, 1, &cmdbuf);

    return 1;
}

int vkapp_init(struct vkapp* vkapp)
{
    uint32_t i, j, ru32;
    VkResult vkres;
    VkBool32 rb32;

    VkApplicationInfo vk_appinfo;
    uint32_t windowexts;
    VkInstanceCreateInfo vk_inst_createinfo;
    VkDebugUtilsMessengerCreateInfoEXT vk_dbgmsg_createinfo;
    VkDebugReportCallbackCreateInfoEXT vk_dbgcb_createinfo;
    const char** neededexts_inst_str; 
    const char** neededexts_inst_window_str; 

    struct needed_ext_or_layer* neededexts_inst;
    uint32_t neededext_inst_count = 0;
    VkExtensionProperties* vk_extprops_inst;
    VkLayerProperties* vk_layerprops = NULL;

    VkPhysicalDevice* vk_phydevs;
    VkExtensionProperties* vk_extprops_dev;
    VkPhysicalDeviceProperties* vk_phydevs_props;
    VkPhysicalDeviceFeatures* vk_phydevs_feats;
    int usephydev = -1;

    /* fill needed exts and layers */
    /* window, debug(neededext_inst[0]), vkapp are merged to one big string array * */
    neededexts_inst_window_str = window_extensions_get(&windowexts);
    neededext_inst_count = windowexts + arysz(neededexts_inst_vkapp_str);
    neededexts_inst_str = malloc(neededext_inst_count * sizeof(char*)); 
    neededexts_inst = calloc(neededext_inst_count, sizeof(struct needed_ext_or_layer));
    for(i = 0; i < neededext_inst_count; i++)
    {
        if(i < windowexts)
            neededexts_inst_str[i] = neededexts_inst_window_str[i];
        if(i >= windowexts && i < arysz(neededexts_inst_vkapp_str) + windowexts)
            neededexts_inst_str[i] = neededexts_inst_vkapp_str[i - windowexts];

        neededexts_inst[i].ext_or_layer = neededexts_inst_str[i];
    }

    vkEnumerateInstanceExtensionProperties(NULL, &ru32, NULL);
    if(ru32 == 0)
    {
        perror("no instance extensions found. probably no vulkan loader in device!");
        return 0;
    }
    vk_extprops_inst = malloc(ru32 * sizeof(VkExtensionProperties));
    vkEnumerateInstanceExtensionProperties(NULL, &ru32, vk_extprops_inst);

    printf("\nexts_inst [%u]:\n", ru32 );
    for(i = 0; i < ru32; i++)
    {
        /* switch to debug utils */
        if(!strcmp(vk_extprops_inst[i].extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME)){
            neededexts_inst[i].ext_or_layer = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
            /* offset into windows exts. first vkapp instance extensioin after window is debug extension */
            neededexts_inst_str[windowexts] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
            neededexts_inst_vkapp_str[0] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
        }

        for(j = 0; j < neededext_inst_count; j++)
        {
            if(strcmp(neededexts_inst[j].ext_or_layer, vk_extprops_inst[i].extensionName) == 0)
                neededexts_inst[j].has = 1;
        }
        printf("%s\n", vk_extprops_inst[i].extensionName);
    }

    /* dev and layers can stay as so */
    memset(neededexts_dev, 0, sizeof(neededexts_dev));
    for(i = 0; i < arysz(neededexts_dev); i++)
    {
        neededexts_dev[i].ext_or_layer = neededexts_dev_str[i];
    }
    memset(neededlayers, 0, sizeof(neededlayers));
    for(i = 0; i < arysz(neededlayers); i++)
    {
        neededlayers[i].ext_or_layer = neededlayers_str[i];
    }
    
    for(i = 0; i < neededext_inst_count; i++)
    {
        if(!neededexts_inst[i].has)
        {
            fprintf(stderr, "missing needed instance extension: %s\n", neededexts_inst[i].ext_or_layer);
            return 0;
        }
    }

    vkEnumerateInstanceLayerProperties(&ru32, NULL);
    printf("\nlayers [%u]:\n", ru32);
    if(ru32 > 0)
    {
        vk_layerprops = malloc(sizeof(VkLayerProperties) * ru32);
        vkEnumerateInstanceLayerProperties(&ru32, vk_layerprops);
        for(i = 0; i < ru32; i++)
        {
            for(j = 0; j < arysz(neededlayers); j++)
            {

                if(strcmp(vk_layerprops[i].layerName, neededlayers[j].ext_or_layer) == 0)
                    neededlayers[j].has = 1;
            }

            printf("%s\n", vk_layerprops[i].layerName);
        }
    }

    if(vkapp->uselayers)
    {
        for(i = 0; i < arysz(neededlayers); i++)
        {
            if(!neededlayers[i].has){
                printf("missing needed layer: %s\n", neededlayers[i].ext_or_layer);
                /* are unfound layers a must???? */
                return 0;
            }
        }
    }

    memset(&vk_appinfo, 0, sizeof(VkApplicationInfo));
    memset(&vk_inst_createinfo, 0, sizeof(VkInstanceCreateInfo));
    memset(&vk_dbgmsg_createinfo, 0, sizeof(VkDebugUtilsMessengerCreateInfoEXT));

    vk_dbgmsg_createinfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    vk_dbgmsg_createinfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    vk_dbgmsg_createinfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    vk_dbgmsg_createinfo.pfnUserCallback = debugutilscallback;


    memset(&vk_dbgcb_createinfo, 0 , sizeof(vk_dbgcb_createinfo));
    vk_dbgcb_createinfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    vk_dbgcb_createinfo.flags = 
        VK_DEBUG_REPORT_INFORMATION_BIT_EXT |
        VK_DEBUG_REPORT_WARNING_BIT_EXT |
        VK_DEBUG_REPORT_ERROR_BIT_EXT;
    vk_dbgcb_createinfo.pfnCallback = debugreportcallback;
    
    vk_appinfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    vk_appinfo.pApplicationName = "Hello Vulkan";
    vk_appinfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
    vk_appinfo.pEngineName = "None";
    vk_appinfo.engineVersion = VK_MAKE_VERSION(0, 0, 1);
    vk_appinfo.apiVersion = VK_API_VERSION_1_0 ;

    vk_inst_createinfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    vk_inst_createinfo.pApplicationInfo = &vk_appinfo;
    vk_inst_createinfo.enabledExtensionCount = neededext_inst_count;
    vk_inst_createinfo.ppEnabledExtensionNames = neededexts_inst_str;
    if(vkapp->uselayers)
        vk_inst_createinfo.enabledLayerCount = arysz(neededlayers_str);
    vk_inst_createinfo.ppEnabledLayerNames = neededlayers_str;
    vk_inst_createinfo.pNext = &vk_dbgmsg_createinfo;
    if(!strcmp(neededexts_inst_vkapp_str[0], VK_EXT_DEBUG_REPORT_EXTENSION_NAME))
        vk_inst_createinfo.pNext = &vk_dbgcb_createinfo;

    vkres = vkCreateInstance(&vk_inst_createinfo, NULL, &vkapp->INSTANCE);
    if(vkres != VK_SUCCESS)
    {
        perror("vkCreateInstance");
        return 0;
    }

    if(!strcmp(neededexts_inst_vkapp_str[0], VK_EXT_DEBUG_REPORT_EXTENSION_NAME))
    {
        vkapp->vkapp_CreateDebugReportCallbackEXT =  (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(vkapp->INSTANCE, "vkCreateDebugReportCallbackEXT");
        vkapp->vkapp_DestroyDebugReportCallbackEXT =  (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(vkapp->INSTANCE, "vkDestroyDebugReportCallbackEXT");
        if(vkapp->vkapp_CreateDebugReportCallbackEXT == NULL || vkapp->vkapp_DestroyDebugReportCallbackEXT == NULL)
        {
            perror("couldn't procaddr vkCreateDebugReportCallbackEXT || vkDestroyDebugReportCallbackEXT");
            return 0;
        }
        vkapp->vkapp_CreateDebugReportCallbackEXT(vkapp->INSTANCE, &vk_dbgcb_createinfo, NULL, &vkapp->vkapp_DebugReportCallbackEXT);
    }else {
        vkapp->vkapp_CreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vkapp->INSTANCE, "vkCreateDebugUtilsMessengerEXT");
        vkapp->vkapp_DestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vkapp->INSTANCE, "vkDestroyDebugUtilsMessengerEXT");
        if(vkapp->vkapp_DestroyDebugUtilsMessengerEXT == NULL || vkapp->vkapp_CreateDebugUtilsMessengerEXT == NULL)
        {
            perror("couldn't procaddr vkCreateDebugUtilsMessengerEXT || vkDestroyDebugUtilsMessengerEXT");
            return 0;
        }
        vkapp->vkapp_CreateDebugUtilsMessengerEXT(vkapp->INSTANCE, &vk_dbgmsg_createinfo, NULL, &vkapp->vkapp_DebugUtilsMessengerEXT);
    }
    vkEnumeratePhysicalDevices(vkapp->INSTANCE, &ru32, NULL);
    if(ru32 == 0)
    {
        perror("vkEnumeratePhysicalDevices failed. device has no vulkan dewice");
        return 0;
    }
    vk_phydevs = malloc(ru32 * sizeof(VkPhysicalDevice));
    vk_phydevs_props = malloc(ru32 * sizeof(VkPhysicalDeviceProperties));
    vk_phydevs_feats = malloc(ru32 * sizeof(VkPhysicalDeviceFeatures));
    vkEnumeratePhysicalDevices(vkapp->INSTANCE, &ru32, vk_phydevs);

    printf("\ndevs [%d]:\n", ru32);
    for(i = 0; i < ru32; i++)
    {
        vkGetPhysicalDeviceProperties(vk_phydevs[i], &vk_phydevs_props[i]);
        vkGetPhysicalDeviceFeatures(vk_phydevs[i], &vk_phydevs_feats[i]);
        printf("dev:%s ver:v%u.%u.%u type:%d\n", 
                vk_phydevs_props[i].deviceName, 
                VK_VERSION_MAJOR(vk_phydevs_props[i].apiVersion), 
                VK_VERSION_MINOR(vk_phydevs_props[i].apiVersion), 
                VK_VERSION_PATCH(vk_phydevs_props[i].apiVersion), 
                vk_phydevs_props[i].deviceType);
        if(
                vk_phydevs_props[i].deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU || 
                vk_phydevs_props[i].deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU
                )
            usephydev = i;
    }

    /*use cpu */
    if(usephydev == -1)
        usephydev = 0;

    printf("\n\nusing %s.\n", vk_phydevs_props[usephydev].deviceName);

    memcpy(&vkapp->PHYDEV, &vk_phydevs[usephydev], sizeof(VkPhysicalDevice));
    memcpy(&vkapp->PHYDEV_FEAT, &vk_phydevs_feats[usephydev], sizeof(VkPhysicalDeviceFeatures));

    vkEnumerateDeviceExtensionProperties(vkapp->PHYDEV, NULL, &ru32, NULL);
    if(ru32 == 0)
    {
        perror("dewice has no extensions");
        return 0;
    }
    vk_extprops_dev = malloc(sizeof(VkExtensionProperties) * ru32);

    vkEnumerateDeviceExtensionProperties(vkapp->PHYDEV, NULL, &ru32, vk_extprops_dev);
    printf("\nexts_dev[%u]:\n", ru32);
    for(i = 0; i < ru32; i++)
    {
        for(j = 0; j < arysz(neededexts_dev); j++)
        {
            if(strcmp( vk_extprops_dev[i].extensionName, neededexts_dev[j].ext_or_layer) == 0)
                neededexts_dev[j].has = 1;
        }
        printf("%s\n",vk_extprops_dev[i].extensionName);
    }
    for(i = 0; i < arysz(neededexts_dev); i++)
    {
        if(!neededexts_dev[i].has)
        {
            fprintf(stderr, "missing needed device extension: %s\n", neededexts_dev[i].ext_or_layer);
            return 0;
        }
    }

    free(neededexts_inst_str);
    free(neededexts_inst);
    free(vk_extprops_inst);
    free(vk_extprops_dev);
    free(vk_layerprops);
    free(vk_phydevs);
    free(vk_phydevs_feats);
    free(vk_phydevs_props);

    return 1;
}

int vkapp_dewice(struct vkapp* vkapp)
{ 
    uint32_t i, ru32;
    VkBool32 rb32;
    VkQueueFamilyProperties* vk_queue_props;
    VkDeviceQueueCreateInfo vk_dev_queue_createinfo;
    const float vk_queue_prio = 1.0f;
    VkDeviceCreateInfo vk_dev_createinfo;

    vkGetPhysicalDeviceQueueFamilyProperties(vkapp->PHYDEV, &ru32, NULL);
    vk_queue_props = malloc(sizeof(VkQueueFamilyProperties) * ru32);
    vkGetPhysicalDeviceQueueFamilyProperties(vkapp->PHYDEV, &ru32, vk_queue_props);
    for(i = 0; i < ru32; i++)
    {
        vkGetPhysicalDeviceSurfaceSupportKHR(vkapp->PHYDEV, i, vkapp->surface, &rb32);
        if(vk_queue_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && rb32 == VK_TRUE)
        {
            vkapp->QGFX_ID = i;
            break;
        }
    }
    if(vkapp->QGFX_ID == -1)
    {
        perror("no queue w/ gfx bit and present support");
        return 0;
    }

    memset(&vk_dev_queue_createinfo, 0, sizeof(VkDeviceQueueCreateInfo));
    vk_dev_queue_createinfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    vk_dev_queue_createinfo.queueFamilyIndex = vkapp->QGFX_ID ;
    vk_dev_queue_createinfo.queueCount = 1;
    vk_dev_queue_createinfo.pQueuePriorities = &vk_queue_prio;

    memset(&vk_dev_createinfo, 0, sizeof(VkDeviceCreateInfo));
    vk_dev_createinfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    vk_dev_createinfo.pQueueCreateInfos = &vk_dev_queue_createinfo;
    vk_dev_createinfo.queueCreateInfoCount = 1;
    vk_dev_createinfo.pEnabledFeatures = &vkapp->PHYDEV_FEAT;
    vk_dev_createinfo.enabledExtensionCount = arysz(neededexts_dev_str);
    vk_dev_createinfo.ppEnabledExtensionNames = neededexts_dev_str; 

    /* deprecated? */
    /*vk_dev_createinfo.ppEnabledLayerNames = vk_inst_createinfo.ppEnabledLayerNames;*/
    /*vk_dev_createinfo.enabledLayerCount = vk_inst_createinfo.enabledLayerCount;*/
    
    if(vkCreateDevice(vkapp->PHYDEV, &vk_dev_createinfo, NULL, &vkapp->DEV) != VK_SUCCESS)
    {
        perror("vkCreateDevice");
        return 0;
    }

    free(vk_queue_props);
    return 1;
}

void _cleanup_swapchain(struct vkapp* vkapp)
{
    uint32_t i;

    vkDeviceWaitIdle(vkapp->DEV);

    for(i = 0; i < vkapp->SWAP_IMGCOUNT; i++)
    {
        vkDestroyImageView(vkapp->DEV, vkapp->SWAP_IMGVIEWS[i], NULL);
        vkDestroyFramebuffer(vkapp->DEV, vkapp->FBUF[i], NULL);
    }
    vkDestroyRenderPass(vkapp->DEV, vkapp->RPASS, NULL);
    vkDestroySwapchainKHR(vkapp->DEV, vkapp->SWAP, NULL);
}

int vkapp_swapchain(struct vkapp* vkapp)
{
    uint32_t i, j;
    uint32_t ru32;
    VkSurfaceFormatKHR* vk_sfc_formats;
    VkPresentModeKHR* vk_sfc_presentmodes;

    VkSwapchainCreateInfoKHR vk_swapchain_createinfo;
    VkImageViewCreateInfo vk_swapchain_imgs_views_createinfo;
    
    VkFramebufferCreateInfo vk_fbuf_createinfo;
    VkAttachmentDescription vk_colorattach;
    VkAttachmentReference vk_colorattach_ref;
    VkSubpassDescription vk_colorattach_subpass;
    VkSubpassDependency vk_subpass_deps;
    VkRenderPassCreateInfo vk_renderpass_createinfo;

    if(vkapp->SWAP != VK_NULL_HANDLE)
        _cleanup_swapchain(vkapp);

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkapp->PHYDEV, vkapp->surface, &vkapp->SFCCAP);
    vkGetPhysicalDeviceSurfaceFormatsKHR(vkapp->PHYDEV, vkapp->surface, &ru32, NULL);
    if(ru32 == 0)
    {
        perror("vkGetPhysicalDeviceSurfaceFormatsKHR. none found");
        return 0;
    }
    vk_sfc_formats = malloc(sizeof(VkSurfaceFormatKHR) * ru32);
    vkGetPhysicalDeviceSurfaceFormatsKHR(vkapp->PHYDEV, vkapp->surface, &ru32, vk_sfc_formats);

    j = -1;
    for(i = 0; i < ru32; i++)
    {
        if(vk_sfc_formats[i].format == VK_FORMAT_B8G8R8A8_SRGB &&
                vk_sfc_formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            j = i;
            break;
        }
    }
    
    if(j == -1)
    {
        j = 0;
        printf("no suitable(bgr8888 and linear) surface found, using 0\n");
    }
    memcpy(&vkapp->SFCFMT, &vk_sfc_formats[j], sizeof(VkSurfaceFormatKHR));
    printf("new surface. w:%u,h:%u\n", 
            vkapp->SFCCAP.currentExtent.width,
            vkapp->SFCCAP.currentExtent.height);

    vkGetPhysicalDeviceSurfacePresentModesKHR(vkapp->PHYDEV, vkapp->surface, &ru32, NULL);
    if(ru32 == 0)
    {
        perror("vkGetPhysicalDeviceSurfacePresentModesKHR. none found");
        return 0;
    }
    vk_sfc_presentmodes = malloc(sizeof(VkPresentModeKHR) * ru32);
    vkGetPhysicalDeviceSurfacePresentModesKHR(vkapp->PHYDEV, vkapp->surface, &ru32, vk_sfc_presentmodes);

    j = -1;
    for(i = 0; i < ru32; i++)
    {
        if(vk_sfc_presentmodes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            j = i;
            break;
        }
    }
    /*use vsync*/
    if(j == -1)
        j = VK_PRESENT_MODE_FIFO_KHR;

    memset(&vk_swapchain_createinfo, 0, sizeof(VkSwapchainCreateInfoKHR));
    vk_swapchain_createinfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    vk_swapchain_createinfo.presentMode = vk_sfc_presentmodes[j];
    vk_swapchain_createinfo.surface = vkapp->surface;
    vk_swapchain_createinfo.minImageCount = vkapp->SFCCAP.minImageCount + 1;
    vk_swapchain_createinfo.imageFormat = vkapp->SFCFMT.format;
    vk_swapchain_createinfo.imageColorSpace = vkapp->SFCFMT.colorSpace;
    vk_swapchain_createinfo.imageExtent = vkapp->SFCCAP.currentExtent;
    vk_swapchain_createinfo.imageArrayLayers = 1;
    vk_swapchain_createinfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    vk_swapchain_createinfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vk_swapchain_createinfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    for(i = 0; i < VK_COMPOSITE_ALPHA_FLAG_BITS_MAX_ENUM_KHR; i++)
    {
        if(vkapp->SFCCAP.supportedCompositeAlpha & (1 << i))
        {
            vk_swapchain_createinfo.compositeAlpha = (1 << i);
            break;
        }
    }
    if(vk_swapchain_createinfo.compositeAlpha == 0)
    {
        fprintf(stderr, "cannot find compatible composite alpha\n");
        return 0;
    }
    vk_swapchain_createinfo.clipped = VK_TRUE;
    if(vkCreateSwapchainKHR(vkapp->DEV, &vk_swapchain_createinfo, NULL, &vkapp->SWAP) != VK_SUCCESS)
    {
        printf("vkCreateSwapchain fail\n");
        return 0;
    }

    vkGetSwapchainImagesKHR(vkapp->DEV, vkapp->SWAP, &ru32, NULL);
    if(vkapp->SWAP_IMGCOUNT == 0){
        vkapp->SWAP_IMGCOUNT = ru32;
        vkapp->SWAP_IMGS = malloc(ru32 * sizeof(VkImage));
        vkapp->SWAP_IMGVIEWS = malloc(ru32 * sizeof(VkImageView));
    }
    vkGetSwapchainImagesKHR(vkapp->DEV, vkapp->SWAP, &ru32, vkapp->SWAP_IMGS);

    memset(&vk_swapchain_imgs_views_createinfo, 0, sizeof(VkImageViewCreateInfo));
    vk_swapchain_imgs_views_createinfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    vk_swapchain_imgs_views_createinfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    vk_swapchain_imgs_views_createinfo.format = vkapp->SFCFMT.format;
    vk_swapchain_imgs_views_createinfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    vk_swapchain_imgs_views_createinfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    vk_swapchain_imgs_views_createinfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    vk_swapchain_imgs_views_createinfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    vk_swapchain_imgs_views_createinfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    vk_swapchain_imgs_views_createinfo.subresourceRange.levelCount = 1;
    vk_swapchain_imgs_views_createinfo.subresourceRange.layerCount = 1;

    memset(&vk_colorattach, 0, sizeof(VkAttachmentDescription));
    vk_colorattach.format = vkapp->SFCFMT.format;
    vk_colorattach.samples = VK_SAMPLE_COUNT_1_BIT;
    vk_colorattach.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    vk_colorattach.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    vk_colorattach.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    vk_colorattach.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    vk_colorattach.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    vk_colorattach.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    memset(&vk_colorattach_ref, 0, sizeof(VkAttachmentReference));
    vk_colorattach_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    vk_colorattach_ref.attachment = 0;

    memset(&vk_colorattach_subpass, 0, sizeof(VkSubpassDescription));
    vk_colorattach_subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    vk_colorattach_subpass.colorAttachmentCount = 1;
    vk_colorattach_subpass.pColorAttachments = &vk_colorattach_ref;

    memset(&vk_subpass_deps, 0, sizeof(VkSubpassDependency));
    vk_subpass_deps.srcSubpass = VK_SUBPASS_EXTERNAL;
    vk_subpass_deps.dstSubpass = 0;
    vk_subpass_deps.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    vk_subpass_deps.srcAccessMask = 0;
    vk_subpass_deps.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    vk_subpass_deps.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    memset(&vk_renderpass_createinfo, 0, sizeof(VkRenderPassCreateInfo));
    vk_renderpass_createinfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    vk_renderpass_createinfo.attachmentCount = 1;
    vk_renderpass_createinfo.pAttachments = &vk_colorattach;
    vk_renderpass_createinfo.subpassCount = 1;
    vk_renderpass_createinfo.pSubpasses = &vk_colorattach_subpass;
    vk_renderpass_createinfo.dependencyCount = 1;
    vk_renderpass_createinfo.pDependencies = &vk_subpass_deps;

    if(vkCreateRenderPass(vkapp->DEV, &vk_renderpass_createinfo, NULL, &vkapp->RPASS) != VK_SUCCESS)
    {
        perror("vkCreateRenderPass");
        return 0;
    }

    memset(&vk_fbuf_createinfo, 0, sizeof(VkFramebufferCreateInfo));
    vk_fbuf_createinfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    vk_fbuf_createinfo.renderPass = vkapp->RPASS;
    vk_fbuf_createinfo.attachmentCount = 1;
    vk_fbuf_createinfo.width = vkapp->SFCCAP.currentExtent.width;
    vk_fbuf_createinfo.height = vkapp->SFCCAP.currentExtent.height;
    vk_fbuf_createinfo.layers = 1;

    if(vkapp->FBUF == NULL)
        vkapp->FBUF = malloc(ru32 * sizeof(VkFramebuffer));

    for(i = 0; i < ru32; i++)
    {
        vk_swapchain_imgs_views_createinfo.image = vkapp->SWAP_IMGS[i];
        if(vkCreateImageView(vkapp->DEV, &vk_swapchain_imgs_views_createinfo, NULL, &vkapp->SWAP_IMGVIEWS[i]) != 
                VK_SUCCESS)
        {
            perror("vkCreateImageView");
            return 0;
        }

        vk_fbuf_createinfo.pAttachments = &vkapp->SWAP_IMGVIEWS[i];
        if(vkCreateFramebuffer(vkapp->DEV, &vk_fbuf_createinfo, NULL, &vkapp->FBUF[i]) != VK_SUCCESS)
        {
            perror("vkCreateFramebuffer");
            return 0;
        }
    }

    free(vk_sfc_formats);
    free(vk_sfc_presentmodes);

    return 1;
}

int vkapp_destroy(struct vkapp *vkapp)
{
    _cleanup_swapchain(vkapp);
    vkDestroyDevice(vkapp->DEV, NULL);
    vkDestroySurfaceKHR(vkapp->INSTANCE, vkapp->surface, NULL);
    if(vkapp->vkapp_DebugUtilsMessengerEXT)
        vkapp->vkapp_DestroyDebugUtilsMessengerEXT(vkapp->INSTANCE, vkapp->vkapp_DebugUtilsMessengerEXT, NULL);
    if(vkapp->vkapp_DebugReportCallbackEXT)
        vkapp->vkapp_DestroyDebugReportCallbackEXT(vkapp->INSTANCE, vkapp->vkapp_DebugReportCallbackEXT, NULL);
    vkDestroyInstance(vkapp->INSTANCE, NULL);

    free(vkapp->SWAP_IMGVIEWS);
    free(vkapp->SWAP_IMGS);
    free(vkapp->FBUF);
    return 1;
}

int hirestime(double* t)
{
    int ret = 0;
#ifdef _WIN32
    LARGE_INTEGER c, f;
#endif
#ifdef __linux__
    struct timespec tp;
    clock_gettime(CLOCK_MONOTONIC, &tp);
    *t = tp.tv_sec + (double)tp.tv_nsec / 1e9;
    ret = 1;
#elif defined (_WIN32)
    QueryPerformanceCounter(&c);
    QueryPerformanceFrequency(&f);
    *t = (double)c.QuadPart / f.QuadPart;
    ret = 1;
#endif
    return ret;
}
