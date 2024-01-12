#include "shader.h"

#include "error.h"
#include "file.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

VkShaderModule createShaderModuleFromFile(const VkDevice device, const char *path) {
#define CHECK_VK(p, m, d) ERROR_IF_WITH((p) != VK_SUCCESS, "createShaderModuleFromFile()", (m), (p), d, NULL)
#define CHECK(p, m, d)    ERROR_IF     (!(p),              "createShaderModuleFromFile()", (m),      d, NULL)

    // ファイルを読み込む
    const char *binary;
    long int binarySize = 0;
    {
        binary = readBinaryFile(path, &binarySize);
        CHECK(binary != NULL, "シェーダファイルの読込みに失敗", {});
    }

    // シェーダモジュールを作成する
    VkShaderModule shader;
    {
        const VkShaderModuleCreateInfo ci = {
            VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            NULL,
            0,
            binarySize,
            (const uint32_t*)binary,
        };
        CHECK_VK(vkCreateShaderModule(device, &ci, NULL, &shader), "シェーダモジュールの作成に失敗", free((void *)binary));
    }

    // メモリを解放する
    {
        free((void *)binary);
    }

    return shader;

#undef CHECK
#undef CHECK_VK
}
