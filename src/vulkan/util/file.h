/// @file file.h
/// @brief 外部ファイルとの提携に関するユーティリティを定義するモジュール

#pragma once

/// @brief バイナリファイルの内容を取得する関数
/// @param path ファイルパス
/// @param size バイナリサイズの格納先。不要ならばNULLを与える
/// @returns 失敗時にNULLを返す。
const char *readBinaryFile(const char *path, long int *size);
