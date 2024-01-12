/// @file memory.h
/// @brief Vulkanにおけるメモリ関連の共通機能を定義するモジュール

#pragma once

#include <stdint.h>
#include <vulkan/vulkan.h>

/// @brief デバイスメモリを確保する関数
/// @param device 論理デバイス
/// @param physDevMemProps 物理デバイス上のメモリプロパティ
/// @param type 要求するメモリタイプ
/// @param memPropFlags 要求するメモリプロパティ
/// @param size 確保するメモリのサイズ
VkDeviceMemory allocateDeviceMemory(
    const VkDevice device,
    const VkPhysicalDeviceMemoryProperties *physDevMemProps,
    uint32_t type,
    VkMemoryPropertyFlags memPropFlags,
    VkDeviceSize size
);

/// @brief デバイスメモリにデータをアップロードする関数
/// @param device 論理デバイス
/// @param devMemory アップロード先のデバイスメモリ
/// @param source アップロードするデータ
/// @param size データサイズ
/// @param 失敗時に0を返す
int uploadToDeviceMemory(const VkDevice device, const VkDeviceMemory devMemory, const void *source, uint32_t size);
