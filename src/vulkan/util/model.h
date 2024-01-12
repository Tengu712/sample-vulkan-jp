/// @file model.h
/// @brief モデルに必要なオブジェクトに関するモジュール

#pragma once

#include "memory/buffer.h"

#include <vulkan/vulkan.h>

typedef struct Model_t {
    int indicesCount;
    Buffer vtxBuffer;
    Buffer idxBuffer;
    const char *data;
} *Model;

/// @brief Modelを破棄する関数
/// @param device 論理デバイス
/// @param model モデルハンドル
void deleteModel(const VkDevice device, Model model);

/// @brief モデルデータファイルからモデルを作成する関数
/// @param device 論理デバイス
/// @param physDevMemProps 物理デバイス上のメモリプロパティ
/// @param path ファイルパス
/// @returns 失敗時にNULLを返す。
Model createModelFromFile(const VkDevice device, const VkPhysicalDeviceMemoryProperties *physDevMemProps, const char *path);
