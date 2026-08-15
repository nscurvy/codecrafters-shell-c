//
// Created by nkinder on 8/15/26.
//
#pragma once

#if defined(__clang__) && __has_feature(nullability)
#define NULLABLE _Nullable
#define NONNULL _Nonnull
#define ASSUME_NONNULL_BEGIN _Pragma("clang assume_nonnull begin")
#define ASSUME_NONNULL_END _Pragma("clang assume_nonnull end")
#else
#define NULLABLE
#define NONNULL
#define ASSUME_NONNULL_BEGIN
#define ASSUME_NONNULL_END
#endif

#if defined(__GNUC__) && !defined(__clang__)
#define GCC_NONNULL(...) __attribute__((nonnull(__VA_ARGS__)))
#else
#define GCC_NONNULL(...)
#endif