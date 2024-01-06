#include "offscreen.h"

// stb_image.hをヘッダーファイルのみで利用するためにはこのマクロを定義する
#define STB_IMAGE_IMPLEMENTATION
// stb_image_write.hをヘッダーファイルのみで利用するためにはこのマクロを定義する
#define STB_IMAGE_WRITE_IMPLEMENTATION
// stb_image_write.h内のsprintf()関数の使用に対してwarningを出さないためにこのマクロを定義する
#define _CRT_SECURE_NO_WARNINGS

#include "../common.h"
#include "../core.h"
#include "../rendering.h"
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
    VkExtent3D imageExtent;
    VkImage image;
    VkMemoryRequirements memReqs;
    VkDeviceMemory devMemory;
    VkImageView imageView;
} *VulkanAppOffscreen;

void deleteVulkanAppOffscreen(const VulkanAppCore core, VulkanAppOffscreen offscreen) {
    if (offscreen == NULL) {
        return;
    }
    vkDeviceWaitIdle(core->device);
    if (offscreen->imageView != NULL) vkDestroyImageView(core->device, offscreen->imageView, NULL);
    if (offscreen->devMemory != NULL) vkFreeMemory(core->device, offscreen->devMemory, NULL);
    if (offscreen->image != NULL) vkDestroyImage(core->device, offscreen->image, NULL);
    free((void *)offscreen);
}

VulkanAppOffscreen createVulkanAppOffscreen(const VulkanAppCore core, int width, int height) {
#define CHECK_VK(p, m) ERROR_IF_WITH((p) != VK_SUCCESS, "createVulkanAppOffscreen()", (m), (p), deleteVulkanAppOffscreen(core, offscreen), NULL)
#define CHECK(p, m)    ERROR_IF     (!(p),              "createVulkanAppOffscreen()", (m),      deleteVulkanAppOffscreen(core, offscreen), NULL)

    const VulkanAppOffscreen offscreen = (VulkanAppOffscreen)malloc(sizeof(struct VulkanAppOffscreen_t));
    CHECK(offscreen != NULL, "VulkanAppOffscreenの確保に失敗");

    // イメージのextentを設定する
    {
        offscreen->imageExtent.width = (uint32_t)width;
        offscreen->imageExtent.height = (uint32_t)height;
        offscreen->imageExtent.depth = 1;
    }

    // 描画先イメージを作成する
    //
    // NOTE: この描画先イメージは後々画像ファイルに保存するために一時バッファへコピーを行う。
    //       そのため、コピー元としても使えるようVK_IMAGE_USAGE_TRANSFER_SRC_BITを指定する。
    {
        const VkImageCreateInfo ci = {
            VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            NULL,
            0,
            VK_IMAGE_TYPE_2D,
            SURFACE_PIXEL_FORMAT,
            offscreen->imageExtent,
            1,
            1,
            VK_SAMPLE_COUNT_1_BIT,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
            VK_SHARING_MODE_EXCLUSIVE,
            0,
            NULL,
            VK_IMAGE_LAYOUT_UNDEFINED,
        };
        CHECK_VK(vkCreateImage(core->device, &ci, NULL, &offscreen->image), "描画先イメージの作成に失敗");
    }

    // 描画先イメージの必要条件を取得する
    {
        vkGetImageMemoryRequirements(core->device, offscreen->image, &offscreen->memReqs);
    }

    // 描画先イメージのためのデバイスメモリを確保する
    {
        offscreen->devMemory = allocateDeviceMemory(
            core->device,
            &core->physDevMemProps,
            offscreen->memReqs.memoryTypeBits,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            offscreen->memReqs.size
        );
        CHECK(offscreen->devMemory != NULL, "描画先イメージのデバイスメモリの確保に失敗");
    }

    // 描画先イメージとデバイスメモリとを関連付ける
    {
        CHECK_VK(vkBindImageMemory(core->device, offscreen->image, offscreen->devMemory, 0), "描画先イメージのイメージとデバイスメモリとの関連付けに失敗");
    }

    // 描画先イメージのイメージビューを作成
    {
        const VkImageViewCreateInfo ci = {
            VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            NULL,
            0,
            offscreen->image,
            VK_IMAGE_VIEW_TYPE_2D,
            SURFACE_PIXEL_FORMAT,
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
    VkBuffer buffer;
    VkMemoryRequirements memReqs;
    VkDeviceMemory devMemory;
    int mapped;
    uint8_t *pixels;
} TempObjsSaveRenderingResult;

void deleteTempObjsSaveRenderingResult(const VulkanAppCore core, TempObjsSaveRenderingResult *temp) {
    if (temp == NULL) {
        return;
    }
    vkDeviceWaitIdle(core->device);
    if (temp->pixels != NULL) free((void *)temp->pixels);
    if (temp->mapped) vkUnmapMemory(core->device, temp->devMemory);
    if (temp->devMemory != NULL) vkFreeMemory(core->device, temp->devMemory, NULL);
    if (temp->buffer != NULL) vkDestroyBuffer(core->device, temp->buffer, NULL);
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
        { 0 },
        NULL,
        0,
        NULL
    };

    // 一時バッファを作成する
    {
        const VkBufferCreateInfo ci = {
            VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            NULL,
            0,
            offscreen->memReqs.size,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_SHARING_MODE_EXCLUSIVE,
            0,
            NULL
        };
        CHECK_VK(vkCreateBuffer(core->device, &ci, NULL, &temp.buffer), "一時バッファの作成に失敗");
    }

    // 一時バッファの必要条件を取得する
    {
        vkGetBufferMemoryRequirements(core->device, temp.buffer, &temp.memReqs);
    }

    // 一時バッファのためのデバイスメモリを確保する
    {
        temp.devMemory = allocateDeviceMemory(
            core->device,
            &core->physDevMemProps,
            temp.memReqs.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
            temp.memReqs.size
        );
        CHECK(temp.devMemory != NULL, "一時バッファのためのデバイスメモリの確保に失敗");
    }

    // 一時バッファとデバイスメモリとを関連付ける
    {
        CHECK_VK(vkBindBufferMemory(core->device, temp.buffer, temp.devMemory, 0), "一時バッファとデバイスメモリとの関連付けに失敗");
    }

    // マップする
    void *mappedData;
    {
        CHECK_VK(vkMapMemory(core->device, temp.devMemory, 0, temp.memReqs.size, 0, &mappedData), "マップに失敗");
        temp.mapped = 1;
    }

    // コマンドバッファを確保し記録を開始する
    VkCommandBuffer cmdBuffer;
    {
        CHECK(allocateAndStartCommandBuffer(core, &cmdBuffer), "コマンドバッファの確保あるいは記録の開始に失敗");
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
                offscreen->imageExtent,
            }
        };
        vkCmdCopyImageToBuffer(cmdBuffer, offscreen->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, temp.buffer, REGIONS_COUNT, regions);
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
        temp.pixels = (uint8_t *)malloc(sizeof(uint8_t) * offscreen->imageExtent.width * offscreen->imageExtent.height * 4);
        for (uint32_t i = 0; i < offscreen->imageExtent.width * offscreen->imageExtent.height; ++i) {
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
                offscreen->imageExtent.width,
                offscreen->imageExtent.height,
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
