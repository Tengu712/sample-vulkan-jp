#include "core.h"

#include "common.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan.h>

VulkanAppCore createVulkanAppCore(
    uint32_t instLayerNamesCount,
    const char *const *instLayerNames,
    uint32_t instExtNamesCount,
    const char *const *instExtNames,
    uint32_t devLayerNamesCount,
    const char *const *devLayerNames,
    uint32_t devExtNamesCount,
    const char *const *devExtNames
) {
#define CHECK_VK(p, m) ERROR_IF_WITH((p) != VK_SUCCESS, "createVulkanAppCore()", (m), (p), deleteVulkanAppCore(core), NULL)
#define CHECK(p, m)    ERROR_IF     (!(p),              "createVulkanAppCore()", (m),      deleteVulkanAppCore(core), NULL)

    const VulkanAppCore core = (VulkanAppCore)malloc(sizeof(struct VulkanAppCore_t));
    CHECK(core != NULL, "VulkanAppCoreのメモリ確保に失敗");

    // Vulkanインスタンスを作成する
    //
    // NOTE: Vulkanはアプリケーションにおいてグローバル環境を作らない。
    //       そのため、Vulkanが状態を保持するためのオブジェクトを作らなければならない。
    //       このオブジェクトがインスタンス。
    //       言ってしまえば、まさにVulkanAppCoreのようなもの。
    {
        const VkApplicationInfo ai = {
            VK_STRUCTURE_TYPE_APPLICATION_INFO,
            NULL,
            "VulkanApplication\0",
            0,
            "VulkanApplication\0",
            VK_MAKE_VERSION(1, 0, 0),
            VK_API_VERSION_1_2,
        };
        const VkInstanceCreateInfo ci = {
            VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            NULL,
            0,
            &ai,
            instLayerNamesCount,
            instLayerNames,
            instExtNamesCount,
            instExtNames,
        };
        CHECK_VK(vkCreateInstance(&ci, NULL, &core->instance), "Vulkanインスタンスの作成に失敗");
    }

    // 物理デバイスを取得する
    //
    // NOTE: 物理デバイスとは、グラフィックスカード、アクセラレータ、DSP等のデバイスのこと。
    //       そもそもVulkanはこれらを扱うためのAPIであるので、これらを選択する必要がある。
    //       今回はグラフィックス目的であるため、グラフィックスカードを選択する。
    //       そのため、本来は、検出された物理デバイスの中から適切に選択しなければならない。
    //       が、今回は初めに発見されたものを選択する。
    {
        uint32_t count = 0;
        CHECK_VK(vkEnumeratePhysicalDevices(core->instance, &count, NULL), "物理デバイス数の取得に失敗");
        VkPhysicalDevice *physDevices = (VkPhysicalDevice *)malloc(sizeof(VkPhysicalDevice) * count);
        CHECK_VK(vkEnumeratePhysicalDevices(core->instance, &count, physDevices), "物理デバイスの列挙に失敗");

        core->physDevice = physDevices[0];
        free(physDevices);

        vkGetPhysicalDeviceMemoryProperties(core->physDevice, &core->physDevMemProps);
    }

    // 物理デバイスのキューファミリーインデックスを取得する
    //
    // NOTE: デバイスはキューに送られた命令を実行していく。
    //       キューにはどの命令に対応しているか種類がある。
    //       同一の種類のキューのまとまりをキューファミリーという。
    //       今回はグラフィックス目的であるため、グラフィックス系の命令に対応しているキューファミリーを選択する。
    int32_t queueFamIndex = -1;
    {
        uint32_t count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(core->physDevice, &count, NULL);
        VkQueueFamilyProperties *props = (VkQueueFamilyProperties *)malloc(sizeof(VkQueueFamilyProperties) * count);
        vkGetPhysicalDeviceQueueFamilyProperties(core->physDevice, &count, props);

        for (int32_t i = 0; i < (int32_t)count; ++i) {
            if ((props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) > 0) {
                queueFamIndex = i;
                break;
            }
        }
        free(props);
        CHECK(queueFamIndex >= 0, "キューファミリーインデックスの取得に失敗");
    }

    // 論理デバイスを作成する
    //
    // NOTE: アプリケーションは論理デバイスを介してデバイスに命令を出す。
    //       見ればわかるが、大体のAPIは論理デバイスを介すことになっている。
    //
    // NOTE: 使用するキューファミリーインデックス・キューの個数・キューのタスク実行の優先度を指定しなければならない。
    //       今回はキュー数は1個、優先度は最高に指定する。
    {
#define QUEUE_FAMILIES_COUNT 1
#define QUEUES_COUNT 1
        const float queuePriors[QUEUES_COUNT] = { 1.0f };
        const VkDeviceQueueCreateInfo queueCIs[QUEUE_FAMILIES_COUNT] = {
            {
                VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                NULL,
                0,
                queueFamIndex,
                QUEUES_COUNT,
                queuePriors,
            },
        };
        const VkDeviceCreateInfo ci = {
            VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            NULL,
            0,
            QUEUE_FAMILIES_COUNT,
            queueCIs,
            devLayerNamesCount,
            devLayerNames,
            devExtNamesCount,
            devExtNames,
            NULL,
        };
        CHECK_VK(vkCreateDevice(core->physDevice, &ci, NULL, &core->device), "論理デバイスの作成に失敗しました");
#undef QUEUES_COUNT
#undef QUEUE_FAMILIES_COUNT
    }

    // キューを取得する
    //
    // NOTE: ここで取得したキューに命令を送ることでデバイスに計算させられる。
    //       今回は、論理デバイス作成時に、1個しかキューを使わないとしたため、1個(0番目)だけ取得する。
    {
        vkGetDeviceQueue(core->device, queueFamIndex, 0, &core->queue);
    }

    // コマンドプールを作成する
    //
    // NOTE: キューに命令を送るためには、いったんコマンドバッファに命令を溜め、コマンドバッファごとキューに送ることになる。
    //       そして、コマンドバッファはコマンドプールから割り当てることになる。
    //       そのため、まずコマンドプールを作成する。
    //
    // NOTE: 割り当てるコマンドバッファには次の二種類ある。
    //         - 使用後すぐに消える
    //         - 再利用できる
    //       今回は上のみを採用する。
    //       コマンドバッファは可変長配列のようなもので、多くコマンドが記録されると複数回リアロケーションが行われる可能性がある。
    //       従って、大体同じ数のコマンドの記録が行われることが予想されるならば、バッファを再利用した方が効率的であると考えられる。
    //       が、今回は設計が複雑になりそうであったので、コマンドバッファを提出するには毎回コマンドバッファを確保するようにする。
    {
        const VkCommandPoolCreateInfo ci = {
            VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            NULL,
            VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
            queueFamIndex,
        };
        CHECK_VK(vkCreateCommandPool(core->device, &ci, NULL, &core->cmdPool), "コマンドプールの作成に失敗");
    }

    return core;

#undef CHECK
#undef CHECK_VK
}

