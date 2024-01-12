#include "offscreen.h"

// stb_image.hをヘッダーファイルのみで利用するためにはこのマクロを定義する
#define STB_IMAGE_IMPLEMENTATION
// stb_image_write.hをヘッダーファイルのみで利用するためにはこのマクロを定義する
#define STB_IMAGE_WRITE_IMPLEMENTATION
// stb_image_write.h内のsprintf()関数の使用に対してwarningを出さないためにこのマクロを定義する
#define _CRT_SECURE_NO_WARNINGS

#include "../../vulkan/core.h"
#include "../../vulkan/rendering.h"
#include "../../vulkan/util/constant.h"
#include "../../vulkan/util/error.h"
#include "../../vulkan/util/memory/buffer.h"
#include "../../vulkan/util/memory/image.h"
#include "stb_image.h"
#include "stb_image_write.h"

#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Vulkanアプリケーションのうちオフスクリーンでのオブジェクトを持つ構造体
typedef struct VulkanAppOffscreen_t {
    Image image;
    VkImageView imageView;
} *VulkanAppOffscreen;

void deleteVulkanAppOffscreen(const VulkanAppCore core, VulkanAppOffscreen offscreen) {
    if (offscreen == NULL) {
        return;
    }
    vkDeviceWaitIdle(core->device);
    if (offscreen->imageView != NULL) vkDestroyImageView(core->device, offscreen->imageView, NULL);
    if (offscreen->image != NULL) deleteImage(core->device, offscreen->image);
    free((void *)offscreen);
}

