/// @file windows.h
/// @brief Windows向けのシステムを定義するモジュール

#pragma once

/// @brief VulkanアプリケーションをWindowsウィンドウで実行するための関数
///
/// @details
/// 指定されたスクリーンサイズを持つウィンドウを表示し、そこへレンダリングする。
/// レンダリングはプロセスが終了されるまで行われる。
///
/// @param width スクリーン幅
/// @param height スクリーン高
/// @returns 正常終了時に0を返す。
int runOnWindows(int width, int height);
