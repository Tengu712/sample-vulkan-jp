#include "windows.h"

#ifdef _WIN64

// このマクロを<vulkan/vulkan.h>のinclude前に定義しないとWin32ウィンドウに関するAPIが使えない。
# define VK_USE_PLATFORM_WIN32_KHR

# include "../../vulkan/core.h"
# include "../../vulkan/presentation.h"
# include "../../vulkan/rendering.h"
# include "../../vulkan/util/error.h"

# include <stdio.h>
# include <vulkan/vulkan.h>
# include <Windows.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK WindowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProc(window, message, wParam, lParam);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// VulkanアプリケーションのうちWindows依存オブジェクトを持つ構造体
typedef struct VulkanAppWindows_t {
    HMODULE instance;
    HWND window;
    VkSurfaceKHR surface;
} *VulkanAppWindows;

void deleteVulkanAppWindows(const VulkanAppCore core, VulkanAppWindows windows) {
    if (windows == NULL) {
        return;
    }
    vkDeviceWaitIdle(core->device);
    if (windows->surface != NULL) vkDestroySurfaceKHR(core->instance, windows->surface, NULL);
    if (windows->window != NULL) DestroyWindow(windows->window);
    UnregisterClassW(L"SampleVulkanJP\0", windows->instance);
    free((void *)windows);
}

