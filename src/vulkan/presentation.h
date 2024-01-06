/// @file presentation.h
/// @brief Vulkanアプリケーションのプレゼンテーションオブジェクトに関するモジュール
/// @warning Vulkanアプリケーション内部向けのヘッダーファイルであるため、外部からはincludeすべきではない。

#pragma once

#include "core.h"

#include <stdint.h>
#include <vulkan/vulkan.h>

/// @brief Vulkanアプリケーションのプレゼンテーションオブジェクトを持つ構造体
typedef struct VulkanAppPresentation_t {
    uint32_t width;
    uint32_t height;
    VkSwapchainKHR swapchain;
    uint32_t imagesCount;
    VkImageView *imageViews;
    uint32_t imageIndex;
    VkSemaphore waitForImageEnabledSemaphore;
    VkSemaphore waitForRenderingSemaphore;
} *VulkanAppPresentation;

/// @brief Vulkanアプリケーションのプレゼンテーションオブジェクトを作成する関数
///
/// 与えられたサーフェスに関連したスワップチェーンを作成し、スワップチェーンの持つ描画先イメージのイメージビューを作成する。
/// これ単体では描画能力は持っていないが、レンダリングオブジェクトによってサーフェスへ描画することができるようになる。
///
/// プラットフォームの差異を吸収するため、サーフェスはプラットフォーム依存のコードで生成し、引数に与える。
///
/// @param core 主要オブジェクトハンドル
/// @param surface サーフェス
/// @returns 失敗時にNULLを返す。
VulkanAppPresentation createVulkanAppPresentation(const VulkanAppCore core, const VkSurfaceKHR surface);

/// @brief Vulkanアプリケーションのプレゼンテーションオブジェクトを破棄する関数
/// @param core 主要オブジェクトハンドル
/// @param presenter プレゼンテーションオブジェクトハンドル
void deleteVulkanAppPresentation(const VulkanAppCore core, VulkanAppPresentation presenter);

/// @brief 次のフレームバッファのインデックスを取得する関数
/// @param core 主要オブジェクトハンドル
/// @param presenter プレゼンテーションオブジェクトハンドル
/// @returns 失敗時に0を返す。
int acquireNextImageIndex(const VulkanAppCore core, const VulkanAppPresentation presenter);

/// @brief プレゼンテーションを行う関数
/// @param core 主要オブジェクトハンドル
/// @param presenter プレゼンテーションオブジェクトハンドル
/// @returns 失敗時に0を返す。
int present(const VulkanAppCore core, const VulkanAppPresentation presenter);
