#include "buffer.h"

#include "../error.h"
#include "memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void deleteBuffer(const VkDevice device, Buffer buffer) {
    if (buffer == NULL) {
        return;
    }
    vkDeviceWaitIdle(device);
    if (buffer->devMemory != NULL) vkFreeMemory(device, buffer->devMemory, NULL);
    if (buffer->buffer != NULL) vkDestroyBuffer(device, buffer->buffer, NULL);
    free((void *)buffer);
}

Buffer createBuffer(
    const VkDevice device,
    const VkPhysicalDeviceMemoryProperties *physDevMemProps,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags memPropFlags,
    VkDeviceSize size
) {
#define CHECK_VK(p, m) ERROR_IF_WITH((p) != VK_SUCCESS, "createBuffer()", (m), (p), deleteBuffer(device, buffer), NULL)
#define CHECK(p, m)    ERROR_IF     (!(p),              "createBuffer()", (m),      deleteBuffer(device, buffer), NULL)

    const Buffer buffer = (Buffer)malloc(sizeof(struct Buffer_t));
    CHECK(buffer != NULL, "バッファの確保に失敗");
    memset(buffer, 0, sizeof(struct Buffer_t));

    // バッファを作成する
    {
        const VkBufferCreateInfo ci = {
            VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            NULL,
            0,
            size,
            usage,
            VK_SHARING_MODE_EXCLUSIVE,
            0,
            NULL,
        };
        CHECK_VK(vkCreateBuffer(device, &ci, NULL, &buffer->buffer), "バッファの作成に失敗");
    }

    // バッファの必要条件を取得する
    {
        vkGetBufferMemoryRequirements(device, buffer->buffer, &buffer->memReqs);
    }

    // バッファのためのデバイスメモリを確保する
    {
        buffer->devMemory = allocateDeviceMemory(
            device,
            physDevMemProps,
            buffer->memReqs.memoryTypeBits,
            memPropFlags,
            size
        );
        CHECK(buffer->devMemory != NULL, "バッファのためのデバイスメモリの確保に失敗");
    }

    // バッファとデバイスメモリとを関連付ける
    {
        CHECK_VK(vkBindBufferMemory(device, buffer->buffer, buffer->devMemory, 0), "バッファとデバイスメモリとの関連付けに失敗");
    }

    return buffer;

#undef CHECK
#undef CHECK_VK
}
