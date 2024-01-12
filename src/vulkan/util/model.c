#include "model.h"

#include "error.h"
#include "file.h"
#include "memory/memory.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

void deleteModel(const VkDevice device, Model model) {
  if (model == NULL) {
    return;
  }
  vkDeviceWaitIdle(device);
  if (model->idxBuffer != NULL) deleteBuffer(device, model->idxBuffer);
  if (model->vtxBuffer != NULL) deleteBuffer(device, model->vtxBuffer);
  if (model->data != NULL) free((void *)model->data);
  free((void *)model);
}

Model createModelFromFile(const VkDevice device, const VkPhysicalDeviceMemoryProperties *physDevMemProps, const char *path) {
#define CHECK_VK(p, m) ERROR_IF_WITH((p) != VK_SUCCESS, "createModelFromFile()", (m), (p), deleteModel(device, model), NULL)
#define CHECK(p, m)    ERROR_IF     (!(p),              "createModelFromFile()", (m),      deleteModel(device, model), NULL)

    ((void*)physDevMemProps);

    const Model model = (Model)malloc(sizeof(struct Model_t));
    CHECK(model != NULL, "Modelの確保に失敗");
    memset(model, 0, sizeof(struct Model_t));

    // モデルデータを読み込む
    {
        model->data = readBinaryFile(path, NULL);
        CHECK(model->data != NULL, "モデルデータの読込みに失敗");
    }

    // キャスト済み配列を用意する
    const uint32_t *dataUInt32 = (const uint32_t *)model->data;
    const float *dataFloat = (const float *)model->data;

    // 一頂点におけるサイズを取得する
    uint32_t sizePerVertex = 3;
    {
        const uint32_t flags = dataUInt32[0];
        // normal
        if (flags & 1) sizePerVertex += 3;
        // uv
        if (flags & 2) sizePerVertex += 2;
    }

    // 頂点数を取得する
    const uint32_t verticesCount = dataUInt32[1];
    // 頂点データを取得する
    const float *vertices = &dataFloat[2];
    const uint32_t verticesSize = sizeof(float) * sizePerVertex * verticesCount;
    // インデックス数を取得する
    const uint32_t indicesCount = dataUInt32[1 + 1 + sizePerVertex * verticesCount];
    // インデックスデータを取得する
    const uint32_t *indices = &dataUInt32[1 + 1 + sizePerVertex * verticesCount + 1];
    const uint32_t indicesSize = sizeof(uint32_t) * indicesCount;

    // 頂点バッファを作成しアップロードする
    {
        model->vtxBuffer = createBuffer(
            device,
            physDevMemProps,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
            verticesSize            
        );
        CHECK(model->vtxBuffer != NULL, "頂点バッファの作成に失敗");
        CHECK(uploadToDeviceMemory(device, model->vtxBuffer->devMemory, vertices, verticesSize), "頂点データのアップロードに失敗");
    }

    // インデックスバッファを作成しアップロードする
    {
        model->idxBuffer = createBuffer(
            device,
            physDevMemProps,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
            indicesSize            
        );
        CHECK(model->idxBuffer != NULL, "インデックスバッファの作成に失敗");
        CHECK(uploadToDeviceMemory(device, model->idxBuffer->devMemory, indices, indicesSize), "インデックスデータのアップロードに失敗");
    }

    // データを解放する
    {
        free((void *)model->data);
    }

    // インデックス数を格納する
    {
        model->indicesCount = indicesCount;
    }

    return model;

#undef CHECK
#undef CHECK_VK
}
