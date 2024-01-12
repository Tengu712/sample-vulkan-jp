/// @file error.h
/// @brief エラーチェックやエラーログのためのマクロを定義するモジュール
/// @warning printf()関数を用いるため、<stdio.h>をincludeするといい。

#pragma once

#define ERROR_LOG_WITH(f, m, s) printf("[ error ] %s: %s: %d\n", (f), (m), (s));
#define ERROR_LOG(f, m)         printf("[ error ] %s: %s\n", (f), (m));

#define ERROR_IF_WITH(condExpr, funName, message, status, destruction, retValue) \
    if ((condExpr)) {                                                            \
        ERROR_LOG_WITH((funName), (message), (status));                          \
        destruction;                                                             \
        return (retValue);                                                       \
    }

#define ERROR_IF(condExpr, funName, message, destruction, retValue) \
    if ((condExpr)) {                                               \
        ERROR_LOG((funName), (message));                            \
        destruction;                                                \
        return (retValue);                                          \
    }
