/// @file ui.h
/// @brief UI用のパイプラインを定義するモジュール

#pragma once

#include <stdint.h>
#include <vulkan/vulkan.h>

/// @brief UI用のシェーダにおけるユニフォームバッファ(binding=0)のレイアウト
typedef struct CameraForUI_t {
    float proj[16];
} CameraForUI;

/// @brief UI用のシェーダにおけるプッシュ定数のレイアウト
typedef struct PushConstantForUI_t {
    float scl[4];
    float trs[4];
    float uv[4];
} PushConstantForUI;

/// @brief UI用のパイプラインにおけるオブジェクトを持つ構造体
typedef struct PipelineForUI_t {
    VkDescriptorSetLayout descSetLayout;
    VkPipelineLayout pipelineLayout;
    VkShaderModule vertShader;
    VkShaderModule fragShader;
    VkPipeline pipeline;
} *PipelineForUI;

/// @brief UI用のパイプラインを破棄する関数
/// @param device 論理デバイス
/// @param pipeline UI用のパイプライン
void deletePipelineForUI(const VkDevice device, PipelineForUI pipeline);

/// @brief UI用のパイプラインを作成する関数
///
/// - シェーダ
///   - ui.vert.spv
///   - ui.frag.spv
/// - バインディング
///   - CameraForUI (binding=0)
/// - プッシュ手数
///   - PushConstantForUI
/// - 頂点データ
///   - ローカル座標 (location=0, float * 3)
///   - UV座標 (location=1, float * 2)
///   - TRIANGLE_LISTで作られるようにこと
/// - ビューポート
///   - 幅width
///   - 高height
/// - カリング無し
/// - マルチサンプリング無し
/// - 深度・ステンシルテスト無し
/// - カラーブレンド
///   - color: src.alpha * src.color + (1 - src.alpha) * dst.color
///   - alpha: ?
///
/// @param device 論理デバイス
/// @param renderPass レンダーパス
/// @param width ビューポート幅
/// @param height ビューポート高
/// @returns 失敗時にNULLを返す。
PipelineForUI createPipelineForUI(const VkDevice device, const VkRenderPass renderPass, uint32_t width, uint32_t height);
