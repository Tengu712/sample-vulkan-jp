#include "rendering.h"

#include "common.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan.h>

VulkanAppRendering createVulkanAppRendering(
    const VulkanAppCore core,
    const VkImageView *imageViews,
    uint32_t imageViewsCount,
    uint32_t width,
    uint32_t height,
    VkImageLayout imageLayout
) {
#define CHECK_VK(p, m) ERROR_IF_WITH((p) != VK_SUCCESS, "createVulkanAppRendering()", (m), (p), deleteVulkanAppRendering(core, renderer), NULL)
#define CHECK(p, m)    ERROR_IF     (!(p),              "createVulkanAppRendering()", (m),      deleteVulkanAppRendering(core, renderer), NULL)

    VulkanAppRendering renderer = (VulkanAppRendering)malloc(sizeof(struct VulkanAppRendering_t));
    CHECK(renderer != NULL, "VulkanAppRenderingの確保に失敗");

    // TODO: レンダーパスを作成する
    const uint32_t rndrPassAttchCount = 1;
    {
        const VkAttachmentDescription attchDescs[] = {
            {
                0,
                SURFACE_PIXEL_FORMAT,
                VK_SAMPLE_COUNT_1_BIT,
                VK_ATTACHMENT_LOAD_OP_CLEAR,
                VK_ATTACHMENT_STORE_OP_STORE,
                VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                VK_ATTACHMENT_STORE_OP_DONT_CARE,
                VK_IMAGE_LAYOUT_UNDEFINED,
                imageLayout,
            },
        };
        const VkAttachmentReference attchRefs[] = {
            {
                0,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            },
        };
        const VkSubpassDescription subpassDescs[] = {
            {
                0,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                0,
                NULL,
                1,
                attchRefs,
                NULL,
                NULL,
                0,
                NULL,
            },
        };
        const VkRenderPassCreateInfo ci = {
            VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            NULL,
            0,
            rndrPassAttchCount,
            attchDescs,
            1,
            subpassDescs,
            0,
            NULL,
        };
        CHECK_VK(vkCreateRenderPass(core->device, &ci, NULL, &renderer->renderPass), "レンダーパスの作成に失敗");
    }

    // TODO: フレームバッファを作成する
    {
        renderer->framebuffersCount = imageViewsCount;
        renderer->framebuffers = (VkFramebuffer *)malloc(sizeof(VkFramebuffer) * renderer->framebuffersCount);
        for (uint32_t i = 0; i < renderer->framebuffersCount; ++i) {
            const VkImageView attachments[] = { imageViews[i] };
            const VkFramebufferCreateInfo ci = {
                VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                NULL,
                0,
                renderer->renderPass,
                rndrPassAttchCount,
                attachments,
                width,
                height,
                1,
            };
            CHECK_VK(vkCreateFramebuffer(core->device, &ci, NULL, &renderer->framebuffers[i]), "フレームバッファの作成に失敗");
        }
    }

    return renderer;

#undef CHECK
#undef CHECK_VK
}

void deleteVulkanAppRendering(const VulkanAppCore core, VulkanAppRendering renderer) {
    vkDeviceWaitIdle(core->device);
    for (uint32_t i = 0; i < renderer->framebuffersCount; ++i) {
        vkDestroyFramebuffer(core->device, renderer->framebuffers[i], NULL);
    }
    vkDestroyRenderPass(core->device, renderer->renderPass, NULL);
}

int render(
    const VulkanAppCore core,
    const VulkanAppRendering renderer,
    uint32_t framebufferIndex,
    int32_t offsetX,
    int32_t offsetY,
    uint32_t width,
    uint32_t height,
    uint32_t waitSemaphoresCount,
    const VkSemaphore *waitSemaphores,
    const VkPipelineStageFlags *waitDstStageMasks,
    uint32_t signalSemaphoresCount,
    const VkSemaphore *signalSemaphores
) {
#define CHECK_VK(p, m) ERROR_IF_WITH((p) != VK_SUCCESS, "render()", (m), (p), "", 0)
#define CHECK(p, m)    ERROR_IF     (!(p),              "render()", (m),      "", 0)
#define COMMAND_BUFFERS_COUNT 1

    // コマンドバッファを確保し記録を開始する
    //
    // NOTE: 詳しくはallocateAndStartCommandBuffer()関数のコメントを参照。
    VkCommandBuffer cmdBuffer;
    {
        CHECK(allocateAndStartCommandBuffer(core, &cmdBuffer), "コマンドバッファの確保あるいは記録の開始に失敗");
    }

    // TODO: レンダーパスを開始する
    {
        const VkClearValue clearValues[] = {
            {{ 0.5f, 0.0f, 0.0f, 1.0f }},
        };
        const VkRenderPassBeginInfo bi = {
            VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            NULL,
            renderer->renderPass,
            renderer->framebuffers[framebufferIndex],
            { {offsetX, offsetY}, {width, height} },
            1,
            clearValues,
        };
        vkCmdBeginRenderPass(cmdBuffer, &bi, VK_SUBPASS_CONTENTS_INLINE);
    }

    // レンダーパスを終了する
    {
        vkCmdEndRenderPass(cmdBuffer);
    }

    // コマンドバッファを終了しキューに提出する
    //
    // NOTE: 詳しくはendAndSubmitCommandBuffer()関数のコメントを参照。
    {
        CHECK(
            endAndSubmitCommandBuffer(
                core,
                cmdBuffer,
                waitSemaphoresCount,
                waitSemaphores,
                waitDstStageMasks,
                signalSemaphoresCount,
                signalSemaphores
            ),
            "コマンドバッファの終了あるいは提出に失敗"
        );
    }

    return 1;

#undef COMMAND_BUFFERS_COUNT
#undef CHECK
#undef CHECK_VK
}
