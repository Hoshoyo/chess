#include "os.h"
#include <stdio.h>
#include <string.h>
#include "light_array.h"
#include <Windows.h>

char*
os_extensionless_filename(const char* filename)
{
    s32 length = (s32)strlen(filename);
    s32 newlen = 0;
    for (s32 i = length - 1; i >= 0; --i)
    {
        if (filename[i] == '.')
        {
            newlen = i;
            break;
        }
    }
    void* mem = calloc(1, newlen + 1);

    memcpy(mem, filename, newlen);
    return (char*)mem;
}

const char*
os_filename_from_path(const char* path)
{
    size_t len = strlen(path);
    size_t i = len - 1;
    for (; i >= 0; --i)
    {
#if defined (_WIN32) || defined(_WIN64)
        if (path[i] == '\\') {
            i++;
            break;
        }
#endif
        if (path[i] == '/')
        {
            i++;
            break;
        }
    }
    return path + i;
}

bool
os_file_exists(const char* path)
{
    FILE* f = fopen(path, "r");
    if (f)
    {
        fclose(f);
        return true;
    }
    return false;
}

char*
os_file_read(const char* path, s32* file_length, void* (*allocator)(size_t))
{
    FILE* file;
    s8* buffer;
    s32 len;

    file = fopen(path, "rb");
    if (file == NULL)
    {
        printf("os_file_read: could not open file %s", path);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    len = ftell(file);
    rewind(file);

    buffer = (s8*)allocator((len + 1) * sizeof(char));
    if (fread(buffer, 1, len, file) != len)
    {
        printf("os_file_read: could not read file %s", path);
        fclose(file);
        free(buffer);
        return NULL;
    }

    fclose(file);

    buffer[len] = '\0';

    if (file_length)
        *file_length = len;

    return buffer;
}

void
os_file_free(char* buf)
{
    free(buf);
}

s32
os_file_write_new(const char* path, const char* buf, s32 size)
{
    FILE* fptr;
    fptr = fopen(path, "wb+");
    if (fptr == NULL)
    {
        printf("os_file_write: could not open file %s", path);
        return -1;
    }

    if (fwrite(buf, 1, size, fptr) != size)
    {
        printf("os_file_write: could not write to file %s", path);
        return -1;
    }

    fclose(fptr);

    return 0;
}

s32
os_file_write(const char* path, const char* buf, s32 size)
{
    FILE* fptr;
    fptr = fopen(path, "wb");
    if (fptr == NULL)
    {
        printf("os_file_write: could not open file %s", path);
        return -1;
    }

    if (fwrite(buf, 1, size, fptr) != size)
    {
        printf("os_file_write: could not write to file %s", path);
        return -1;
    }

    fclose(fptr);

    return 0;
}

static r64 perf_frequency;
static void
os_set_query_frequency()
{
    LARGE_INTEGER li = { 0 };
    QueryPerformanceFrequency(&li);
    perf_frequency = (r64)(li.QuadPart);
}

r64
os_time_us()
{
    static initialized = false;
    if (!initialized)
    {
        os_set_query_frequency();
        initialized = true;
    }

    LARGE_INTEGER li = { 0 };
    QueryPerformanceCounter(&li);
    return ((r64)(li.QuadPart) / perf_frequency) * 1000000.0;
}

void
os_usleep(u64 microseconds)
{
    if (microseconds >= 1000)
        Sleep((u32)(microseconds / 1000));
}

u64
os_file_last_modified(const char* filename)
{
    WIN32_FIND_DATA file_info;
    HANDLE handle = FindFirstFileA(filename, &file_info);
    FILETIME last_written = file_info.ftLastWriteTime;
    FindClose(handle);
    return *(u64*)&last_written;
}

u64
os_timestamp()
{
    return 0;
}

void
os_toggle_fullscreen()
{
    static HWND g_win32_window = 0;
    static WINDOWPLACEMENT g_win32_window_position = { sizeof(g_win32_window_position) };
    if (!g_win32_window)
    {
        g_win32_window = GetActiveWindow();
    }

    DWORD style = GetWindowLong(g_win32_window, GWL_STYLE);
    if (style & WS_OVERLAPPEDWINDOW)
    {
        MONITORINFO mi = { sizeof(mi) };
        if (GetWindowPlacement(g_win32_window, &g_win32_window_position) &&
            GetMonitorInfo(MonitorFromWindow(g_win32_window, MONITOR_DEFAULTTOPRIMARY), &mi))
        {
            SetWindowLong(g_win32_window, GWL_STYLE, style & ~WS_OVERLAPPEDWINDOW);
            SetWindowPos(g_win32_window, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right - mi.rcMonitor.left,
                mi.rcMonitor.bottom - mi.rcMonitor.top, SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    }
    else
    {
        SetWindowLong(g_win32_window, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(g_win32_window, &g_win32_window_position);
        SetWindowPos(g_win32_window, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}

int
os_warning(const char* title, const char* fmt, ...)
{
    int length = 0;
    va_list args;
    va_start(args, fmt);

    char buffer[1024] = { 0 };
    length = vsprintf(buffer, fmt, args);
    int status = MessageBoxA(0, buffer, title, MB_OKCANCEL | MB_ICONWARNING);
    switch (status)
    {
    case IDCANCEL: length = -1; break;
    default:       length = 0; break;
    }

    va_end(args);

    return length;
}

bool
os_copy_file(const char* filename, const char* copy_name)
{
    s32 length;
    char* data = os_file_read(filename, &length, malloc);
    if (!data)
        return false;

    if (os_file_write_new(copy_name, data, length) == -1)
        return false;
    return true;
}