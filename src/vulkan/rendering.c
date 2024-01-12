#include "rendering.h"

#include "util/constant.h"
#include "util/error.h"
#include "util/memory/memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void deleteVulkanAppRendering(const VulkanAppCore core, VulkanAppRendering renderer) {
    if (renderer == NULL) {
        return;
    }
    vkDeviceWaitIdle(core->device);
    if (renderer->square != NULL) deleteModel(core->device, renderer->square);
    if (renderer->uiPipeline != NULL) deletePipelineForUI(core->device, renderer->uiPipeline);
    if (renderer->uniBufferForUI != NULL) deleteBuffer(core->device, renderer->uniBufferForUI);
    if (renderer->descSetForUI != NULL) vkFreeDescriptorSets(core->device, renderer->descPool, 1, &renderer->descSetForUI);
    if (renderer->descPool != NULL) vkDestroyDescriptorPool(core->device, renderer->descPool, NULL);
    if (renderer->framebuffers != NULL) {
        for (uint32_t i = 0; i < renderer->framebuffersCount; ++i) {
            if (renderer->framebuffers[i] != NULL) vkDestroyFramebuffer(core->device, renderer->framebuffers[i], NULL);
        }
        free((void *)renderer->framebuffers);
    }
    if (renderer->renderPass != NULL) vkDestroyRenderPass(core->device, renderer->renderPass, NULL);
    free((void *)renderer);
}

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
#define ATTACHMENTS_COUNT 1

    const VulkanAppRendering renderer = (VulkanAppRendering)malloc(sizeof(struct VulkanAppRendering_t));
    CHECK(renderer != NULL, "VulkanAppRenderingの確保に失敗");
    memset(renderer, 0, sizeof(struct VulkanAppRendering_t));

    // レンダーパスを作成する
    //
    // NOTE: レンダーパスとは、描画工程のこと。
    //       現実世界でも、一枚の絵を完成するために何枚も絵を必要とすることはある。
    //       例えば、アニメの一コマも、一枚のキャンバスにすべてを描き込んでいくわけではない。
    //       一例としては、レイアウトを決め終わった後、
    //         - 人物を描く
    //           1. 原画用紙へ原画を描く
    //           2. 動画用紙へ原画トレスや中割りや裏塗りを描く
    //           3. 動画用紙を彩色する
    //         - 美術背景を描く
    //         - エフェクトを描く
    //       最後に撮影(合成)することで一コマが完成する。
    //       並列して行える作業もあれば、前工程に依存する作業もある。
    //       Vulkanにおけるこのようなものをレンダーパスと言う。
    {
        const VkAttachmentDescription attchDescs[ATTACHMENTS_COUNT] = {
            {
                0,
                RENDER_TARGET_PIXEL_FORMAT,
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
            ATTACHMENTS_COUNT,
            attchDescs,
            1,
            subpassDescs,
            0,
            NULL,
        };
        CHECK_VK(vkCreateRenderPass(core->device, &ci, NULL, &renderer->renderPass), "レンダーパスの作成に失敗");
    }

    // フレームバッファを作成する
    //
    // NOTE: 
    {
        renderer->framebuffersCount = imageViewsCount;
        renderer->framebuffers = (VkFramebuffer *)malloc(sizeof(VkFramebuffer) * renderer->framebuffersCount);
        for (uint32_t i = 0; i < renderer->framebuffersCount; ++i) {
            const VkImageView attachments[ATTACHMENTS_COUNT] = { imageViews[i] };
            const VkFramebufferCreateInfo ci = {
                VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                NULL,
                0,
                renderer->renderPass,
                ATTACHMENTS_COUNT,
                attachments,
                width,
                height,
                1,
            };
            CHECK_VK(vkCreateFramebuffer(core->device, &ci, NULL, &renderer->framebuffers[i]), "フレームバッファの作成に失敗");
        }
    }

    // ディスクリプタプールを作成する
    {
#define SIZES_COUNT 1
        const VkDescriptorPoolSize sizes[SIZES_COUNT] = {
            {
                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                1,
            },
        };
        const VkDescriptorPoolCreateInfo ci = {
            VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            NULL,
            VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
            1,
            SIZES_COUNT,
            sizes,
        };
        CHECK_VK(vkCreateDescriptorPool(core->device, &ci, NULL, &renderer->descPool), "ディスクリプタプールの作成に失敗");
#undef SIZES_COUNT
    }

    // パイプラインを作成する
    renderer->uiPipeline = createPipelineForUI(core->device, renderer->renderPass, width, height);
    CHECK(renderer->uiPipeline != NULL, "UI用のパイプラインの作成に失敗");

    // UI用シェーダのカメラのためのディスクリプタセットを確保する
    {
#define DESC_SETS_COUNT 1
        const VkDescriptorSetLayout descSetLayouts[DESC_SETS_COUNT] = { renderer->uiPipeline->descSetLayout };
        const VkDescriptorSetAllocateInfo ai = {
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            NULL,
            renderer->descPool,
            DESC_SETS_COUNT,
            descSetLayouts,
        };
        CHECK_VK(vkAllocateDescriptorSets(core->device, &ai, &renderer->descSetForUI), "UI用シェーダのカメラのためのディスクリプタセットの確保に失敗");
#undef DESC_SETS_COUNT
    }

    // UI用シェーダのカメラのためのユニフォームバッファを作成し設定する
    {
        renderer->uniBufferForUI = createBuffer(
            core->device,
            &core->physDevMemProps,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            sizeof(CameraForUI)
        );
        CHECK(renderer->uniBufferForUI != NULL, "UI用シェーダのカメラのためのユニフォームバッファの作成に失敗");

        const float source[16] = {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f,
        };
        CHECK(
            uploadToDeviceMemory(core->device, renderer->uniBufferForUI->devMemory, (const void *)source, sizeof(float) * 16),
            "UI用シェーダのカメラのためのユニフォームバッファへのデータのアップロードに失敗"
        );
    }

    // UI用シェーダのカメラのためのユニフォームバッファをディスクリプタセットにアップロードする
    {
        const VkDescriptorBufferInfo bi = {
            renderer->uniBufferForUI->buffer,
            0,
            VK_WHOLE_SIZE,
        };
        const VkWriteDescriptorSet wi[] = {
            {
                VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                NULL,
                renderer->descSetForUI,
                0,
                0,
                1,
                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                NULL,
                &bi,
                NULL,
            },
        };
        vkUpdateDescriptorSets(core->device, 1, wi, 0, NULL);
    }

    // モデルを作成する
    renderer->square = createModelFromFile(core->device, &core->physDevMemProps, "./model/square.raw");
    CHECK(renderer->square != NULL, "モデルの作成に失敗: ./model/square.raw");

    return renderer;

#undef ATTACHMENTS_COUNT
#undef CHECK
#undef CHECK_VK
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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
#define CHECK_VK(p, m) ERROR_IF_WITH((p) != VK_SUCCESS, "render()", (m), (p), {}, 0)
#define CHECK(p, m)    ERROR_IF     (!(p),              "render()", (m),      {}, 0)
#define COMMAND_BUFFERS_COUNT 1

    // コマンドバッファを確保し記録を開始する
    //
    // NOTE: 詳しくはallocateAndStartCommandBuffer()関数のコメントを参照。
    VkCommandBuffer cmdBuffer = allocateAndStartCommandBuffer(core);
    CHECK(cmdBuffer != NULL, "コマンドバッファの確保あるいは記録の開始に失敗");

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

    // TODO:
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer->uiPipeline->pipeline);

    // TODO:
    {
#define DESC_SET_INDEX 0
#define DYNAMIC_OFFSETS_COUNT 0
        vkCmdBindDescriptorSets(
            cmdBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            renderer->uiPipeline->pipelineLayout,
            0,
            1,
            &renderer->descSetForUI,
            DYNAMIC_OFFSETS_COUNT,
            NULL
        );

        const VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &renderer->square->vtxBuffer->buffer, &offset);
        vkCmdBindIndexBuffer(cmdBuffer, renderer->square->idxBuffer->buffer, offset, VK_INDEX_TYPE_UINT32);
        const PushConstantForUI pushConstant = {
            {1.0f, 1.0f, 1.0f, 1.0f},
            {0.0f, 0.0f, 0.0f, 1.0f},
            {0.0f, 0.0f, 1.0f, 1.0f},
        };
        vkCmdPushConstants(
            cmdBuffer,
            renderer->uiPipeline->pipelineLayout,
            VK_SHADER_STAGE_VERTEX_BIT,
            0,
            sizeof(PushConstantForUI),
            (const void *)&pushConstant
        );
        vkCmdDrawIndexed(cmdBuffer, renderer->square->indicesCount, 1, 0, 0, 0);
#undef DYNAMIC_OFFSETS_COUNT
#undef DESC_SET_INDEX
    }

    // レンダーパスを終了する
    vkCmdEndRenderPass(cmdBuffer);

    // コマンドバッファを終了しキューに提出する
    //
    // NOTE: 詳しくはendAndSubmitCommandBuffer()関数のコメントを参照。
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

    return 1;

#undef COMMAND_BUFFERS_COUNT
#undef CHECK
#undef CHECK_VK
}
