#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

typedef long long s64;
typedef int s32;
typedef short s16;
typedef char s8;
typedef unsigned long long u64;
typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;
typedef double r64;
typedef float r32;

typedef int bool;
#define true 1
#define false 0

// Files
bool        os_file_exists(const char* path);
void        os_file_free(char* buf);
char*       os_file_read(const char* path, s32* file_length, void* (*allocator)(size_t));
s32         os_file_write(const char* path, const char* buf, s32 size);
s32         os_file_write_new(const char* path, const char* buf, s32 size);
char*       os_extensionless_filename(const char* filename);
const char* os_filename_from_path(const char* path);
u64         os_file_last_modified(const char* filename);
bool        os_copy_file(const char* filename, const char* copy_name);

// Timing
r64         os_time_us();
u64         os_timestamp();
void        os_usleep(u64 microseconds);

// Window
void        os_toggle_fullscreen();
int         os_warning(const char* title, const char* fmt, ...);