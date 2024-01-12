/// @file rendering.h
/// @brief Vulkanアプリケーションのレンダリングオブジェクトに関するモジュール

#pragma once

#include "core.h"
#include "pipelines/ui.h"
#include "util/memory/buffer.h"
#include "util/model.h"

#include <stdint.h>
#include <vulkan/vulkan.h>

/// @brief Vulkanアプリケーションのレンダリングオブジェクトを持つ構造体
typedef struct VulkanAppRendering_t {
    VkRenderPass renderPass;
    VkFramebuffer *framebuffers;
    uint32_t framebuffersCount;
    VkDescriptorPool descPool;
    PipelineForUI uiPipeline;
    VkDescriptorSet descSetForUI;
    Buffer uniBufferForUI;
    Model square;
} *VulkanAppRendering;

/// @brief Vulkanアプリケーションのレンダリングオブジェクトを破棄する関数
/// @param core 主要オブジェクトハンドル
/// @param renderer レンダリングオブジェクトハンドル
void deleteVulkanAppRendering(const VulkanAppCore core, VulkanAppRendering renderer);

/// @brief Vulkanアプリケーションのレンダリングオブジェクトを作成する関数
///
/// @param core 主要オブジェクトハンドル
/// @param imageViews 描画先イメージビューの配列
/// @param imageViewsCount imageViewsの要素数
/// @param width 描画先イメージビューの幅
/// @param height 描画先イメージビューの高
/// @param imageLayout 描画先イメージのレイアウト
/// @returns 失敗時にNULLを返す。
VulkanAppRendering createVulkanAppRendering(
    const VulkanAppCore core,
    const VkImageView *imageViews,
    uint32_t imageViewsCount,
    uint32_t width,
    uint32_t height,
    VkImageLayout imageLayout
);

/// @brief 描画関数
/// @param core 主要オブジェクトハンドル
/// @param renderer レンダリングオブジェクトハンドル
/// @param framebufferIndex 描画先フレームバッファのインデックス
/// @param offsetX 描画領域の左オフセット
/// @param offsetY 描画領域の上オフセット
/// @param width 描画領域の幅
/// @param height 描画領域の高
/// @param waitSemaphoresCount waitForImageEnabledSemaphoresの要素数
/// @param waitSemaphores 描画開始を待機するセマフォの配列
/// @param waitDstStageMasks waitForImageEnabledSemaphoresのそれぞれのセマフォがどのパイプラインステージを待機するかの配列
/// @param signalSemaphoresCount waitForRenderingSemaphoresの要素数
/// @param signalSemaphores 描画終了を待機するセマフォの配列
/// @returns 正常終了時に1を返す。異常終了時に0を返す。
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
);