void deleteVulkanAppCore(VulkanAppCore core) {
    vkDeviceWaitIdle(core->device);
    vkDestroyCommandPool(core->device, core->cmdPool, NULL);
    vkDestroyDevice(core->device, NULL);
    vkDestroyInstance(core->instance, NULL);
    free((void *)core);
}

int allocateAndStartCommandBuffer(VulkanAppCore core, VkCommandBuffer *cmdBuffer) {
#define CHECK_VK(p, m) ERROR_IF_WITH((p) != VK_SUCCESS, "createAndStartCommandBuffer()", (m), (p), "", 0)

    // コマンドバッファを確保する
    //
    // NOTE: ここにコマンドを記録していき、キューに提出することでデバイスに計算させる。
    //       今回コマンドプールは「実行されたら破棄される」コマンドバッファしか確保できない設定にしている。
    //       そのため、このコマンドバッファは解放する必要がない。
    {
        const VkCommandBufferAllocateInfo ai = {
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            NULL,
            core->cmdPool,
            VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            1,
        };
        CHECK_VK(vkAllocateCommandBuffers(core->device, &ai, cmdBuffer), "コマンドバッファの確保に失敗");
    }

    // コマンドバッファへの記録を開始する
    //
    // NOTE: VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BITを指定すると、コマンドバッファは実行されたらその内容が消滅する。
    //       逆に、消滅しないようにすれば、全く同じ内容のコマンドバッファを何度も提出できるようになる。
    {
        const VkCommandBufferBeginInfo bi = {
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            NULL,
            VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            NULL,
        };
        CHECK_VK(vkBeginCommandBuffer(*cmdBuffer, &bi), "コマンドバッファへのコマンド記録の開始を失敗");
    }

    return 1;

#undef CHECK_VK
}

int endAndSubmitCommandBuffer(
    VulkanAppCore core,
    VkCommandBuffer cmdBuffer,
    uint32_t waitSemaphoresCount,
    const VkSemaphore *waitSemaphores,
    const VkPipelineStageFlags *waitDstStageMasks,
    uint32_t signalSemaphoresCount,
    const VkSemaphore *signalSemaphores
) {
#define CHECK_VK(p, m) ERROR_IF_WITH((p) != VK_SUCCESS, "submitCommandBuffer()", (m), (p), "", 0)

    // コマンドバッファを終了する
    {
        vkEndCommandBuffer(cmdBuffer);
    }

    // コマンドバッファをキューに提出する
    //
    // NOTE: このコマンドバッファが実行されるまでwaitSemaphoresのシグナルを待機する。
    //       このコマンドバッファが実行されたらsignalSemaphoresをシグナルする。
    {
#define SUBMITS_COUNT 1
        const VkSubmitInfo sis[SUBMITS_COUNT] = {
            {
                VK_STRUCTURE_TYPE_SUBMIT_INFO,
                NULL,
                waitSemaphoresCount,
                waitSemaphores,
                waitDstStageMasks,
                1,
                &cmdBuffer,
                signalSemaphoresCount,
                signalSemaphores,
            },
        };
        CHECK_VK(vkQueueSubmit(core->queue, SUBMITS_COUNT, sis, NULL), "コマンドバッファのエンキューに失敗");
#undef SUBMITS_COUNT
    }

    return 1;

#undef CHECK_VK
}
