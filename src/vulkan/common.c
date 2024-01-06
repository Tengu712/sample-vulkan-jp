#include "common.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan.h>

VkDeviceMemory allocateDeviceMemory(
    const VkDevice device,
    const VkPhysicalDeviceMemoryProperties *physDevMemProps,
    uint32_t type,
    VkMemoryPropertyFlags props,
    VkDeviceSize size
) {
#define CHECK_VK(p, m) ERROR_IF_WITH((p) != VK_SUCCESS, "allocateDeviceMemory()", (m), (p), "", NULL)
#define CHECK(p, m)    ERROR_IF     (!(p),              "allocateDeviceMemory()", (m),      "", NULL)

    // 要求に従った物理デバイス上のメモリタイプのインデックスを取得する
    uint32_t memTypeIndex = 0;
    {
        int found = 0;
        for (uint32_t i = 0; i < physDevMemProps->memoryTypeCount; ++i) {
            if ((1 << i) & type) {
                if ((physDevMemProps->memoryTypes[i].propertyFlags & props) == props) {
                    memTypeIndex = i;
                    found = 1;
                    break;
                }
            }
        }
        CHECK(found, "メモリタイプのインデックスの取得に失敗");
    }

    // デバイスメモリを確保する
    VkDeviceMemory devMemory;
    {
        const VkMemoryAllocateInfo ai = {
            VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            NULL,
            size,
            memTypeIndex,
        };
        CHECK_VK(vkAllocateMemory(device, &ai, NULL, &devMemory), "デバイスメモリの確保に失敗");
    }

    return devMemory;

#undef CHECK
#undef CHECK_VK
}
