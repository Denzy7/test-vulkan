/* ultra basic vulkan program that just clears screen 
 * basis of a vulkan program
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include "vkapp.h"
#define WINDOW_VK
#include "window.h"

#ifdef _WIN32
#include <windows.h>
#endif



int main(int argc, char** argv)
{
    uint32_t i;
    double t;
    VkResult vkres;
    struct vkapp vkapp;
    struct window vkwindow;

    VkCommandPoolCreateInfo vk_cmdpool_createinfo;
    VkCommandPool vk_cmdpool;
    VkCommandBufferAllocateInfo vk_cmdbuf_allocinfo;
    VkCommandBuffer* vk_cmdbuf;
    VkCommandBufferBeginInfo vk_cmdbuf_begininfo;
    VkRenderPassBeginInfo vk_renderpass_begininfo;

    VkSemaphoreCreateInfo vk_semphr_createinfo;
    VkFenceCreateInfo vk_fen_creatinfo;
    VkFence *vk_fen_tosfc;
    VkSemaphore *vk_semphr_imgavail, *vk_semphr_rendered;
    VkSubmitInfo vk_submitinfo;

    VkClearValue vk_clear;
    uint32_t vk_buffered_frames = 3;
    VkPipelineStageFlags vk_pipeline_stageflags[] =
    {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };
    VkPresentInfoKHR vk_presentinfo;

    uint32_t vk_swapchain_imgidx;
    int vk_current_frame = 0;


    for(i = 0; i < argc; i++)
    {
        if(!strcmp(argv[i], "-uselayers"))
            vkapp.uselayers = 1;
    }

    memset(&vkwindow, 0, sizeof(struct window));
    memset(&vkapp, 0, sizeof(struct vkapp));

    vkwindow.width = 854;
    vkwindow.height = 480;
    vkwindow.title = "Vulkan Clear";

    if(!window_init() || !window_create(&vkwindow))
    {
        printf("cannot create window or vk surface");
        return 1;
    }
    if(!vkapp_init(&vkapp) || !window_vk(vkapp.INSTANCE, &vkapp.surface, &vkwindow) )
    {
        fprintf(stderr, "cannot init vkapp or surface\n");
        return 1;
    }

    if(!vkapp_dewice(&vkapp) || !vkapp_swapchain(&vkapp))
    {
        fprintf(stderr, "cannot init devicw or swapchain\n");
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

    memset(&vk_cmdbuf_allocinfo, 0, sizeof(VkCommandBufferAllocateInfo));
    vk_cmdbuf_allocinfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    vk_cmdbuf_allocinfo.commandPool = vk_cmdpool;
    vk_cmdbuf_allocinfo.commandBufferCount = vk_buffered_frames;
    vk_cmdbuf_allocinfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    vk_cmdbuf = malloc(vk_buffered_frames * sizeof(VkCommandBuffer));
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
    vk_submitinfo.pWaitDstStageMask = vk_pipeline_stageflags;
    vk_submitinfo.commandBufferCount = 1;
    vk_submitinfo.signalSemaphoreCount = 1;
    vk_submitinfo.waitSemaphoreCount = 1;

    memset(&vk_presentinfo, 0, sizeof(VkPresentInfoKHR));
    vk_presentinfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    vk_presentinfo.swapchainCount = 1;
    vk_presentinfo.pSwapchains = &vkapp.SWAP;
    vk_presentinfo.waitSemaphoreCount = 1;

    vkGetDeviceQueue(vkapp.DEV, vkapp.QGFX_ID, 0, &vkapp.QGFX);

    while(vkwindow.RUNNING)
    {
        hirestime(&t);
        vk_clear.color.float32[0] = fabs(sin(t));
        vk_clear.color.float32[1] = 1.0f - fabs(sin(t));
        vk_clear.color.float32[2] = fabs(sin(t)) / 2.0f;

        vkWaitForFences(vkapp.DEV, 1, &vk_fen_tosfc[vk_current_frame], VK_TRUE, UINT64_MAX);

        vkres = vkAcquireNextImageKHR(vkapp.DEV, vkapp.SWAP, UINT64_MAX, vk_semphr_imgavail[vk_current_frame], VK_NULL_HANDLE, &vk_swapchain_imgidx);


        vkResetFences(vkapp.DEV, 1, &vk_fen_tosfc[vk_current_frame]);

        vkResetCommandBuffer(vk_cmdbuf[vk_current_frame], 0);

        if(vkBeginCommandBuffer(vk_cmdbuf[vk_current_frame], &vk_cmdbuf_begininfo) != VK_SUCCESS)
        {
            perror("vkBeginCommandBuffer");
            return 1;
        }
        vk_renderpass_begininfo.framebuffer = vkapp.FBUF[vk_swapchain_imgidx];
        vkCmdBeginRenderPass(vk_cmdbuf[vk_current_frame], &vk_renderpass_begininfo, VK_SUBPASS_CONTENTS_INLINE);

        /* nothing since no graphics pipeline */

        vkCmdEndRenderPass(vk_cmdbuf[vk_current_frame]);
        if(vkEndCommandBuffer(vk_cmdbuf[vk_current_frame]) != VK_SUCCESS)
        {
            perror("vkEndCommandBUffer");
            return 1;
        }
        vk_submitinfo.pWaitSemaphores = &vk_semphr_imgavail[vk_current_frame];
        vk_submitinfo.pSignalSemaphores = &vk_semphr_rendered[vk_current_frame];
        vk_submitinfo.pCommandBuffers = &vk_cmdbuf[vk_current_frame];

        if(vkQueueSubmit(vkapp.QGFX, 1, &vk_submitinfo,  vk_fen_tosfc[vk_current_frame]) != VK_SUCCESS)
        {
            perror("vkQueueSubmit");
            return 1;
        }

        vk_presentinfo.pImageIndices = &vk_swapchain_imgidx;
        vk_presentinfo.pWaitSemaphores = &vk_semphr_rendered[vk_current_frame];
        vkQueuePresentKHR(vkapp.QGFX, &vk_presentinfo);
        vk_current_frame = (vk_current_frame + 1) % vk_buffered_frames;
        if(vkres == VK_SUBOPTIMAL_KHR){
            vkapp_swapchain(&vkapp);
            vk_renderpass_begininfo.renderPass = vkapp.RPASS;
            vk_renderpass_begininfo.renderArea.extent = vkapp.SFCCAP.currentExtent;
        }
        window_poll(&vkwindow);
    }
    vkDeviceWaitIdle(vkapp.DEV);
    for(i = 0; i < vk_buffered_frames; i++)
    {
        vkDestroySemaphore(vkapp.DEV, vk_semphr_rendered[i], NULL);
        vkDestroySemaphore(vkapp.DEV, vk_semphr_imgavail[i], NULL);
        vkDestroyFence(vkapp.DEV, vk_fen_tosfc[i], NULL);
    }
    vkDestroyCommandPool(vkapp.DEV, vk_cmdpool, NULL);
    vkapp_destroy(&vkapp);

    window_destroy(&vkwindow);
    window_terminate();


    free(vk_cmdbuf);
    free(vk_semphr_imgavail);
    free(vk_semphr_rendered);
    free(vk_fen_tosfc);
    return 0;
}
