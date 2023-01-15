#pragma once
#include <stdint.h>

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef float r32;
typedef double r64;
typedef int bool;

#define true 1
#define false 0

#define RESULT_OK 0
#define RESULT_ERROR (-1)

#define LOG(...) printf(__VA_ARGS__)
#define LOG_INFO(...) printf(__VA_ARGS__)

#define ARRAY_LENGTH(A) (sizeof(A)/sizeof(*(A)))