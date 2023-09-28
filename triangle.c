#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <vulkan/vulkan_core.h>

#define WINDOW_VK
#include "window.h"
#include "spv.h"

PFN_vkCreateDebugUtilsMessengerEXT create_dbg_util_msgr = NULL;
PFN_vkDestroyDebugUtilsMessengerEXT destroy_dbg_util_msgr = NULL;

#define arysz(ary) sizeof(ary)/sizeof(ary[0])

struct needed_ext_or_layer
{
    const char* ext_or_layer;
    int has;
};

struct vertex
{
    float aPos[3];
    float aColour[3];
};

static const char* neededexts_dev_str[] =
{
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};


static const char* neededlayers_str[] =
{
    "VK_LAYER_KHRONOS_validation",
    /*"VK_LAYER_MANGOHUD_overlay_x86_64"*/
    /*"VK_LAYER_LUNARG_api_dump"*/
};


static struct needed_ext_or_layer neededexts_dev[arysz(neededexts_dev_str)];
static struct needed_ext_or_layer* neededexts_inst;
static struct needed_ext_or_layer neededlayers [arysz(neededlayers_str)];

static VKAPI_ATTR VkBool32 VKAPI_CALL debugcallback(VkDebugUtilsMessageSeverityFlagsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type, const VkDebugUtilsMessengerCallbackDataEXT* cbdata, void* userdata )
{
    if(severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        printf("\ndebugcb:> error: %s\n", cbdata->pMessage);
        exit(1);
    }
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

int vkmain(int argc, char** argv)
{
    /* common */
    uint32_t i, j;
    FILE* f;
    long f_sz;
    void* mem;
    uint32_t* mem_u32;
    struct spv spv;

    struct window vkwindow;
    int uselayers = 0;

    VkApplicationInfo vk_appinfo;
    VkInstanceCreateInfo vk_inst_createinfo;
    VkDebugUtilsMessengerCreateInfoEXT vk_dbgmsg_createinfo;
    VkDebugUtilsMessengerEXT vk_dbgmsg;
    uint32_t ext_count_inst, layer_count;
    uint32_t ext_needed_inst;
    const char** neededexts_inst_str; 
    VkExtensionProperties* vk_extprops_inst;
    VkLayerProperties* vk_layerprops;

    VkResult vkres;
    VkInstance vk_inst;

    uint32_t count_devs;
    VkPhysicalDevice* vk_phydevs;
    VkPhysicalDeviceProperties* vk_phydevs_props;
    VkPhysicalDeviceFeatures* vk_phydevs_feats;
    int usephydev = -1;

    uint32_t qcount;
    VkQueueFamilyProperties* vk_q_props;
    int q_gfx_idx = -1;
    VkQueue q_gfx;
    VkDeviceQueueCreateInfo q_createinfo;
    float q_gfx_priority = 1.0;
    VkBool32 q_present_support;

    VkDeviceCreateInfo vk_dev_createinfo;
    VkSurfaceKHR vk_sfc;
    uint32_t vk_sfc_format_count;
    VkSurfaceFormatKHR* vk_sfc_formats;
    int vk_sfc_ok = 0;
    VkSurfaceCapabilitiesKHR vk_sfc_caps;
    uint32_t vk_sfc_presentmode_count;
    int vk_sfc_presentmode_ok = -1;
    VkPresentModeKHR* vk_sfc_presentmodes;
    VkDevice vk_dev;
    uint32_t ext_count_dev;
    VkExtensionProperties* vk_extprops_dev;

    VkSwapchainCreateInfoKHR vk_swapchain_createinfo;
    VkSwapchainKHR vk_swapchain;
    VkImage* vk_swapchain_imgs;
    VkImageView* vk_swapchain_imgs_views;
    uint32_t vk_swapchain_imgs_count;
    VkImageViewCreateInfo vk_swapchain_imgs_views_createinfo;

    VkShaderModuleCreateInfo vk_shadermod_createinfo;
    VkShaderModule vk_shader_vert;
    VkShaderModule vk_shader_frag;
 VkPipelineShaderStageCreateInfo vk_pipeline_shader_createinfo[] =
    {
        {
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            NULL,
            0,
            VK_SHADER_STAGE_VERTEX_BIT,
            VK_NULL_HANDLE,
            "main",
            NULL
        },
        {
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            VK_NULL_HANDLE,
            0,
            VK_SHADER_STAGE_FRAGMENT_BIT,
            VK_NULL_HANDLE,
            "main",
            NULL
        },
    };
    struct vertex vertices[] = 
    {
        {{-0.5f, 0.5f, 0.0f},{1.0f, 0.0f, 0.0f}},
        {{0.0f, -0.5f, 0.0f},{0.0f, 1.0f, 0.0f}},
        {{0.5f, 0.5f, 0.0f},{0.0f, 0.0f, 1.0f}},
    };
    VkVertexInputBindingDescription vk_vertexbind_desc;
    VkVertexInputAttributeDescription vk_vertexattr_desc[2]; /* aPos and aColour */
    VkMemoryRequirements vk_memreq;
    VkMemoryAllocateInfo vk_memallocinfo;
    VkPipelineVertexInputStateCreateInfo vk_pipeline_vertexinput;
    VkPipelineInputAssemblyStateCreateInfo vk_pipeline_inputassembly;
    VkViewport vk_viewport;
    VkRect2D vk_scissor;
    VkPipelineViewportStateCreateInfo vk_pipeline_viewport_createinfo;
    VkPipelineRasterizationStateCreateInfo vk_pipeline_raster_createinfo;
    VkPipelineMultisampleStateCreateInfo vk_pipeline_msaa_createinfo;
    VkPipelineColorBlendAttachmentState vk_pipeline_colorblend_attachmentstate;
    VkPipelineColorBlendStateCreateInfo vk_pipeline_colorblend_createinfo;
    VkPipelineDynamicStateCreateInfo vk_pipeline_dynamicstate_createinfo;
    VkDynamicState vk_pipeline_dynamicstate[] =
    {
        VK_DYNAMIC_STATE_VIEWPORT
    };
    VkRenderPass vk_renderpass;
    VkRenderPassCreateInfo vk_renderpass_createinfo;
    VkPipelineLayout vk_pipeline_layout;
    VkPipelineLayoutCreateInfo vk_pipeline_layout_createinfo;
    VkAttachmentDescription vk_colorattach;
    VkAttachmentReference vk_colorattach_ref;
    VkSubpassDescription vk_colorattach_subpass;
    VkGraphicsPipelineCreateInfo vk_pipeline_createinfo;
    VkPipeline vk_pipeline;
    VkFramebufferCreateInfo vk_fbuf_createinfo;
    VkFramebuffer* vk_fbuf;
    VkCommandPoolCreateInfo vk_cmdpool_createinfo;
    VkCommandPool vk_cmdpool;
    VkBufferCreateInfo vk_vbuf_createinfo;
    VkBuffer vk_vbuf;
    VkDeviceMemory vk_vbuf_mem;
    VkDeviceSize vk_vbuf_off[] = {0};
    VkCommandBufferAllocateInfo vk_cmdbuf_allocinfo;
    VkCommandBuffer* vk_cmdbuf;
    VkCommandBufferBeginInfo vk_cmdbuf_begininfo;
    VkRenderPassBeginInfo vk_renderpass_begininfo;

    VkSemaphoreCreateInfo vk_semphr_createinfo;
    VkSemaphore* vk_semphr_imgavail;
    VkSemaphore* vk_semphr_rendered;
    VkFenceCreateInfo vk_fen_creatinfo;
    VkFence* vk_fen_tosfc;
    VkSubmitInfo vk_submitinfo;
    VkPipelineStageFlags vk_pipeline_stagewait[] = 
    {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };
    VkSubpassDependency vk_subpass_deps;

    uint32_t vk_swapchain_imgidx;
    VkClearValue vk_clear;
    VkPresentInfoKHR vk_presentinfo;
    int vk_buffered_frames = 3, vk_current_frame = 0;

    for(i = 0; i < argc; i++)
    {
        printf("v: %s\n", argv[i]);
        if(!strcmp(argv[i], "-uselayers"))
            uselayers = 1;
        if(!strcmp(argv[i], "-bufferframes")
                && argv[i + 1] != NULL)
            sscanf(argv[i + 1], "%d", &vk_buffered_frames);
    }

    if(!window_init())
        return 1;

    /* fill needed exts and layers */
    memset(neededexts_dev, 0, sizeof(neededexts_dev));
    for(i = 0; i < arysz(neededexts_dev); i++)
    {
        neededexts_dev[i].ext_or_layer = neededexts_dev_str[i];
    }
    neededexts_inst_str = window_extensions_get(&ext_needed_inst);
    neededexts_inst = malloc(ext_needed_inst * sizeof(struct needed_ext_or_layer));
    for(i = 0; i < ext_needed_inst; i++)
    {
        neededexts_inst[i].ext_or_layer = neededexts_inst_str[i];
    }
    memset(neededlayers, 0, sizeof(neededlayers));
    for(i = 0; i < arysz(neededlayers); i++)
    {
        neededlayers[i].ext_or_layer = neededlayers_str[i];
    }

    memset(&vk_appinfo, 0, sizeof(VkApplicationInfo));
    memset(&vk_inst_createinfo, 0, sizeof(VkInstanceCreateInfo));
    memset(&vk_dbgmsg_createinfo, 0, sizeof(VkDebugUtilsMessengerCreateInfoEXT));

    vk_dbgmsg_createinfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    vk_dbgmsg_createinfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    vk_dbgmsg_createinfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    vk_dbgmsg_createinfo.pfnUserCallback = debugcallback;
    
    vk_appinfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    vk_appinfo.pApplicationName = "Hello Vulkan";
    vk_appinfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
    vk_appinfo.pEngineName = "None";
    vk_appinfo.engineVersion = VK_MAKE_VERSION(0, 0, 1);
    vk_appinfo.apiVersion = VK_API_VERSION_1_0 ;
    
    vkEnumerateInstanceExtensionProperties(NULL, &ext_count_inst, NULL);
    if(ext_count_inst == 0)
    {
        perror("no instance extensions found. probably no vulkan in device!");
        return 1;
    }
    vk_extprops_inst = malloc(ext_count_inst * sizeof(VkExtensionProperties));
    vkEnumerateInstanceExtensionProperties(NULL, &ext_count_inst, vk_extprops_inst);
    printf("\nexts_inst [%u]:\n", ext_count_inst );
    for(i = 0; i < ext_count_inst; i++)
    {
        for(j = 0; j < ext_needed_inst; j++)
        {
            if(strcmp(neededexts_inst[j].ext_or_layer, vk_extprops_inst[i].extensionName) == 0)
                neededexts_inst[j].has = 1;
        }
        printf("%s\n", vk_extprops_inst[i].extensionName);
    }
    for(i = 0; i < ext_needed_inst; i++)
    {
        if(!neededexts_inst[i].has)
        {
            fprintf(stderr, "missing needed instance extension: %s\n", neededexts_inst[i].ext_or_layer);
            return 1;
        }
    }

    vkEnumerateInstanceLayerProperties(&layer_count, NULL);
    printf("\nlayers [%u]:\n", layer_count);
    if(layer_count > 0)
    {
        vk_layerprops = malloc(sizeof(VkLayerProperties) * layer_count);
        vkEnumerateInstanceLayerProperties(&layer_count, vk_layerprops);
        for(i = 0; i < layer_count; i++)
        {
            for(j = 0; j < arysz(neededlayers); j++)
            {

                if(strcmp(vk_layerprops[i].layerName, neededlayers[j].ext_or_layer) == 0)
                    neededlayers[j].has = 1;
            }

            printf("%s\n", vk_layerprops[i].layerName);
        }
    }

    if(uselayers)
    {
        for(i = 0; i < arysz(neededlayers); i++)
        {
            if(!neededlayers[i].has){
                printf("missing needed layer: %s\n", neededlayers[i].ext_or_layer);
                /* are unfound layers a must???? */
                return 1;
            }
        }
    }

    vk_inst_createinfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    vk_inst_createinfo.pApplicationInfo = &vk_appinfo;
    vk_inst_createinfo.enabledExtensionCount = ext_needed_inst;
    vk_inst_createinfo.ppEnabledExtensionNames = neededexts_inst_str;
    if(uselayers)
        vk_inst_createinfo.enabledLayerCount = arysz(neededlayers_str);
    vk_inst_createinfo.ppEnabledLayerNames = neededlayers_str;
    if(uselayers)
        vk_inst_createinfo.pNext = &vk_dbgmsg_createinfo;

    vkres = vkCreateInstance(&vk_inst_createinfo, NULL, &vk_inst);
    if(vkres != VK_SUCCESS)
    {
        perror("vkCreateInstance");
        return 1;
    }
    create_dbg_util_msgr = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vk_inst, "vkCreateDebugUtilsMessengerEXT");
    if(create_dbg_util_msgr == NULL)
    {
        perror("couldn't procaddr vkCreateDebugUtilsMessengerEXT");
        return 1;
    }
    destroy_dbg_util_msgr = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vk_inst, "vkDestroyDebugUtilsMessengerEXT");
    if(destroy_dbg_util_msgr == NULL)
    {
        perror("couldn't procaddr vkDestroyDebugUtilsMessengerEXT");
        return 1;
    }

    create_dbg_util_msgr(vk_inst, &vk_dbgmsg_createinfo, NULL, &vk_dbgmsg);
    memset(&vkwindow, 0, sizeof(struct window));
    vkwindow.width = 854;
    vkwindow.height = 480;
    if(window_create(&vkwindow) == 0 || window_vk(vk_inst, &vk_sfc, &vkwindow) == 0)
    {
        perror("cannot create window or vk");
        return 1;
    }

    vkEnumeratePhysicalDevices(vk_inst, &count_devs, NULL);
    if(count_devs == 0)
    {
        perror("vkEnumeratePhysicalDevices failed. device has no vulkan dewice");
        return 1;
    }
    vk_phydevs = malloc(count_devs * sizeof(VkPhysicalDevice));
    vk_phydevs_props = malloc(count_devs * sizeof(VkPhysicalDeviceProperties));
    vk_phydevs_feats = malloc(count_devs * sizeof(VkPhysicalDeviceFeatures));
    vkEnumeratePhysicalDevices(vk_inst, &count_devs, vk_phydevs);

    printf("\ndevs [%d]:\n", count_devs);
    for(i = 0; i < count_devs; i++)
    {
        vkGetPhysicalDeviceProperties(vk_phydevs[i], &vk_phydevs_props[i]);
        vkGetPhysicalDeviceFeatures(vk_phydevs[i], &vk_phydevs_feats[i]);
        printf("dev:%s ver:%u type:%d\n", vk_phydevs_props[i].deviceName, vk_phydevs_props[i].apiVersion, vk_phydevs_props[i].deviceType);
        if(vk_phydevs_props[i].deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            usephydev = i;
    }

    /*use igpu*/
    if(usephydev == -1)
        usephydev = 0;

    vkGetPhysicalDeviceQueueFamilyProperties(vk_phydevs[usephydev], &qcount, NULL);
    vk_q_props = malloc(sizeof(VkQueueFamilyProperties) * qcount);
    vkGetPhysicalDeviceQueueFamilyProperties(vk_phydevs[usephydev], &qcount, vk_q_props);
    for(i = 0; i < qcount; i++)
    {
        vkGetPhysicalDeviceSurfaceSupportKHR(vk_phydevs[usephydev], i, vk_sfc, &q_present_support);
        if(vk_q_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && q_present_support)
        {
            q_gfx_idx = i;
            break;
        }
    }
    if(q_gfx_idx == -1)
    {
        perror("no queue w/ gfx bit and present support");
        return 1;
    }

    vkEnumerateDeviceExtensionProperties(vk_phydevs[usephydev], NULL, &ext_count_dev, NULL);
    if(ext_count_dev == 0)
    {
        perror("dewice has no extensions");
        return 1;
    }
    vk_extprops_dev = malloc(sizeof(VkExtensionProperties) * ext_count_dev);
    vkEnumerateDeviceExtensionProperties(vk_phydevs[usephydev], NULL, &ext_count_dev, vk_extprops_dev);
    printf("\nexts_dev[%u]:\n", ext_count_dev);
    for(i = 0; i < ext_count_dev; i++)
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
            return 1;
        }
    }

    memset(&q_createinfo, 0, sizeof(VkDeviceQueueCreateInfo));
    q_createinfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    q_createinfo.queueFamilyIndex = q_gfx_idx;
    q_createinfo.queueCount = 1;
    q_createinfo.pQueuePriorities = &q_gfx_priority;

    memset(&vk_dev_createinfo, 0, sizeof(VkDeviceCreateInfo));
    vk_dev_createinfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    vk_dev_createinfo.pQueueCreateInfos = &q_createinfo;
    vk_dev_createinfo.queueCreateInfoCount = 1;
    vk_dev_createinfo.pEnabledFeatures = &vk_phydevs_feats[usephydev];
    vk_dev_createinfo.ppEnabledLayerNames = vk_inst_createinfo.ppEnabledLayerNames;
    vk_dev_createinfo.enabledExtensionCount = arysz(neededexts_dev_str);
    vk_dev_createinfo.ppEnabledExtensionNames = neededexts_dev_str; 
    vk_dev_createinfo.enabledLayerCount = vk_inst_createinfo.enabledLayerCount;
    
    vkres = vkCreateDevice(vk_phydevs[usephydev], &vk_dev_createinfo, NULL, &vk_dev);
    if(vkres != VK_SUCCESS)
    {
        perror("vkCreateDevice");
        return 1;
    }

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk_phydevs[usephydev], vk_sfc, &vk_sfc_caps);

    vkGetPhysicalDeviceSurfaceFormatsKHR(vk_phydevs[usephydev], vk_sfc, &vk_sfc_format_count, NULL);
    if(vk_sfc_format_count == 0)
    {
        perror("vkGetPhysicalDeviceSurfaceFormatsKHR. none found");
        return 1;
    }
    vk_sfc_formats = malloc(sizeof(VkSurfaceFormatKHR) * vk_sfc_format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(vk_phydevs[usephydev], vk_sfc, &vk_sfc_format_count, vk_sfc_formats);
    for(i = 0; i < vk_sfc_format_count; i++)
    {
        if(vk_sfc_formats[i].format == VK_FORMAT_B8G8R8_SRGB &&
                vk_sfc_formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            vk_sfc_ok = i;
            break;
        }
    }
    if(vk_sfc_ok == 0)
    {
        printf("no suitable(bgr8888 and linear) surface found, using 0\n");
    }

    vkGetPhysicalDeviceSurfacePresentModesKHR(vk_phydevs[usephydev], vk_sfc, &vk_sfc_presentmode_count, NULL);
    if(vk_sfc_presentmode_count == 0)
    {
        perror("vkGetPhysicalDeviceSurfacePresentModesKHR. none found");
        return 1;
    }
    vk_sfc_presentmodes = malloc(sizeof(VkPresentModeKHR) * vk_sfc_presentmode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(vk_phydevs[usephydev], vk_sfc, &vk_sfc_presentmode_count, vk_sfc_presentmodes);
    for(i = 0; i < vk_sfc_presentmode_count; i++)
    {
        if(vk_sfc_presentmodes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            vk_sfc_presentmode_ok = i;
            break;
        }
    }
    /*use vsync*/
    if(vk_sfc_presentmode_ok == -1)
        vk_sfc_presentmode_ok = VK_PRESENT_MODE_FIFO_KHR;

    memset(&vk_swapchain_createinfo, 0, sizeof(VkSwapchainCreateInfoKHR));
    vk_swapchain_createinfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    vk_swapchain_createinfo.surface = vk_sfc;
    vk_swapchain_createinfo.minImageCount = vk_sfc_caps.minImageCount + 1;
    vk_swapchain_createinfo.imageFormat = vk_sfc_formats[vk_sfc_ok].format;
    vk_swapchain_createinfo.imageColorSpace = vk_sfc_formats[vk_sfc_ok].colorSpace;
    vk_swapchain_createinfo.imageExtent = vk_sfc_caps.currentExtent;
    vk_swapchain_createinfo.imageArrayLayers = 1;
    vk_swapchain_createinfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    vk_swapchain_createinfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vk_swapchain_createinfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    for(i = 0; i < VK_COMPOSITE_ALPHA_FLAG_BITS_MAX_ENUM_KHR; i++)
    {
        if(vk_sfc_caps.supportedCompositeAlpha & (1 << i))
        {
            vk_swapchain_createinfo.compositeAlpha = (1 << i);
            break;
        }
    }
    if(vk_swapchain_createinfo.compositeAlpha == 0)
    {
        fprintf(stderr, "cannot find compatible composite alpha\n");
    }
    vk_swapchain_createinfo.presentMode = vk_sfc_presentmodes[vk_sfc_presentmode_ok];
    vk_swapchain_createinfo.clipped = VK_TRUE;
    vk_swapchain_createinfo.oldSwapchain = VK_NULL_HANDLE;

    if(vkCreateSwapchainKHR(vk_dev, &vk_swapchain_createinfo, NULL, &vk_swapchain) != VK_SUCCESS)
    {
        perror("vkCreateSwapchain");
        return 1;
    }

    vkGetSwapchainImagesKHR(vk_dev, vk_swapchain, &vk_swapchain_imgs_count, NULL);
    vk_swapchain_imgs = malloc(vk_swapchain_imgs_count * sizeof(VkImage));
    vkGetSwapchainImagesKHR(vk_dev, vk_swapchain, &vk_swapchain_imgs_count, vk_swapchain_imgs);
    vk_swapchain_imgs_views = malloc(vk_swapchain_imgs_count * sizeof(VkImageView));

    memset(&vk_swapchain_imgs_views_createinfo, 0, sizeof(VkImageViewCreateInfo));
    vk_swapchain_imgs_views_createinfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    vk_swapchain_imgs_views_createinfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    vk_swapchain_imgs_views_createinfo.format = vk_sfc_formats[vk_sfc_ok].format;
    vk_swapchain_imgs_views_createinfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    vk_swapchain_imgs_views_createinfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    vk_swapchain_imgs_views_createinfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    vk_swapchain_imgs_views_createinfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    vk_swapchain_imgs_views_createinfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    vk_swapchain_imgs_views_createinfo.subresourceRange.baseMipLevel = 0;
    vk_swapchain_imgs_views_createinfo.subresourceRange.levelCount = 1;
    vk_swapchain_imgs_views_createinfo.subresourceRange.baseArrayLayer = 0;
    vk_swapchain_imgs_views_createinfo.subresourceRange.layerCount = 1;

    for(i = 0; i < vk_swapchain_imgs_count; i++)
    {
        vk_swapchain_imgs_views_createinfo.image = vk_swapchain_imgs[i];
        if(vkCreateImageView(vk_dev, &vk_swapchain_imgs_views_createinfo, NULL, &vk_swapchain_imgs_views[i]) != 
                VK_SUCCESS)
        {
            perror("vkCreateImageView");
            return 1;
        }

    }

    memset(&vk_shadermod_createinfo, 0, sizeof(VkShaderModuleCreateInfo));
    memset(&spv, 0, sizeof(struct spv));

    vk_shadermod_createinfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

    spv_load("vert.spv", &spv);
    vk_shadermod_createinfo.codeSize = spv.sz;
    vk_shadermod_createinfo.pCode = spv.code;
    if(vkCreateShaderModule(vk_dev, &vk_shadermod_createinfo, NULL, &vk_shader_vert) != VK_SUCCESS)
    {
        perror("cant create vert shader");
        return 1;
    }
    spv_free(&spv);

    spv_load("frag.spv", &spv);
    vk_shadermod_createinfo.codeSize = spv.sz;
    vk_shadermod_createinfo.pCode = spv.code;
    if(vkCreateShaderModule(vk_dev, &vk_shadermod_createinfo, NULL, &vk_shader_frag) != VK_SUCCESS)
    {
        perror("cant create frag shader");
        return 1;
    }
    spv_free(&spv);
    
    vk_pipeline_shader_createinfo[0].module = vk_shader_vert;
    vk_pipeline_shader_createinfo[1].module = vk_shader_frag;

    memset(&vk_vertexbind_desc, 0, sizeof(VkVertexInputBindingDescription));
    vk_vertexbind_desc.binding = 0;
    vk_vertexbind_desc.stride = sizeof(struct vertex);
    vk_vertexbind_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    memset(&vk_vertexattr_desc, 0, sizeof(vk_vertexattr_desc)); 
    vk_vertexattr_desc[0].binding = 0;
    vk_vertexattr_desc[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    vk_vertexattr_desc[0].location = 0;
    vk_vertexattr_desc[0].offset = offsetof(struct vertex, aPos);

    vk_vertexattr_desc[1].binding = 0;
    vk_vertexattr_desc[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    vk_vertexattr_desc[1].location = 1;
    vk_vertexattr_desc[1].offset = offsetof(struct vertex, aColour);

    memset(&vk_pipeline_vertexinput, 0, sizeof(VkPipelineVertexInputStateCreateInfo));
    vk_pipeline_vertexinput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vk_pipeline_vertexinput.vertexBindingDescriptionCount = 1;
    vk_pipeline_vertexinput.pVertexBindingDescriptions = &vk_vertexbind_desc;
    vk_pipeline_vertexinput.vertexAttributeDescriptionCount = arysz(vk_vertexattr_desc);
    vk_pipeline_vertexinput.pVertexAttributeDescriptions = vk_vertexattr_desc;

    memset(&vk_pipeline_inputassembly, 0, sizeof(VkPipelineInputAssemblyStateCreateInfo));
    /* primrestart..=0 */
    vk_pipeline_inputassembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    vk_pipeline_inputassembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    memset(&vk_viewport, 0, sizeof(VkViewport));
    /* x, minDepth and y are 0 coz of memset */ 
    vk_viewport.width = vk_sfc_caps.currentExtent.width;
    vk_viewport.height = vk_sfc_caps.currentExtent.height;
    vk_viewport.maxDepth = 1.0f;

    memset(&vk_scissor, 0, sizeof(VkRect2D));
    /* offset = 0,0 coz yk */
    vk_scissor.extent = vk_sfc_caps.currentExtent;

    memset(&vk_pipeline_viewport_createinfo, 0, sizeof(VkPipelineViewportStateCreateInfo));
    vk_pipeline_viewport_createinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vk_pipeline_viewport_createinfo.viewportCount = 1;
    vk_pipeline_viewport_createinfo.pViewports = &vk_viewport;
    vk_pipeline_viewport_createinfo.scissorCount = 1;
    vk_pipeline_viewport_createinfo.pScissors = &vk_scissor;

    memset(&vk_pipeline_raster_createinfo, 0, sizeof(VkPipelineRasterizationStateCreateInfo));
    /* =0 > depthclamp, rasterdiscard, polygonmode, cullmode, 
     * depthstuff*/
    vk_pipeline_raster_createinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    vk_pipeline_raster_createinfo.lineWidth = 1.0f;

    memset(&vk_pipeline_msaa_createinfo, 0, sizeof(VkPipelineMultisampleStateCreateInfo));
    /* msaa disabled */
    vk_pipeline_msaa_createinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    vk_pipeline_msaa_createinfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    memset(&vk_pipeline_colorblend_attachmentstate, 0, sizeof(VkPipelineColorBlendAttachmentState));
    /* blending disabled by memset */
    vk_pipeline_colorblend_attachmentstate.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    memset(&vk_pipeline_colorblend_createinfo, 0, sizeof(VkPipelineColorBlendStateCreateInfo));
    vk_pipeline_colorblend_createinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    vk_pipeline_colorblend_createinfo.attachmentCount = 1;
    vk_pipeline_colorblend_createinfo.pAttachments = &vk_pipeline_colorblend_attachmentstate;

    memset(&vk_pipeline_dynamicstate_createinfo, 0, sizeof(VkPipelineDynamicStateCreateInfo));
    vk_pipeline_dynamicstate_createinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    vk_pipeline_dynamicstate_createinfo.dynamicStateCount = arysz(vk_pipeline_dynamicstate);
    vk_pipeline_dynamicstate_createinfo.pDynamicStates = vk_pipeline_dynamicstate;

    memset(&vk_pipeline_layout_createinfo, 0, sizeof(VkPipelineLayoutCreateInfo));
    vk_pipeline_layout_createinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    if(vkCreatePipelineLayout(vk_dev, &vk_pipeline_layout_createinfo, NULL, &vk_pipeline_layout) != VK_SUCCESS)
    {
        perror("vkCreatePipelineLayout");
        return 1;
    }
    
    memset(&vk_pipeline_layout_createinfo, 0, sizeof(VkPipelineLayoutCreateInfo));
    vk_pipeline_layout_createinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    memset(&vk_colorattach, 0, sizeof(VkAttachmentDescription));
    vk_colorattach.format = vk_sfc_formats[vk_sfc_ok].format;
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

    if(vkCreateRenderPass(vk_dev, &vk_renderpass_createinfo, NULL, &vk_renderpass) != VK_SUCCESS)
    {
        perror("vkCreateRenderPass");
        return 1;
    }

    memset(&vk_pipeline_createinfo, 0, sizeof(VkGraphicsPipelineCreateInfo));
    vk_pipeline_createinfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    /* depthstate, subpass and dynamic state = 0 */
    vk_pipeline_createinfo.stageCount = 2;
    vk_pipeline_createinfo.pStages = vk_pipeline_shader_createinfo;
    vk_pipeline_createinfo.pVertexInputState = &vk_pipeline_vertexinput;
    vk_pipeline_createinfo.pInputAssemblyState = &vk_pipeline_inputassembly;
    vk_pipeline_createinfo.pViewportState = &vk_pipeline_viewport_createinfo;
    vk_pipeline_createinfo.pRasterizationState = &vk_pipeline_raster_createinfo;
    vk_pipeline_createinfo.pMultisampleState = &vk_pipeline_msaa_createinfo;
    vk_pipeline_createinfo.pColorBlendState = &vk_pipeline_colorblend_createinfo;
    vk_pipeline_createinfo.layout = vk_pipeline_layout;
    vk_pipeline_createinfo.renderPass = vk_renderpass;
    vk_pipeline_createinfo.subpass = 0;
    vk_pipeline_createinfo.basePipelineIndex = -1;

    if(vkCreateGraphicsPipelines(vk_dev, VK_NULL_HANDLE, 1, &vk_pipeline_createinfo, NULL, &vk_pipeline) != VK_SUCCESS)
    {
        perror("vkCreateGraphicsPipelines");
        return 1;
    }

    vkDestroyShaderModule(vk_dev, vk_shader_vert, NULL);
    vkDestroyShaderModule(vk_dev, vk_shader_frag, NULL);

    memset(&vk_fbuf_createinfo, 0, sizeof(VkFramebufferCreateInfo));
    vk_fbuf_createinfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    vk_fbuf_createinfo.renderPass = vk_renderpass;
    vk_fbuf_createinfo.attachmentCount = 1;
    vk_fbuf_createinfo.width = vk_sfc_caps.currentExtent.width;
    vk_fbuf_createinfo.height = vk_sfc_caps.currentExtent.height;
    vk_fbuf_createinfo.layers = 1;
    vk_fbuf = malloc(vk_swapchain_imgs_count * sizeof(VkFramebuffer));
    for(i = 0; i < vk_swapchain_imgs_count; i++)
    {
        vk_fbuf_createinfo.pAttachments = &vk_swapchain_imgs_views[i];
        if(vkCreateFramebuffer(vk_dev, &vk_fbuf_createinfo, NULL, &vk_fbuf[i]) != VK_SUCCESS)
        {
            perror("vkCreateFramebuffer");
            return 1;
        }
    }


    memset(&vk_vbuf_createinfo, 0, sizeof(vk_vbuf_createinfo));
    vk_vbuf_createinfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vk_vbuf_createinfo.size = sizeof(vertices);
    vk_vbuf_createinfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    vk_vbuf_createinfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if(vkCreateBuffer(vk_dev, &vk_vbuf_createinfo, NULL, &vk_vbuf) != VK_SUCCESS)
    {
        perror("vkCreateBuffer");
        return 1;
    }

    vkGetBufferMemoryRequirements(vk_dev, vk_vbuf, &vk_memreq);
    memset(&vk_memallocinfo, 0, sizeof(vk_memallocinfo));
    vk_memallocinfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    vk_memallocinfo.allocationSize = vk_memreq.size;
    vk_memallocinfo.memoryTypeIndex = findmem(vk_phydevs[usephydev], vk_memreq.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    
    if(vkAllocateMemory(vk_dev, &vk_memallocinfo, NULL, &vk_vbuf_mem) != VK_SUCCESS)
    {
        perror("vkAllocateMemory");
        return 1;
    }

    vkBindBufferMemory(vk_dev, vk_vbuf, vk_vbuf_mem, 0);
    if(vkMapMemory(vk_dev, vk_vbuf_mem, 0, vk_vbuf_createinfo.size, 0, &mem) != VK_SUCCESS)
    {
        perror("vkMapMemory");
        return 1;
    }
    memcpy(mem, vertices, vk_vbuf_createinfo.size);
    vkUnmapMemory(vk_dev, vk_vbuf_mem);

    memset(&vk_cmdpool_createinfo, 0, sizeof(VkCommandPoolCreateInfo));
    vk_cmdpool_createinfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    vk_cmdpool_createinfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    vk_cmdpool_createinfo.queueFamilyIndex = q_gfx_idx;

    if(vkCreateCommandPool(vk_dev, &vk_cmdpool_createinfo, NULL, &vk_cmdpool) != VK_SUCCESS)
    {
        perror("vkCreateCommandPool");
        return 1;
    }   
    vk_cmdbuf = malloc(sizeof(VkCommandBuffer) * vk_buffered_frames);

    memset(&vk_cmdbuf_allocinfo, 0, sizeof(VkCommandBufferAllocateInfo));
    vk_cmdbuf_allocinfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    vk_cmdbuf_allocinfo.commandPool = vk_cmdpool;
    vk_cmdbuf_allocinfo.commandBufferCount = vk_buffered_frames;
    vk_cmdbuf_allocinfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    if(vkAllocateCommandBuffers(vk_dev, &vk_cmdbuf_allocinfo, vk_cmdbuf) != VK_SUCCESS)
    {
        perror("vkAllocateCommandBuffers");
        return 1;
    }

    memset(&vk_semphr_createinfo, 0, sizeof(VkSemaphoreCreateInfo));
    vk_semphr_createinfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    memset(&vk_fen_creatinfo, 0, sizeof(VkFenceCreateInfo));
    vk_fen_creatinfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    vk_fen_creatinfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    vk_semphr_imgavail = malloc(sizeof(VkSemaphore) * vk_buffered_frames);
    vk_semphr_rendered = malloc(sizeof(VkSemaphore) * vk_buffered_frames);
    vk_fen_tosfc = malloc(sizeof(VkFence) * vk_buffered_frames);

    for(i = 0; i < vk_buffered_frames; i++)
    {

        if(vkCreateSemaphore(vk_dev, &vk_semphr_createinfo, NULL, &vk_semphr_imgavail[i]) || vkCreateSemaphore(vk_dev, &vk_semphr_createinfo, NULL, &vk_semphr_rendered[i]) || vkCreateFence(vk_dev, &vk_fen_creatinfo, NULL, &vk_fen_tosfc[i]) != VK_SUCCESS)
        {
            perror("failed to create sync objs");
            return 1;
        }
    }

    memset(&vk_cmdbuf_begininfo, 0, sizeof(VkCommandBufferBeginInfo));
    vk_cmdbuf_begininfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    memset(&vk_renderpass_begininfo, 0, sizeof(VkRenderPassBeginInfo));
    memset(&vk_clear, 0, sizeof(VkClearValue));

    vk_clear.color.float32[0] = 1.0f;
    vk_clear.color.float32[1] = 0.5f;
    vk_clear.color.float32[2] = 0.3f;
    vk_clear.color.float32[3] = 1.0f;

    /* offset is 0 */
    vk_renderpass_begininfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    vk_renderpass_begininfo.renderPass = vk_renderpass;
    vk_renderpass_begininfo.renderArea.extent = vk_sfc_caps.currentExtent;
    vk_renderpass_begininfo.clearValueCount = 1;
    vk_renderpass_begininfo.pClearValues = &vk_clear;
    
    memset(&vk_submitinfo, 0, sizeof(VkSubmitInfo));
    vk_submitinfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    vk_submitinfo.waitSemaphoreCount = 1;
    vk_submitinfo.pWaitDstStageMask = vk_pipeline_stagewait;
    vk_submitinfo.commandBufferCount = 1;
    vk_submitinfo.signalSemaphoreCount = 1;

    memset(&vk_presentinfo, 0, sizeof(VkPresentInfoKHR));
    vk_presentinfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    vk_presentinfo.waitSemaphoreCount = 1;
    vk_presentinfo.swapchainCount = 1;
    vk_presentinfo.pSwapchains = &vk_swapchain;
        
    vkGetDeviceQueue(vk_dev, q_gfx_idx, 0, &q_gfx);

    while(vkwindow.RUNNING)
    {
        window_poll(&vkwindow);

        /*draw time ! */
        vkWaitForFences(vk_dev, 1, &vk_fen_tosfc[vk_current_frame], VK_TRUE, UINT64_MAX);

        vkResetFences(vk_dev, 1, &vk_fen_tosfc[vk_current_frame]);

        vkAcquireNextImageKHR(vk_dev, vk_swapchain, UINT64_MAX, vk_semphr_imgavail[vk_current_frame], VK_NULL_HANDLE, &vk_swapchain_imgidx);

        vkResetCommandBuffer(vk_cmdbuf[vk_current_frame], 0);

        if(vkBeginCommandBuffer(vk_cmdbuf[vk_current_frame], &vk_cmdbuf_begininfo) != VK_SUCCESS)
        {
            perror("vkBeginCommandBuffer");
            return 1;
        }
        vk_renderpass_begininfo.framebuffer = vk_fbuf[vk_swapchain_imgidx];
        vkCmdBeginRenderPass(vk_cmdbuf[vk_current_frame], &vk_renderpass_begininfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(vk_cmdbuf[vk_current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, vk_pipeline);
        vkCmdBindVertexBuffers(vk_cmdbuf[vk_current_frame], 0, 1, &vk_vbuf, vk_vbuf_off);
        vkCmdDraw(vk_cmdbuf[vk_current_frame], arysz(vertices), 1, 0, 0);
        vkCmdEndRenderPass(vk_cmdbuf[vk_current_frame]);

        if(vkEndCommandBuffer(vk_cmdbuf[vk_current_frame]) != VK_SUCCESS)
        {
            perror("vkEndCommandBUffer");
            return 1;
        }

        vk_submitinfo.pWaitSemaphores = &vk_semphr_imgavail[vk_current_frame];
        vk_submitinfo.pSignalSemaphores = &vk_semphr_rendered[vk_current_frame];
        vk_submitinfo.pCommandBuffers = &vk_cmdbuf[vk_current_frame];
        if(vkQueueSubmit(q_gfx, 1, &vk_submitinfo, vk_fen_tosfc[vk_current_frame]) != VK_SUCCESS)
        {
            perror("vkQueueSubmit");
            return 1;
        }

         vk_presentinfo.pImageIndices = &vk_swapchain_imgidx;
         vk_presentinfo.pWaitSemaphores = &vk_semphr_rendered[vk_current_frame];
         vkQueuePresentKHR(q_gfx, &vk_presentinfo);
         vk_current_frame = (vk_current_frame + 1) % vk_buffered_frames;
    }

    vkDeviceWaitIdle(vk_dev);

    vkDestroyBuffer(vk_dev, vk_vbuf, NULL);
    vkFreeMemory(vk_dev, vk_vbuf_mem, NULL);
    for(i = 0; i < vk_buffered_frames; i++)
    {
        vkDestroySemaphore(vk_dev, vk_semphr_rendered[i], NULL);
        vkDestroySemaphore(vk_dev, vk_semphr_imgavail[i], NULL);
        vkDestroyFence(vk_dev, vk_fen_tosfc[i], NULL);
    }

        for(i = 0; i < vk_swapchain_imgs_count; i++)
    {
        vkDestroyImageView(vk_dev, vk_swapchain_imgs_views[i], NULL);
    }
    for(i = 0; i < vk_swapchain_imgs_count; i++)
    {
        vkDestroyFramebuffer(vk_dev, vk_fbuf[i], NULL);
    }
    vkDestroyCommandPool(vk_dev, vk_cmdpool, NULL);
    vkDestroyPipeline(vk_dev, vk_pipeline, NULL);
    vkDestroyPipelineLayout(vk_dev, vk_pipeline_layout, NULL);
    vkDestroyRenderPass(vk_dev, vk_renderpass, NULL);
    vkDestroySwapchainKHR(vk_dev, vk_swapchain, NULL);
    vkDestroyDevice(vk_dev, NULL);
    vkDestroySurfaceKHR(vk_inst, vk_sfc, NULL);
    destroy_dbg_util_msgr(vk_inst, vk_dbgmsg, NULL);
    vkDestroyInstance(vk_inst, NULL);

    window_destroy(&vkwindow);
    window_terminate();

    free(neededexts_inst);
    free(vk_extprops_inst);
    if(layer_count > 0){
        free(vk_layerprops);
    }
    free(vk_phydevs);
    free(vk_phydevs_props);
    free(vk_phydevs_feats);
    free(vk_q_props);
    free(vk_extprops_dev);
    free(vk_sfc_formats);
    free(vk_sfc_presentmodes);
    free(vk_swapchain_imgs);
    free(vk_swapchain_imgs_views);
    free(vk_fbuf);
    free(vk_cmdbuf);
    free(vk_semphr_imgavail);
    free(vk_semphr_rendered);
    free(vk_fen_tosfc);

    return 0;
}

int main(int argc, char** argv)
{
    vkmain(argc, argv);
}
