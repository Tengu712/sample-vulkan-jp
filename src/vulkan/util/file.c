#include "file.h"

// fopen()関数の使用に対してwarningを出さないためにこのマクロを定義する
#define _CRT_SECURE_NO_WARNINGS

#include "error.h"

#include <stdio.h>
#include <stdlib.h>

const char *readBinaryFile(const char *path, long int *size) {
#define CHECK(p, m, d) ERROR_IF(!(p), "readBinaryFile()", (m), d, NULL)

    // ファイルを開く
    FILE *file;
    {
        file = fopen(path, "rb");
        CHECK(file != NULL, "ファイルのオープンに失敗", {});
    }

    // ファイルサイズを取得する
    fpos_t pos = 0;
    {
        CHECK(fseek(file, 0L, SEEK_END) == 0, "ファイルの後尾へのシークに失敗", fclose(file));
        CHECK(fgetpos(file, &pos) == 0, "ファイルサイズの取得に失敗", fclose(file));
        CHECK(fseek(file, 0L, SEEK_SET) == 0, "ファイルの先頭へのシークに失敗", fclose(file));
    }

    // 内容を読み込む
    const char *binary;
    {
        binary = (const char *)malloc(sizeof(const char) * pos);
        CHECK(binary != NULL, "メモリの確保に失敗", fclose(file));

        CHECK(
            fread((void *)binary, sizeof(const char), pos, file) == (size_t)pos,
            "ファイルの読込みに失敗",
            { free((void *)binary); fclose(file); }
        );
    }

    // ファイルを閉じる
    {
        fclose(file);
    }

    // サイズを格納する
    if (size != NULL) {
        *size = (long int)pos;
    }
    
    return binary;

#undef CHECK
}
