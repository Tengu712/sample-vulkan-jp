/// @file main.c
/// @brief エントリーモジュール

#include "vulkan/offscreen/offscreen.h"
#include "vulkan/windows/windows.h"

#include <stdio.h>
#include <string.h>

/// @brief エントリーポイント
///
/// コマンドライン引数にプラットフォームを指定する。
/// コマンドライン引数が指定されていない場合はオフスクリーンレンダリングが採用される。
/// 有効なコマンドライン引数は次の通り。
/// - offscreen: オフスクリーンレンダリング
/// - windows: Win32ウィンドウへのレンダリング
///
/// @param argc コマンドライン引数の個数
/// @param argv コマンドライン引数の配列
/// @returns 正常終了時に0を返す。
int main(int argc, char *argv[]) {
    const int width = 640;
    const int height = 480;

    if (argc < 2)  return runOnOffscreen(width, height);
    if (strcmp(argv[1], "offscreen") == 0) return runOnOffscreen(width, height);
    if (strcmp(argv[1], "windows") == 0) return runOnWindows(width, height);
    
    printf("[ error ] main(): 無効な実行形式の指定です: %s\n", argv[1]);
    return 1;
}
