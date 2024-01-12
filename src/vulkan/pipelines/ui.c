#include "ui.h"

#include "../util/error.h"
#include "../util/shader.h"

#include <stdio.h>
#include <stdlib.h>

void deletePipelineForUI(const VkDevice device, PipelineForUI pipeline) {
    if (pipeline == NULL) {
        return;
    }
    vkDeviceWaitIdle(device);
    if (pipeline->pipeline != NULL) vkDestroyPipeline(device, pipeline->pipeline, NULL);
    if (pipeline->fragShader != NULL) vkDestroyShaderModule(device, pipeline->fragShader, NULL);
    if (pipeline->vertShader != NULL) vkDestroyShaderModule(device, pipeline->vertShader, NULL);
    if (pipeline->pipelineLayout != NULL) vkDestroyPipelineLayout(device, pipeline->pipelineLayout, NULL);
    if (pipeline->descSetLayout != NULL) vkDestroyDescriptorSetLayout(device, pipeline->descSetLayout, NULL);
    free((void *)pipeline);
}

PipelineForUI createPipelineForUI(const VkDevice device, const VkRenderPass renderPass, uint32_t width, uint32_t height) {
#define CHECK_VK(p, m) ERROR_IF_WITH((p) != VK_SUCCESS, "createPipelineForUI()", (m), (p), deletePipelineForUI(device, pipeline), NULL)
#define CHECK(p, m)    ERROR_IF     (!(p),              "createPipelineForUI()", (m),      deletePipelineForUI(device, pipeline), NULL)

    const PipelineForUI pipeline = (PipelineForUI)malloc(sizeof(struct PipelineForUI_t));
    CHECK(pipeline != NULL, "PipelineForUIの確保に失敗");
    pipeline->descSetLayout = NULL;
    pipeline->pipelineLayout = NULL;
    pipeline->vertShader = NULL;
    pipeline->fragShader = NULL;
    pipeline->pipeline = NULL;

    // ディスクリプタセットレイアウトを作成する
    {
#define BINDINGS_COUNT 1
        const VkDescriptorSetLayoutBinding bindings[BINDINGS_COUNT] = {
            {
                0,
                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                1,
                VK_SHADER_STAGE_VERTEX_BIT,
                NULL,
            },
        };
        const VkDescriptorSetLayoutCreateInfo ci = {
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            NULL,
            0,
            BINDINGS_COUNT,
            bindings,
        };
        CHECK_VK(vkCreateDescriptorSetLayout(device, &ci, NULL, &pipeline->descSetLayout), "ディスクリプタセットレイアウトの作成に失敗");
#undef BINDINGS_COUNT
    }

    // パイプラインレイアウトを作成する
    {
#define DESC_SET_LAYOUTS_COUNT 1
#define PUSH_CONSTANTS_COUNT 1
        const VkDescriptorSetLayout descSetLayouts[DESC_SET_LAYOUTS_COUNT] = { pipeline->descSetLayout };
        const VkPushConstantRange pushConstants[PUSH_CONSTANTS_COUNT] = {
            {
                VK_SHADER_STAGE_VERTEX_BIT,
                0,
                sizeof(PushConstantForUI),
            },
        };
        const VkPipelineLayoutCreateInfo ci = {
            VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            NULL,
            0,
            DESC_SET_LAYOUTS_COUNT,
            descSetLayouts,
            PUSH_CONSTANTS_COUNT,
            pushConstants,
        };
        CHECK_VK(vkCreatePipelineLayout(device, &ci, NULL, &pipeline->pipelineLayout), "UI用のパイプラインレイアウトの作成に失敗");
#undef PUSH_CONSTANTS_COUNT
#undef DESC_SET_LAYOUTS_COUNT
    }

    // シェーダモジュールを作成する
    {
        pipeline->vertShader = createShaderModuleFromFile(device, "./shader/ui.vert.spv");
        CHECK(pipeline->vertShader != NULL, "UI用のヴァーテックスシェーダの読込みに失敗: ./shader/ui.vert.spv");
        pipeline->fragShader = createShaderModuleFromFile(device, "./shader/ui.frag.spv");
        CHECK(pipeline->fragShader != NULL, "UI用のフラグメントシェーダの読込みに失敗: ./shader/ui.frag.spv");
    }

    // パイプラインを作成する
    {
#define SHADERS_COUNT 2
#define VERT_INP_BIND_DESCS_COUNT 1
#define VERT_INP_ATTR_DESCS_COUNT 2
#define VIEWPORTS_COUNT 1
#define COLOR_BLEND_ATTACHMENTS_COUNT 1

        // シェーダステージ
        const VkPipelineShaderStageCreateInfo shaderCIs[SHADERS_COUNT] = {
            {
                VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                NULL,
                0,
                VK_SHADER_STAGE_VERTEX_BIT,
                pipeline->vertShader,
                "main",
                NULL,
            },
            {
                VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                NULL,
                0,
                VK_SHADER_STAGE_FRAGMENT_BIT,
                pipeline->fragShader,
                "main",
                NULL,
            },
        };

        // 頂点入力ステート
        const VkVertexInputBindingDescription vertInpBindDescs[VERT_INP_BIND_DESCS_COUNT] = {
            { 0, sizeof(float) * 5, VK_VERTEX_INPUT_RATE_VERTEX },
        };
        const VkVertexInputAttributeDescription vertInpAttrDescs[VERT_INP_ATTR_DESCS_COUNT] = {
            // position
            { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 },
            // uv
            { 1, 0, VK_FORMAT_R32G32_SFLOAT, sizeof(float) * 3 },
        };
        const VkPipelineVertexInputStateCreateInfo vertInpCI = {
            VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            NULL,
            0,
            VERT_INP_BIND_DESCS_COUNT,
            vertInpBindDescs,
            VERT_INP_ATTR_DESCS_COUNT,
            vertInpAttrDescs,
        };

        // 入力アセンブリステート
        const VkPipelineInputAssemblyStateCreateInfo inpAssemCI = {
            VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            NULL,
            0,
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            VK_FALSE,
        };

        // ビューポートステート
        const VkViewport viewports[VIEWPORTS_COUNT] = {
            { 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f },
        };
        const VkRect2D scissors[VIEWPORTS_COUNT] = {
            { {0, 0}, {width, height} },
        };
        const VkPipelineViewportStateCreateInfo viewportCI = {
            VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            NULL,
            0,
            VIEWPORTS_COUNT,
            viewports,
            VIEWPORTS_COUNT,
            scissors,
        };

        // ラスタライゼーションステート
        const VkPipelineRasterizationStateCreateInfo rasterCI = {
            VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            NULL,
            0,
            VK_FALSE,
            VK_FALSE,
            VK_POLYGON_MODE_FILL,
            VK_CULL_MODE_NONE,
            VK_FRONT_FACE_COUNTER_CLOCKWISE,
            VK_FALSE,
            0.0f,
            0.0f,
            0.0f,
            1.0f,
        };

        // マルチサンプルステート
        const VkPipelineMultisampleStateCreateInfo multisampleCI = {
            VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            NULL,
            0,
            VK_SAMPLE_COUNT_1_BIT,
            VK_FALSE,
            0.0f,
            NULL,
            VK_FALSE,
            VK_FALSE,
        };

        // カラーブレンドステート
        const VkPipelineColorBlendAttachmentState colorBlendAttchs[COLOR_BLEND_ATTACHMENTS_COUNT] = {
            {
                VK_TRUE,
                VK_BLEND_FACTOR_SRC_ALPHA,
                VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                VK_BLEND_OP_ADD,
                VK_BLEND_FACTOR_SRC_ALPHA,
                VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                VK_BLEND_OP_ADD,
                VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
            }
        };
        const VkPipelineColorBlendStateCreateInfo colorBlendCI = {
            VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            NULL,
            0,
            VK_FALSE,
            (VkLogicOp)0,
            COLOR_BLEND_ATTACHMENTS_COUNT,
            colorBlendAttchs,
            {0.0f, 0.0f, 0.0f, 0.0f},
        };

        const VkGraphicsPipelineCreateInfo ci = {
            VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            NULL,
            0,
            SHADERS_COUNT,
            shaderCIs,
            &vertInpCI,
            &inpAssemCI,
            NULL,
            &viewportCI,
            &rasterCI,
            &multisampleCI,
            NULL,
            &colorBlendCI,
            NULL,
            pipeline->pipelineLayout,
            renderPass,
            0,
            NULL,
            0,
        };
        CHECK_VK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &ci, NULL, &pipeline->pipeline), "UI用のパイプラインの作成に失敗");

#undef COLOR_BLEND_ATTACHMENTS_COUNT
#undef VIEWPORTS_COUNT
#undef VERT_INP_ATTR_DESCS_COUNT
#undef VERT_INP_BIND_DESCS_COUNT
#undef SHADERS_COUNT
    }

    return pipeline;

#undef CHECK_VK
}
