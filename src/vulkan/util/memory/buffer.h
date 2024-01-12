/// @file buffer.h
/// @brief バッファに関するモジュール

#pragma once

#include <vulkan/vulkan.h>

/// @brief バッファに必要なオブジェクトを持つ構造体
typedef struct Buffer_t {
    VkBuffer buffer;
    VkDeviceMemory devMemory;
    VkMemoryRequirements memReqs;
} *Buffer;

/// @brief Bufferを破棄する関数
/// @param device 論理デバイス
/// @param buffer バッファオブジェクトハンドル
void deleteBuffer(const VkDevice device, Buffer buffer);

/// @brief Bufferを作成する関数
/// @param device 論理デバイス
/// @param physDevMemProps 物理デバイス上のメモリプロパティ
/// @param usage バッファの使用方法
/// @param memPropFlags 要求するメモリプロパティ
/// @param size 確保するバッファのサイズ
/// @returns 失敗時にNULLを返す。
Buffer createBuffer(
    const VkDevice device,
    const VkPhysicalDeviceMemoryProperties *physDevMemProps,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags memPropFlags,
    VkDeviceSize size
);
