/// @file common.h
/// @brief Vulkanアプリケーションの定義モジュールにおいて用いる共通機能
/// @warning Vulkanアプリケーション内部向けのヘッダーファイルであるため、外部からはincludeすべきではない。

#pragma once

#include <stdint.h>
#include <vulkan/vulkan.h>

#define ERROR_LOG_WITH(f, m, s) printf("[ error ] %s: %s: %d\n", (f), (m), (s));
#define ERROR_LOG(f, m)         printf("[ error ] %s: %s\n", (f), (m));

#define ERROR_IF_WITH(condExpr, funName, message, status, destruction, retValue) \
    if ((condExpr)) {                                                            \
        ERROR_LOG_WITH((funName), (message), (status));                          \
        (destruction);                                                           \
        return (retValue);                                                       \
    }

#define ERROR_IF(condExpr, funName, message, destruction, retValue) \
    if ((condExpr)) {                                               \
        ERROR_LOG((funName), (message));                            \
        (destruction);                                              \
        return (retValue);                                          \
    }

#define SURFACE_PIXEL_FORMAT VK_FORMAT_B8G8R8A8_SRGB
#define SURFACE_COLOR_SPACE VK_COLOR_SPACE_SRGB_NONLINEAR_KHR

/// @brief デバイスメモリを確保する関数
/// @param device 論理デバイス
/// @param physDevMemProps 物理デバイス上のメモリプロパティ
/// @param type 要求するメモリタイプ
/// @param props 要求するメモリプロパティ
/// @param size 確保するメモリのサイズ
/// @returns 発見できなかった場合NULLを返す。
VkDeviceMemory allocateDeviceMemory(
    const VkDevice device,
    const VkPhysicalDeviceMemoryProperties *physDevMemProps,
    uint32_t type,
    VkMemoryPropertyFlags props,
    VkDeviceSize size
);