VulkanAppWindows createVulkanAppWindows(const VulkanAppCore core, int width, int height) {
#define CHECK_VK(p, m) ERROR_IF_WITH((p) != VK_SUCCESS, "createVulkanAppWindows()", (m), (p), deleteVulkanAppWindows(core, windows), NULL)
#define CHECK(p, m)    ERROR_IF     (!(p),              "createVulkanAppWindows()", (m),      deleteVulkanAppWindows(core, windows), NULL)

    const VulkanAppWindows windows = (VulkanAppWindows)malloc(sizeof(struct VulkanAppWindows_t));
    CHECK(windows != NULL, "VulkanAppWindowsのメモリ確保に失敗");

    // インスタンスハンドルを取得する
    {
        windows->instance = GetModuleHandleW(NULL);
        CHECK(windows->instance != NULL, "インスタンスハンドルの取得に失敗");
    }

    // ウィンドウを作成する
    {
        const WNDCLASSEXW wcex = {
            sizeof(WNDCLASSEXW),
            CS_CLASSDC,
            WindowProcedure,
            0,
            0,
            windows->instance,
            NULL,
            NULL,
            NULL,
            NULL,
            L"SampleVulkanJP\0",
            NULL,
        };
        CHECK(RegisterClassExW(&wcex) != 0, "ウィンドウクラスの登録に失敗");

        RECT rect = { 0, 0, width, height };
        const DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
        AdjustWindowRect(&rect, style, 0);

        windows->window = CreateWindowExW(
            0,
            L"SampleVulkanJP\0",
            L"SampleVulkanJP\0",
            style,
            0,
            0,
            rect.right - rect.left,
            rect.bottom - rect.top,
            NULL,
            NULL,
            windows->instance,
            NULL
        );
        CHECK(windows->window != NULL, "ウィンドウの作成に失敗");

        ShowWindow(windows->window, SW_SHOWDEFAULT);
        UpdateWindow(windows->window);
    }

    // サーフェスを作成する
    {
        const VkWin32SurfaceCreateInfoKHR ci = {
            VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
            NULL,
            0,
            windows->instance,
            windows->window,
        };
        CHECK_VK(vkCreateWin32SurfaceKHR(core->instance, &ci, NULL, &windows->surface), "サーフェスの作成に失敗");
    }

    return windows;

# undef CHECK
# undef CHECK_VK
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Windows向けVulkanアプリケーションで必要なモジュールを持つ構造体
typedef struct ModulesForWindows_t {
    VulkanAppCore core;
    VulkanAppWindows windows;
    VulkanAppPresentation presenter;
    VulkanAppRendering renderer;
} ModulesForWindows;

void deleteModulesForWindows(const ModulesForWindows *mods) {
    if (mods == NULL) {
        return;
    }
    if (mods->renderer != NULL) deleteVulkanAppRendering(mods->core, mods->renderer);
    if (mods->presenter != NULL) deleteVulkanAppPresentation(mods->core, mods->presenter);
    if (mods->windows != NULL) deleteVulkanAppWindows(mods->core, mods->windows);
    if (mods->core != NULL) deleteVulkanAppCore(mods->core);
}

int runOnWindows(int width, int height) {
# define CHECK(p, m) ERROR_IF(!(p), "runOnWindows()", (m), deleteModulesForWindows(&mods), 1)

    ModulesForWindows mods = {
        NULL,
        NULL,
        NULL
    };

    // 主要オブジェクトを作成する
    const uint32_t instLayerNamesCount = 1;
    const char *instLayerNames[] = { "VK_LAYER_KHRONOS_validation" };
    const uint32_t instExtNamesCount = 2;
    const char *instExtNames[] = { "VK_KHR_surface", "VK_KHR_win32_surface" };
    const uint32_t devLayerNamesCount = 0;
    const char *devLayerNames[] = { "" };
    const uint32_t devExtNamesCount = 1;
    const char *devExtNames[] = { "VK_KHR_swapchain" };
    mods.core = createVulkanAppCore(
        instLayerNamesCount,
        instLayerNames,
        instExtNamesCount,
        instExtNames,
        devLayerNamesCount,
        devLayerNames,
        devExtNamesCount,
        devExtNames
    );
    CHECK(mods.core != NULL, "主要オブジェクトの作成に失敗");

    // Windows依存オブジェクトを作成する
    mods.windows = createVulkanAppWindows(mods.core, width, height);
    CHECK(mods.windows != NULL, "Windows依存オブジェクトの作成に失敗");

    // プレゼンテーションオブジェクトの作成に失敗
    mods.presenter = createVulkanAppPresentation(mods.core, mods.windows->surface);
    CHECK(mods.presenter != NULL, "プレゼンテーションオブジェクトの作成に失敗");

    // レンダリングオブジェクトを作成する
    mods.renderer = createVulkanAppRendering(
        mods.core,
        mods.presenter->imageViews,
        mods.presenter->imagesCount,
        mods.presenter->width,
        mods.presenter->height,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    );
    CHECK(mods.renderer != NULL, "レンダリングオブジェクトの作成に失敗");

    // メインループ
    MSG message;
    while (1) {
        // ウィンドウメッセージを処理する
        if (PeekMessageW(&message, NULL, 0, 0, PM_REMOVE) != 0) {
            if (message.message == WM_QUIT) {
                break;
            }
            TranslateMessage(&message);
            DispatchMessageW(&message);
            continue;
        }
        // 以降デッドタイム
        // 描画可能な次のイメージのインデックスを取得する
        if (!acquireNextImageIndex(mods.core, mods.presenter)) {
            continue;
        }
        // 描画する
        const uint32_t waitSemaphoresCount = 1;
        const VkSemaphore waitSemaphores[] = { mods.presenter->waitForImageEnabledSemaphore };
        const VkPipelineStageFlags waitDstStageMasks[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        const uint32_t signalSemaphoresCount = 1;
        const VkSemaphore signalSemaphores[] = { mods.presenter->waitForRenderingSemaphore };
        if (!render(
            mods.core,
            mods.renderer,
            mods.presenter->imageIndex,
            0,
            0,
            mods.presenter->width,
            mods.presenter->height,
            waitSemaphoresCount,
            waitSemaphores,
            waitDstStageMasks,
            signalSemaphoresCount,
            signalSemaphores
        )) {
            continue;
        }
        // プレゼンテーションを行う
        if (!present(mods.core, mods.presenter)) {
            continue;
        }
    }

    deleteModulesForWindows(&mods);
    return 0;

# undef CHECK
}

#else

int runOnWindows(void) {
    printf("[ info ] runOnWindows(): Windows向けに対応していません\n");
    return 1;
}

#endif
