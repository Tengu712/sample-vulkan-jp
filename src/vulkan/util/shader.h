/// @file shader.h
/// @brief シェーダモジュールオブジェクトに関するモジュール

#pragma once

#include <vulkan/vulkan.h>

/// @brief SPIR-Vバイナリファイルからシェーダモジュールを作成する関数
/// @param device 論理デバイス
/// @param path SPIR-Vバイナリファイルのパス
/// @returns 失敗時にNULLを返す。
VkShaderModule createShaderModuleFromFile(const VkDevice device, const char *path);
