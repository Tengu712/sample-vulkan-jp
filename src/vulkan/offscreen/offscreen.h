/// @file offscreen.h
/// @brief オフスクリーン向けのシステムを定義するモジュール

#pragma once

/// @brief Vulkanアプリケーションをオフスクリーンで実行するための関数
///
/// 1フレームだけレンダリングを行い、その結果をout/rendering-result.pngに保存する。
///
/// @param width スクリーン幅
/// @param height スクリーン高
/// @returns 正常終了時に0を返す。
int runOnOffscreen(int width, int height);
