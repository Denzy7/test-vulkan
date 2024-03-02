/* create a graphics pipeline object and render 
 * triangle 
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>

#define WINDOW_VK
#include "window.h"
#include "vkapp.h"
#include "spv.h"


struct vertex
{
    float aPos[3];
    float aColour[3];
};

int vkmain(int argc, char** argv)
{
    /* common */
    int32_t i;
    void* mem;

    struct window vkwindow;
    struct vkapp vkapp;
    struct spv spv;
    
    VkResult vkres;

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
    VkPipelineVertexInputStateCreateInfo vk_pipeline_vertexinput;
    VkPipelineInputAssemblyStateCreateInfo vk_pipeline_inputassembly;
    VkViewport vk_viewport;
    VkRect2D vk_scissor;
    VkPipelineViewportStateCreateInfo vk_pipeline_viewport_createinfo;
    VkPipelineRasterizationStateCreateInfo vk_pipeline_raster_createinfo;
    VkPipelineMultisampleStateCreateInfo vk_pipeline_msaa_createinfo;
    VkPipelineColorBlendAttachmentState vk_pipeline_colorblend_attachmentstate;
    VkPipelineColorBlendStateCreateInfo vk_pipeline_colorblend_createinfo;
    VkPipelineLayout vk_pipeline_layout;
    VkPipelineLayoutCreateInfo vk_pipeline_layout_createinfo;
    VkGraphicsPipelineCreateInfo vk_pipeline_createinfo;
    VkPipeline vk_pipeline;

    VkCommandPoolCreateInfo vk_cmdpool_createinfo;
    VkCommandPool vk_cmdpool;

    VkBuffer vk_buf_staging;
    VkDeviceMemory vk_buf_staging_mem;
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

    uint32_t vk_swapchain_imgidx;
    VkClearValue vk_clear;
    VkPresentInfoKHR vk_presentinfo;
    int vk_buffered_frames = 3, vk_current_frame = 0;

    memset(&vkwindow, 0, sizeof(struct window));
    memset(&vkapp, 0, sizeof(struct vkapp));

    for(i = 0; i < argc; i++)
    {
        printf("v: %s\n", argv[i]);
        if(!strcmp(argv[i], "-uselayers"))
            vkapp.uselayers = 1;
        if(!strcmp(argv[i], "-bufferframes")
                && argv[i + 1] != NULL)
            sscanf(argv[i + 1], "%d", &vkapp.bufferedframes);
    }

    vkwindow.title = "Vulkan Triangle";
    vkwindow.width = 854;
    vkwindow.height = 480;

    if(!window_init() || !window_create(&vkwindow))
        return 1;

    if(!vkapp_init(&vkapp) || !window_vk(vkapp.INSTANCE, &vkapp.surface, &vkwindow))
    {
        perror("cannot init vkapp or vkwindow");
        return 1;
    }

    if(!vkapp_dewice(&vkapp) || !vkapp_swapchain(&vkapp))
    {
        fprintf(stderr, "cannot init device or swapchain\n");
        return 1;
    }

    memset(&vk_shadermod_createinfo, 0, sizeof(VkShaderModuleCreateInfo));
    memset(&spv, 0, sizeof(struct spv));

    vk_shadermod_createinfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

    if(!spv_load("vert.spv", &spv))
    {
        perror("cannot load vert.spv, check its next to the executable if not android");
        return 1;
    }
    vk_shadermod_createinfo.codeSize = spv.sz;
    vk_shadermod_createinfo.pCode = spv.code;
    if(vkCreateShaderModule(vkapp.DEV, &vk_shadermod_createinfo, NULL, &vk_shader_vert) != VK_SUCCESS)
    {
        perror("cant create vert shader");
        return 1;
    }
    spv_free(&spv);

    if(!spv_load("frag.spv", &spv))
    {
        perror("cannot load frag.spv, check its next to the executable if not android");
        return 1;
    }
    vk_shadermod_createinfo.codeSize = spv.sz;
    vk_shadermod_createinfo.pCode = spv.code;
    if(vkCreateShaderModule(vkapp.DEV, &vk_shadermod_createinfo, NULL, &vk_shader_frag) != VK_SUCCESS)
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
    vk_viewport.width = vkapp.SFCCAP.currentExtent.width;
    vk_viewport.height = vkapp.SFCCAP.currentExtent.height;
    vk_viewport.maxDepth = 1.0f;

    memset(&vk_scissor, 0, sizeof(VkRect2D));
    /* offset = 0,0 coz yk */
    vk_scissor.extent = vkapp.SFCCAP.currentExtent;

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

    memset(&vk_pipeline_layout_createinfo, 0, sizeof(VkPipelineLayoutCreateInfo));
    vk_pipeline_layout_createinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    if(vkCreatePipelineLayout(vkapp.DEV, &vk_pipeline_layout_createinfo, NULL, &vk_pipeline_layout) != VK_SUCCESS)
    {
        perror("vkCreatePipelineLayout");
        return 1;
    }
    
    memset(&vk_pipeline_layout_createinfo, 0, sizeof(VkPipelineLayoutCreateInfo));
    vk_pipeline_layout_createinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

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
    vk_pipeline_createinfo.renderPass = vkapp.RPASS;
    vk_pipeline_createinfo.subpass = 0;
    vk_pipeline_createinfo.basePipelineIndex = -1;

    if(vkCreateGraphicsPipelines(vkapp.DEV, VK_NULL_HANDLE, 1, &vk_pipeline_createinfo, NULL, &vk_pipeline) != VK_SUCCESS)
    {
        perror("vkCreateGraphicsPipelines");
        return 1;
    }

    if(!mkbuffer(vkapp.PHYDEV, vkapp.DEV, sizeof(vertices), 
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                &vk_buf_staging, &vk_buf_staging_mem))
    {
        return 1;
    }

    if(vkMapMemory(vkapp.DEV, vk_buf_staging_mem, 0, sizeof(vertices), 0, &mem) != VK_SUCCESS)
    {
        perror("vkMapMemory");
        return 1;
    }
    memcpy(mem, vertices, sizeof(vertices));
    vkUnmapMemory(vkapp.DEV, vk_buf_staging_mem);

    if(!mkbuffer(vkapp.PHYDEV, vkapp.DEV, sizeof(vertices), 
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                &vk_vbuf, &vk_vbuf_mem))
    {
        return 1;
    }

    memset(&vk_cmdpool_createinfo, 0, sizeof(VkCommandPoolCreateInfo));
    vk_cmdpool_createinfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    vk_cmdpool_createinfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    vk_cmdpool_createinfo.queueFamilyIndex = vkapp.QGFX_ID;

    if(vkCreateCommandPool(vkapp.DEV, &vk_cmdpool_createinfo, NULL, &vk_cmdpool) != VK_SUCCESS)
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
    if(vkAllocateCommandBuffers(vkapp.DEV, &vk_cmdbuf_allocinfo, vk_cmdbuf) != VK_SUCCESS)
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

        if(vkCreateSemaphore(vkapp.DEV, &vk_semphr_createinfo, NULL, &vk_semphr_imgavail[i]) || vkCreateSemaphore(vkapp.DEV, &vk_semphr_createinfo, NULL, &vk_semphr_rendered[i]) || vkCreateFence(vkapp.DEV, &vk_fen_creatinfo, NULL, &vk_fen_tosfc[i]) != VK_SUCCESS)
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
    vk_renderpass_begininfo.renderPass = vkapp.RPASS;
    vk_renderpass_begininfo.renderArea.extent = vkapp.SFCCAP.currentExtent;
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
    vk_presentinfo.pSwapchains = &vkapp.SWAP;
        
    vkGetDeviceQueue(vkapp.DEV, vkapp.QGFX_ID, 0, &vkapp.QGFX);

    if(bufcpy(vkapp.DEV, vk_cmdpool, vkapp.QGFX, vk_buf_staging, vk_vbuf, sizeof(vertices)) == 0)
    {
        perror("bufcpy failed");
        return 1;
    }

    while(vkwindow.RUNNING)
    {
        /*draw time ! */
        vkWaitForFences(vkapp.DEV, 1, &vk_fen_tosfc[vk_current_frame], VK_TRUE, UINT64_MAX);

        vkResetFences(vkapp.DEV, 1, &vk_fen_tosfc[vk_current_frame]);

        vkres = vkAcquireNextImageKHR(vkapp.DEV, vkapp.SWAP, UINT64_MAX, vk_semphr_imgavail[vk_current_frame], VK_NULL_HANDLE, &vk_swapchain_imgidx);


        vkResetCommandBuffer(vk_cmdbuf[vk_current_frame], 0);

        if(vkBeginCommandBuffer(vk_cmdbuf[vk_current_frame], &vk_cmdbuf_begininfo) != VK_SUCCESS)
        {
            perror("vkBeginCommandBuffer");
            return 1;
        }
        vk_renderpass_begininfo.framebuffer = vkapp.FBUF[vk_swapchain_imgidx];
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
        if(vkQueueSubmit(vkapp.QGFX, 1, &vk_submitinfo, vk_fen_tosfc[vk_current_frame]) != VK_SUCCESS)
        {
            perror("vkQueueSubmit");
            return 1;
        }

         vk_presentinfo.pImageIndices = &vk_swapchain_imgidx;
         vk_presentinfo.pWaitSemaphores = &vk_semphr_rendered[vk_current_frame];
         vkQueuePresentKHR(vkapp.QGFX, &vk_presentinfo);
         vk_current_frame = (vk_current_frame + 1) % vk_buffered_frames;

        if(vkres == VK_SUBOPTIMAL_KHR)
        {
            vkapp_swapchain(&vkapp);
            vk_renderpass_begininfo.renderArea.extent = vkapp.SFCCAP.currentExtent;
            vk_renderpass_begininfo.renderPass = vkapp.RPASS;
            vk_viewport.width = vkapp.SFCCAP.currentExtent.width;
            vk_viewport.height = vkapp.SFCCAP.currentExtent.height;
            vk_scissor.extent = vkapp.SFCCAP.currentExtent;
            vk_pipeline_createinfo.renderPass = vkapp.RPASS;
            vkDestroyPipeline(vkapp.DEV, vk_pipeline, NULL);
            if(vkCreateGraphicsPipelines(vkapp.DEV, VK_NULL_HANDLE, 1, &vk_pipeline_createinfo, NULL, &vk_pipeline) != VK_SUCCESS)
            {
                perror("vkCreateGraphicsPipelines");
                return 1;
            }

        }
        window_poll(&vkwindow);
    }

    vkDeviceWaitIdle(vkapp.DEV);

    vkDestroyBuffer(vkapp.DEV, vk_buf_staging, NULL);
    vkFreeMemory(vkapp.DEV, vk_buf_staging_mem, NULL);
    vkDestroyBuffer(vkapp.DEV, vk_vbuf, NULL);
    vkFreeMemory(vkapp.DEV, vk_vbuf_mem, NULL);
    for(i = 0; i < vk_buffered_frames; i++)
    {
        vkDestroySemaphore(vkapp.DEV, vk_semphr_rendered[i], NULL);
        vkDestroySemaphore(vkapp.DEV, vk_semphr_imgavail[i], NULL);
        vkDestroyFence(vkapp.DEV, vk_fen_tosfc[i], NULL);
    }
    vkDestroyCommandPool(vkapp.DEV, vk_cmdpool, NULL);
    vkDestroyShaderModule(vkapp.DEV, vk_shader_vert, NULL);
    vkDestroyShaderModule(vkapp.DEV, vk_shader_frag, NULL);

    vkDestroyPipeline(vkapp.DEV, vk_pipeline, NULL);
    vkDestroyPipelineLayout(vkapp.DEV, vk_pipeline_layout, NULL);
    vkapp_destroy(&vkapp);

    window_destroy(&vkwindow);
    window_terminate();
    free(vk_cmdbuf);
    free(vk_semphr_imgavail);
    free(vk_semphr_rendered);
    free(vk_fen_tosfc);

    return 0;
}

int main(int argc, char** argv)
{
    vkmain(argc, argv);
    return 0;
}
