/// @file image.h
/// @brief イメージに関するモジュール

#pragma once

#include <vulkan/vulkan.h>

/// @brief イメージに必要なオブジェクトを持つ構造体
typedef struct Image_t {
    VkExtent3D extent;
    VkImage image;
    VkDeviceMemory devMemory;
    VkMemoryRequirements memReqs;
} *Image;

/// @brief Imageを破棄する関数
/// @param device 論理デバイス
/// @param image イメージオブジェクトハンドル
void deleteImage(const VkDevice device, Image image);

/// @brief Imageを作成する関数
/// @param device 論理デバイス
/// @param physDevMemProps 物理デバイス上のメモリプロパティ
/// @param usage バッファの使用方法
/// @param memPropFlags 要求するメモリプロパティ
/// @param format ピクセルフォーマット
/// @param extent 三次元サイズ
/// @returns 失敗時にNULLを返す。
Image createImage(
    const VkDevice device,
    const VkPhysicalDeviceMemoryProperties *physDevMemProps,
    VkImageUsageFlags usage,
    VkMemoryPropertyFlags memPropFlags,
    VkFormat format,
    const VkExtent3D *extent
);
