/// @file core.h
/// @brief Vulkanアプリケーションの主要オブジェクトに関するモジュール
/// @warning Vulkanアプリケーション内部向けのヘッダーファイルであるため、外部からはincludeすべきではない。

#pragma once

#include <stdint.h>
#include <vulkan/vulkan.h>

/// @brief Vulkanアプリケーションの主要オブジェクトを持つ構造体
typedef struct VulkanAppCore_t {
    VkInstance instance;
    VkPhysicalDevice physDevice;
    VkPhysicalDeviceMemoryProperties physDevMemProps;
    VkDevice device;
    VkQueue queue;
    VkCommandPool cmdPool;
} *VulkanAppCore;

/// @brief Vulkanアプリケーションの主要オブジェクトを作成する関数
///
/// デバイスにコマンドを発行するために必要な最小限のオブジェクトを初期化する。
/// 以降、以下の手順でデバイスに計算させられる。
/// 1. コマンドバッファをコマンドプールから確保
/// 2. コマンドバッファへコマンドを記録
/// 3. コマンドバッファをキューへ提出
///
/// 要件に依って必要な機能が異なるため、その部分は引数に与えるようにしてある。
///
/// @param instLayerNamesCount instLayerNamesの要素数
/// @param instLayerNames Vulkanインスタンスに適応したいレイヤー名の配列
/// @param instExtNamesCount instExtNamesの要素数
/// @param instExtNames Vulkanインスタンスに適応したい拡張機能名の配列
/// @param devLayerNamesCount devLayerNamesの要素数
/// @param devLayerNames 論理デバイスに適応したいレイヤー名の配列
/// @param devExtNamesCount devExtNamesの要素数
/// @param devExtNames 論理デバイスに適応したい拡張機能名の配列
/// @returns 失敗時にNULLを返す。
VulkanAppCore createVulkanAppCore(
    uint32_t instLayerNamesCount,
    const char *const *instLayerNames,
    uint32_t instExtNamesCount,
    const char *const *instExtNames,
    uint32_t devLayerNamesCount,
    const char *const *devLayerNames,
    uint32_t devExtNamesCount,
    const char *const *devExtNames
);

/// @brief Vulkanアプリケーションの主要オブジェクトを破棄する関数
/// @param core 主要オブジェクトハンドル
void deleteVulkanAppCore(VulkanAppCore core);

/// @brief コマンドバッファを確保し記録を開始する関数
///
/// コマンドバッファを一つだけ確保し、コマンドの記録を開始する。
/// なお、コマンドバッファは複数同時に作成できるが、この関数は一つだけ作成する。
/// また、コマンドバッファの内容は実行されたら消滅する。
///
/// @param core 主要オブジェクトハンドル
/// @param cmdBuffer 作成されたコマンドバッファの格納先
/// @returns 失敗時に0を返す
int allocateAndStartCommandBuffer(VulkanAppCore core, VkCommandBuffer *cmdBuffer);

/// @brief コマンドバッファの記録を終了しキューに提出する関数
///
/// このコマンドバッファが実行されるまでwaitSemaphoresのシグナルを待機する。
/// このコマンドバッファが実行されたらsignalSemaphoresをシグナルする。
///
/// コマンドバッファは複数同時にキューに提出するできるが、この関数は一つだけ提出する。
///
/// @param core 主要オブジェクトハンドル
/// @param cmdBuffer コマンドバッファ
/// @param waitSemaphoresCount waitForImageEnabledSemaphoresの要素数
/// @param waitSemaphores 描画開始を待機するセマフォの配列
/// @param waitDstStageMasks waitForImageEnabledSemaphoresのそれぞれのセマフォがどのパイプラインステージを待機するかの配列
/// @param signalSemaphoresCount waitForRenderingSemaphoresの要素数
/// @param signalSemaphores 描画終了を待機するセマフォの配列
/// @return 
int endAndSubmitCommandBuffer(
    VulkanAppCore core,
    VkCommandBuffer cmdBuffer,
    uint32_t waitSemaphoresCount,
    const VkSemaphore *waitSemaphores,
    const VkPipelineStageFlags *waitDstStageMasks,
    uint32_t signalSemaphoresCount,
    const VkSemaphore *signalSemaphores
);
