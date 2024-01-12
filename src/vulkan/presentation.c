#include "presentation.h"

#include "util/constant.h"
#include "util/error.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void deleteVulkanAppPresentation(const VulkanAppCore core, VulkanAppPresentation presenter) {
    if (presenter == NULL) {
        return;
    }
    vkDeviceWaitIdle(core->device);
    if (presenter->waitForRenderingSemaphore) vkDestroySemaphore(core->device, presenter->waitForRenderingSemaphore, NULL);
    if (presenter->waitForImageEnabledSemaphore) vkDestroySemaphore(core->device, presenter->waitForImageEnabledSemaphore, NULL);
    if (presenter->imageViews != NULL) {
        for (uint32_t i = 0; i < presenter->imagesCount; ++i) {
            if (presenter->imageViews[i] != NULL) vkDestroyImageView(core->device, presenter->imageViews[i], NULL);
        }
        free((void *)presenter->imageViews);
    }
    if (presenter->swapchain != NULL) vkDestroySwapchainKHR(core->device, presenter->swapchain, NULL);
    free((void *)presenter);
}

VulkanAppPresentation createVulkanAppPresentation(const VulkanAppCore core, const VkSurfaceKHR surface) {
#define CHECK_VK(p, m) ERROR_IF_WITH((p) != VK_SUCCESS, "createVulkanAppPresentation()", (m), (p), deleteVulkanAppPresentation(core, presenter), NULL)
#define CHECK(p, m)    ERROR_IF     (!(p),              "createVulkanAppPresentation()", (m),      deleteVulkanAppPresentation(core, presenter), NULL)

    const VulkanAppPresentation presenter = (VulkanAppPresentation)malloc(sizeof(struct VulkanAppPresentation_t));
    CHECK(presenter != NULL, "VulkanAppPresentationのメモリ確保に失敗");
    memset(presenter, 0, sizeof(struct VulkanAppPresentation_t));

    // サーフェスが条件を満たしているか確認する
    //
    // NOTE: サーフェスフォーマットは次の二つの情報を持つ。
    //         - ピクセルフォーマット: データ形式
    //         - カラースペース: 色の表現方法
    //       これらを見て、サーフェスが期待しているものであるか確認する。
    {
        uint32_t count = 0;
        CHECK_VK(vkGetPhysicalDeviceSurfaceFormatsKHR(core->physDevice, surface, &count, NULL), "サーフェスフォーマットの数の取得に失敗");
        VkSurfaceFormatKHR *formats = (VkSurfaceFormatKHR *)malloc(sizeof(VkSurfaceFormatKHR) * count);
        CHECK_VK(vkGetPhysicalDeviceSurfaceFormatsKHR(core->physDevice, surface, &count, formats), "サーフェスフォーマットの列挙に失敗");

        int found = 0;
        for (uint32_t i = 0; i < count; ++i) {
            if (formats[i].format == RENDER_TARGET_PIXEL_FORMAT && formats[i].colorSpace == RENDER_TARGET_COLOR_SPACE) {
                found = 1;
                break;
            }
        }

        free(formats);
        CHECK(found, "サーフェスフォーマットの取得に失敗");
    }

    // サーフェスのサイズを取得する & イメージの個数を決定する
    //
    // NOTE: サーフェスのサイズはスクリーンのサイズと同じになるはず。
    //
    // NOTE: サーフェスによって描画先イメージの最小個数が変わる。
    //       ダブルバッファリングのために、最小個数が1個でも2個使うようにする。
    {
        VkSurfaceCapabilitiesKHR capas;
        CHECK_VK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(core->physDevice, surface, &capas), "サーフェスキャパビリティの取得に失敗");

        presenter->width = capas.currentExtent.width;
        presenter->height = capas.currentExtent.height;

        if (capas.minImageCount > 2) {
            presenter->imagesCount = capas.minImageCount;
        } else {
            presenter->imagesCount = 2;
        }
    }

    // スワップチェーンを作成する
    //
    // NOTE: スワップチェーンとは、表示を制御するオブジェクト。
    //       サーフェスに関連して作成され、サーフェスに関連した描画先イメージを持つ。
    //       描画結果をディスプレイに表示したり、ディスプレイの垂直同期を待ったりする機能を持つ。
    //       描画を行うのはデバイスなのであって、スワップチェーンは「表示」を制御するだけに過ぎない。
    //       つまり、オフスクリーンレンダリングがそうであるように、スワップチェーンがなくとも「描画」はできる。
    {
        const VkSwapchainCreateInfoKHR ci = {
            VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            NULL,
            0,
            surface,
            presenter->imagesCount,
            RENDER_TARGET_PIXEL_FORMAT,
            RENDER_TARGET_COLOR_SPACE,
            { presenter->width, presenter->height },
            1,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            VK_SHARING_MODE_EXCLUSIVE,
            0,
            NULL,
            VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
            VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            VK_PRESENT_MODE_FIFO_KHR,
            VK_TRUE,
            NULL,
        };
        CHECK_VK(vkCreateSwapchainKHR(core->device, &ci, NULL, &presenter->swapchain), "スワップチェーンの作成に失敗");
    }

    // 描画先イメージビューを作成する
    //
    // NOTE: イメージとは、Vulkanのメモリリソースである「バッファ」と「イメージ」の後者。
    //       バッファが非構造化データのためのメモリであることに比べ、イメージはフォーマットやレイアウト等の複雑な情報を持つ。
    //       しかし、まだイメージ単体ではメタ情報が少ない。
    //       イメージビューとは、それを解決するためのラッパーオブジェクト。
    {
        VkImage *images = (VkImage *)malloc(sizeof(VkImage) * presenter->imagesCount);
        CHECK_VK(vkGetSwapchainImagesKHR(core->device, presenter->swapchain, &presenter->imagesCount, images), "イメージの取得に失敗");

        presenter->imageViews = (VkImageView *)malloc(sizeof(VkImageView) * presenter->imagesCount);
        for (uint32_t i = 0; i < presenter->imagesCount; ++i) {
            VkImageViewCreateInfo ci = {
                VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                NULL,
                0,
                images[i],
                VK_IMAGE_VIEW_TYPE_2D,
                RENDER_TARGET_PIXEL_FORMAT,
                { 0 },
                { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
            };
            CHECK_VK(vkCreateImageView(core->device, &ci, NULL, &presenter->imageViews[i]), "イメージビューの作成に失敗");
        }

        free(images);
    }

    // 描画開始を待機するためのセマフォと描画完了を待機するためのセマフォを作成する
    //
    // NOTE: 描画先イメージに正しいタイミングで描画するために同期を取る。
    //       また、描画結果イメージを正しいタイミングで画面に表示するために同期を取る。
    //       その同期を実現するためのセマフォを作成する。
    //       これらは適切にrender()関数に指定されなければならない。
    {
        const VkSemaphoreCreateInfo ci = {
            VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            NULL,
            0,
        };
        CHECK_VK(vkCreateSemaphore(core->device, &ci, NULL, &presenter->waitForImageEnabledSemaphore), "描画開始待機用のセマフォの作成に失敗");
        CHECK_VK(vkCreateSemaphore(core->device, &ci, NULL, &presenter->waitForRenderingSemaphore), "描画完了待機用のセマフォの作成に失敗");
    }

    return presenter;

#undef CHECK
#undef CHECK_VK
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int acquireNextImageIndex(const VulkanAppCore core, const VulkanAppPresentation presenter) {
#define CHECK_VK(p, m) ERROR_IF_WITH((p) != VK_SUCCESS, "acquireNextImageIndex()", (m), (p), "", 0)
    // 利用可能な次のイメージのインデックスを取得する
    //
    // NOTE: ダブルバッファリングを行う場合どうせ1枚目2枚目1枚目...と続くので手動でもいいように思えるが必須の処理。
    //       真にイメージが利用可能になるのを同期するために、セマフォを指定する。
    //       フェンスもセマフォもNULLにするとvalidationに怒られる。
    CHECK_VK(
        vkAcquireNextImageKHR(
            core->device,
            presenter->swapchain,
            UINT64_MAX,
            presenter->waitForImageEnabledSemaphore,
            NULL,
            &presenter->imageIndex
        ),
        "次のフレームバッファのインデックスの取得に失敗"
    );
    return 1;
#undef CHECK_VK
}

int present(const VulkanAppCore core, const VulkanAppPresentation presenter) {
#define CHECK_VK(p, m) ERROR_IF_WITH((p) != VK_SUCCESS, "present()", (m), (p), "", 0)
#define SEMAPHORES_COUNT 1
#define SWAPCHAINS_COUNT 1
    // プレゼンテーションコマンドをエンキューする
    //
    // NOTE: これが成功すると画面に描画結果が表示される。
    //       スワップチェーンが垂直同期を取るよう設定されている場合は、この関数でスレッドが一時停止する。
    //       60fpsのリフレッシュレート環境で動作している場合は60fpsループを簡単に実現できる。
    //
    // NOTE: vkQueuePresentKHR()関数は複数のセマフォを待機できる。
    //       今回は描画完了を待機する。
    //
    // NOTE: また、vkQueuePresentKHR()関数は一度に複数のスワップチェーンをプレゼンテーションできる(1スワップチェーンにつき1イメージ)。
    //       それに伴って、各スワップチェーンでのプレゼンテーションが完了したかを取得できる。
    //       今回は一つしかスワップチェーンを使わない(一画面しかない)ため一つだけ指定する。
    const VkSemaphore semaphores[SEMAPHORES_COUNT] = { presenter->waitForRenderingSemaphore };
    const VkSwapchainKHR swapchains[SWAPCHAINS_COUNT] = { presenter->swapchain };
    const uint32_t imageIndices[SWAPCHAINS_COUNT] = { presenter->imageIndex };
    VkResult results[SWAPCHAINS_COUNT] = { VK_SUCCESS };
    const VkPresentInfoKHR pi = {
        VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        NULL,
        SEMAPHORES_COUNT,
        semaphores,
        SWAPCHAINS_COUNT,
        swapchains,
        imageIndices,
        results,
    };
    CHECK_VK(vkQueuePresentKHR(core->queue, &pi), "プレゼンテーションコマンドのエンキューに失敗");
    CHECK_VK(results[0], "プレゼンテーションに失敗");
    return 1;
#undef SWAPCHAINS_COUNT
#undef SEMAPHORES_COUNT
#undef CHECK_VK
}