VulkanAppOffscreen createVulkanAppOffscreen(const VulkanAppCore core, int width, int height) {
#define CHECK_VK(p, m) ERROR_IF_WITH((p) != VK_SUCCESS, "createVulkanAppOffscreen()", (m), (p), deleteVulkanAppOffscreen(core, offscreen), NULL)
#define CHECK(p, m)    ERROR_IF     (!(p),              "createVulkanAppOffscreen()", (m),      deleteVulkanAppOffscreen(core, offscreen), NULL)

    const VulkanAppOffscreen offscreen = (VulkanAppOffscreen)malloc(sizeof(struct VulkanAppOffscreen_t));
    CHECK(offscreen != NULL, "VulkanAppOffscreenの確保に失敗");

    // 描画先イメージを作成する
    //
    // NOTE: この描画先イメージは後々画像ファイルに保存するために一時バッファへコピーを行う。
    //       そのため、コピー元としても使えるようUsageにVK_IMAGE_USAGE_TRANSFER_SRC_BITも指定する。
    //       ただし、描画先イメージはデバイスローカルメモリになければならないため、Memory PropertyにVK_MEMORY_PROPERTY_DEVICE_LOCAL_BITを指定する。
    {
        const VkExtent3D extent = { (uint32_t)width, (uint32_t)height, 1 };
        offscreen->image = createImage(
            core->device,
            &core->physDevMemProps,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            RENDER_TARGET_PIXEL_FORMAT,
            &extent
        );
        CHECK(offscreen->image != NULL, "描画先イメージの作成に失敗");
    }

    // 描画先イメージのイメージビューを作成
    {
        const VkImageViewCreateInfo ci = {
            VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            NULL,
            0,
            offscreen->image->image,
            VK_IMAGE_VIEW_TYPE_2D,
            RENDER_TARGET_PIXEL_FORMAT,
            { 0 },
            { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
        };
        CHECK_VK(vkCreateImageView(core->device, &ci, NULL, &offscreen->imageView), "描画先イメージビューの作成に失敗");
    }

    return offscreen;

#undef CHECK
#undef CHECK_VK
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// saveRenderingResult()関数の中で一時的に作成されるオブジェクトを持つ構造体
typedef struct TempObjsSaveRenderingResult_t {
    Buffer buffer;
    int mapped;
    uint8_t *pixels;
} TempObjsSaveRenderingResult;

void deleteTempObjsSaveRenderingResult(const VulkanAppCore core, TempObjsSaveRenderingResult *temp) {
    if (temp == NULL) {
        return;
    }
    vkDeviceWaitIdle(core->device);
    if (temp->pixels != NULL) free((void *)temp->pixels);
    if (temp->mapped) vkUnmapMemory(core->device, temp->buffer->devMemory);
    if (temp->buffer != NULL) deleteBuffer(core->device, temp->buffer);
}

// 描画結果を画像ファイルに保存する関数
//
// NOTE: 描画先(結果)イメージはホスト(CPU)から見えないデバイスローカルメモリに作られる。
//       デバイスローカルメモリを直接読み取ることはできない。
//       これを解決するには、
//         1. ホストから見えるメモリを確保する
//         2. そのメモリをローカルメモリへマップする
//         3. 描画結果イメージからそのメモリへコピーする
//       ただし、描画結果イメージのイメージレイアウトがVK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMALであることが前提である。
//       今回はレンダーパスの最後にそうなるよう設定している。
//       もし、VK_IMAGE_LAYOUT_PRESENT_SRC_KHR等である場合は、vkCmdPipelineBarrier()関数でイメージレイアウトを変更する必要がある。
int saveRenderingResult(const VulkanAppCore core, const VulkanAppOffscreen offscreen) {
#define CHECK_VK(p, m) ERROR_IF_WITH((p) != VK_SUCCESS, "saveRenderingResult()", (m), (p), deleteTempObjsSaveRenderingResult(core, &temp), 0)
#define CHECK(p, m)    ERROR_IF     (!(p),              "saveRenderingResult()", (m),      deleteTempObjsSaveRenderingResult(core, &temp), 0)
#define COMMAND_BUFFERS_COUNT 1

    TempObjsSaveRenderingResult temp = {
        NULL,
        0,
        NULL
    };

    // 一時バッファを作成する
    {
        temp.buffer = createBuffer(
            core->device,
            &core->physDevMemProps,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
            offscreen->image->memReqs.size
        );
        CHECK(temp.buffer != NULL, "一時バッファの作成に失敗");
    }

    // マップする
    void *mappedData;
    {
        CHECK_VK(vkMapMemory(core->device, temp.buffer->devMemory, 0, temp.buffer->memReqs.size, 0, &mappedData), "マップに失敗");
        temp.mapped = 1;
    }

    // コマンドバッファを確保し記録を開始する
    VkCommandBuffer cmdBuffer;
    {
        cmdBuffer = allocateAndStartCommandBuffer(core);
        CHECK(cmdBuffer != NULL, "コマンドバッファの確保あるいは記録の開始に失敗");
    }

    // コピーコマンドを記録する
    {
#define REGIONS_COUNT 1
        const VkBufferImageCopy regions[REGIONS_COUNT] = {
            {
                0,
                0,
                0,
                { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
                { 0, 0, 0 },
                offscreen->image->extent,
            }
        };
        vkCmdCopyImageToBuffer(cmdBuffer, offscreen->image->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, temp.buffer->buffer, REGIONS_COUNT, regions);
#undef REGIONS_COUNT
    }

    // コマンドバッファを終了しキューに提出する
    {
        CHECK(endAndSubmitCommandBuffer(core, cmdBuffer, 0, NULL, NULL, 0, NULL), "コマンドバッファの終了あるいは提出に失敗");
    }

    // 計算終了を待機する
    {
        vkDeviceWaitIdle(core->device);
    }

    // 描画結果を加工して取得する
    {
        const uint32_t wh = offscreen->image->extent.width * offscreen->image->extent.height;
        temp.pixels = (uint8_t *)malloc(sizeof(uint8_t) * wh * 4);
        for (uint32_t i = 0; i < wh; ++i) {
            temp.pixels[i * 4 + 0] = ((uint8_t *)mappedData)[i * 4 + 2];
            temp.pixels[i * 4 + 1] = ((uint8_t *)mappedData)[i * 4 + 1];
            temp.pixels[i * 4 + 2] = ((uint8_t *)mappedData)[i * 4 + 0];
            temp.pixels[i * 4 + 3] = ((uint8_t *)mappedData)[i * 4 + 3];
        }
    }

    // 描画結果をpngに保存する
    {
        CHECK(
            stbi_write_png(
                "rendering-result.png",
                offscreen->image->extent.width,
                offscreen->image->extent.height,
                sizeof(uint8_t) * 4,
                (const void *)temp.pixels,
                0
            ),
            "描画結果の保存に失敗"
        );
    }

    deleteTempObjsSaveRenderingResult(core, &temp);
    return 1;
    
#undef CHECK
#undef CHECK_VK
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// オフスクリーン向けVulkanアプリケーションで必要なモジュールを持つ構造体
typedef struct ModulesForOffscreen_t {
    VulkanAppCore core;
    VulkanAppOffscreen offscreen;
    VulkanAppRendering renderer;
} ModulesForOffscreen;

void deleteModulesForOffscreen(const ModulesForOffscreen *mods) {
    if (mods == NULL) {
        return;
    }
    if (mods->renderer != NULL) deleteVulkanAppRendering(mods->core, mods->renderer);
    if (mods->offscreen != NULL) deleteVulkanAppOffscreen(mods->core, mods->offscreen);
    if (mods->core != NULL) deleteVulkanAppCore(mods->core);
}

int runOnOffscreen(int width, int height) {
#define CHECK(p, m) ERROR_IF(!(p), "runOnOffscreen()", (m), deleteModulesForOffscreen(&mods), 1)

    ModulesForOffscreen mods = {
        NULL,
        NULL,
        NULL,
    };

    // 主要オブジェクトを作成する
    const uint32_t instLayerNamesCount = 1;
    const char *instLayerNames[] = { "VK_LAYER_KHRONOS_validation" };
    mods.core = createVulkanAppCore(instLayerNamesCount, instLayerNames, 0, NULL, 0, NULL, 0, NULL);
    CHECK(mods.core != NULL, "主要オブジェクトの作成に失敗");

    // オフスクリーン依存オブジェクトを作成する
    mods.offscreen = createVulkanAppOffscreen(mods.core, width, height);
    CHECK(mods.offscreen != NULL, "オフスクリーン依存オブジェクトの作成に失敗");

    // レンダリングオブジェクトを作成する
    //
    // NOTE: 描画結果イメージのデータを取得するためにレンダーパスの最後にイメージレイアウトをVK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMALにする。
    const uint32_t imageViewsCount = 1;
    const VkImageView imageViews[] = { mods.offscreen->imageView };
    mods.renderer = createVulkanAppRendering(
        mods.core,
        imageViews,
        imageViewsCount,
        width,
        height,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
    );
    CHECK(mods.renderer != NULL, "レンダリングオブジェクトの作成に失敗");

    // 描画する
    CHECK(render(mods.core, mods.renderer, 0, 0, 0, width, height, 0, NULL, NULL, 0, NULL), "描画に失敗");

    // 描画の完了を待機する
    vkDeviceWaitIdle(mods.core->device);

    // 描画結果を画像ファイルに保存する
    CHECK(saveRenderingResult(mods.core, mods.offscreen), "描画結果の保存に失敗");

    deleteModulesForOffscreen(&mods);
    return 0;

#undef CHECK
}
