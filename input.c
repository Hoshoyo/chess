#include <stdio.h>
#include <windowsx.h>
#include "input.h"

#define MAX_KEYS 1024
#define MAX_EVENTS 32
#define MAX_MOUSE_BUTTONS 8

#define MOUSE_ACTION_RELEASE 0
#define MOUSE_ACTION_PRESS 1

typedef struct {
    HWND window;

    // event queue is a circular array
    Hinp_Event events[MAX_EVENTS];
    int        event_index;
    int        event_count;

    // Mouse info
    int mouse_x, mouse_y;
    int mouse_delta_x, mouse_delta_y;
    int mouse_button_released[MAX_MOUSE_BUTTONS];
    int mouse_button_pressed[MAX_MOUSE_BUTTONS];
    int mouse_button_was_released[MAX_MOUSE_BUTTONS];
    int mouse_button_was_pressed[MAX_MOUSE_BUTTONS];
    int mouse_buttons[MAX_MOUSE_BUTTONS];
    float mouse_wheel;

    // Keyboard info
    int key_state[MAX_KEYS];

    // Window info
    int window_width, window_height;
    int window_x, window_y;
} Hinp_Ctx;
static Hinp_Ctx ctx;

static void
event_push(Hinp_Event* ev)
{
    int index = (ctx.event_index + ctx.event_count) % MAX_EVENTS;
    ctx.events[index] = *ev;
    if (ctx.event_count == MAX_EVENTS)
        ctx.event_index = (ctx.event_index + 1) % MAX_EVENTS;
    else
        ctx.event_count++;
}

void
hinp_event_clear()
{
    ctx.event_index = 0;
    ctx.event_count = 0;
}

void
hinp_clear()
{
    for (int i = 0; i < sizeof(ctx.mouse_button_pressed) / sizeof(*ctx.mouse_button_pressed); ++i)
    {
        ctx.mouse_button_was_pressed[i] = 0;
        ctx.mouse_button_was_released[i] = 0;
    }

    ctx.mouse_wheel = 0.0f;
    ctx.mouse_delta_x = 0;
    ctx.mouse_delta_y = 0;
    memset(ctx.mouse_button_pressed, 0, sizeof(ctx.mouse_button_pressed));
    memset(ctx.mouse_button_released, 0, sizeof(ctx.mouse_button_released));
}

int
hinp_event_next(Hinp_Event* ev)
{
    if (ctx.event_count == 0) return 0;

    *ev = ctx.events[ctx.event_index];
    ctx.event_index = (ctx.event_index + 1) % MAX_EVENTS;
    ctx.event_count--;
    return ctx.event_count + 1;
}

/* UTF8 */

static int
unicode_to_utf8(uint32_t unicode, uint8_t* buffer)
{
    char* start = buffer;
    char* result = buffer;
    {
        if (unicode <= 0x7f)
        {
            *result++ = (uint8_t)unicode;
        }
        else if (unicode >= 0x80 && unicode <= 0x7ff)
        {
            uint8_t b1 = 0xc0 | (unicode >> 6);
            uint8_t b2 = 0x80 | ((unicode & 0x3f) | 0x30000000);
            *result++ = b1;
            *result++ = b2;
        }
        else if (unicode >= 0x800 && unicode <= 0xffff)
        {
            uint8_t b1 = 0xe0 | (unicode >> 12);
            uint8_t b2 = 0x80 | (((unicode >> 6) & 0x3f) | 0x30000000);
            uint8_t b3 = 0x80 | ((unicode & 0x3f) | 0x30000000);
            *result++ = b1;
            *result++ = b2;
            *result++ = b3;
        }
        else if (unicode >= 0x00010000 && unicode <= 0x001fffff)
        {
            uint8_t b1 = 0xf0 | (unicode >> 18);
            uint8_t b2 = 0x80 | (((unicode >> 12) & 0x3f) | 0x30000000);
            uint8_t b3 = 0x80 | (((unicode >> 6) & 0x3f) | 0x30000000);
            uint8_t b4 = 0x80 | ((unicode & 0x3f) | 0x30000000);
            *result++ = b1;
            *result++ = b2;
            *result++ = b3;
            *result++ = b4;
        }
    }
    return (int)(result - start);
}


/* -------------------------------------------- */

#if 0
static void
cursor_enter_callback(GLFWwindow* window, int entered)
{
    Hinp_Event ev = {
        .type = (entered) ? HINP_EVENT_MOUSE_ENTER : HINP_EVENT_MOUSE_LEAVE,
        .mouse.x = ctx.mouse_x,
        .mouse.y = ctx.mouse_y,
    };
    event_push(&ev);
}
#endif

int
hinp_mouse_button_released(int button)
{
    if (button >= MAX_MOUSE_BUTTONS) return 0;
    return(ctx.mouse_button_released[button]);
}

int
hinp_mouse_button_pressed(int button)
{
    if (button >= MAX_MOUSE_BUTTONS) return 0;
    return(ctx.mouse_button_pressed[button]);
}

int
hinp_mouse_button_was_released(int button)
{
    if (button >= MAX_MOUSE_BUTTONS) return 0;
    return(ctx.mouse_button_was_released[button]);
}

int
hinp_mouse_button_was_pressed(int button)
{
    if (button >= MAX_MOUSE_BUTTONS) return 0;
    return(ctx.mouse_button_was_pressed[button]);
}

int
hinp_mouse_button_down(int button)
{
    if (button >= MAX_MOUSE_BUTTONS) return 0;
    return(ctx.mouse_buttons[button]);
}

float
hinp_mouse_wheel()
{
    return ctx.mouse_wheel * 0.1f;
}

void
hinp_mouse_delta(float* x, float* y)
{
    *x = (float)ctx.mouse_delta_x;
    *y = (float)ctx.mouse_delta_y;
}

static void
process_mouse_button(int button, int action, int x, int y, Hinp_Mods mods)
{
    Hinp_Event ev = {
        .type = HINP_EVENT_MOUSE_CLICK,
        .mouse.x = x,
        .mouse.y = y,
        .mouse.action = action,
        .mouse.mods = mods,
        .mouse.button = button,
    };
    event_push(&ev);

    if (action == MOUSE_ACTION_RELEASE)
    {
        if (!ctx.mouse_button_released[button])
            ctx.mouse_button_was_released[button] = 1;
        ctx.mouse_button_released[button] = 1;
        ctx.mouse_buttons[button] = 0;  // state
    }
    else if (action == MOUSE_ACTION_PRESS)
    {
        if (!ctx.mouse_button_pressed[button])
            ctx.mouse_button_was_pressed[button] = 1;
        ctx.mouse_button_pressed[button] = 1;
        ctx.mouse_buttons[button] = 1;  // state
    }
}

static void
process_mouse_wheel(int offset, int x, int y, Hinp_Mods mods)
{
    Hinp_Event ev = {
        .type = HINP_EVENT_MOUSE_SCROLL,
        .mouse.x = x,
        .mouse.y = y,
        .mouse.scroll_delta_x = offset,
        .mouse.scroll_delta_y = offset,
        .mouse.mods = mods
    };

    ctx.mouse_wheel = (offset / 120.0f);

    event_push(&ev);
}

static void
process_cursor_position(int xpos, int ypos)
{
    Hinp_Event ev = {
        .type = HINP_EVENT_MOUSE_MOVE,
        .mouse.x = xpos,
        .mouse.y = ypos,
        .mouse.px = ctx.mouse_x,
        .mouse.py = ctx.mouse_y,
        .mouse.deltax = xpos - ctx.mouse_x,
        .mouse.deltay = ypos - ctx.mouse_y,
    };
    ctx.mouse_x = xpos;
    ctx.mouse_y = ypos;

    ctx.mouse_delta_x += ev.mouse.deltax;
    ctx.mouse_delta_y += ev.mouse.deltay;

    event_push(&ev);
}

static void
process_user_event(MSG msg)
{
    SYSTEMTIME systime = { 0 };
    GetLocalTime(&systime);

    Hinp_Event ev = {
        .type = HINP_EVENT_USER,
        .user.msg = msg.message,
        .user.lparam = msg.lParam,
        .user.wparam = msg.wParam,
        .user.time = msg.time,
        .user.systime = systime,
    };
    event_push(&ev);
}

static void
process_keypress(int action, int key, Hinp_Mods mods, int scancode)
{
    Hinp_Event ev = {
        .type = HINP_EVENT_KEYBOARD,
        .keyboard.key = key,
        .keyboard.action = action,
        .keyboard.mods = mods,
        .keyboard.scancode = scancode
    };
    event_push(&ev);
    if (key >= 0 && key < 1024)
        ctx.key_state[key] = action;
}

static void
process_drop_file(const char* buffer, int x, int y)
{
    Hinp_Event ev = {
        .type = HINP_EVENT_DROP_FILE,
        .drop_file.mouse_x = x,
        .drop_file.mouse_y = y,
    };
    memcpy(ev.drop_file.filepath, buffer, MAX_PATH);
    event_push(&ev);
}

static void
process_char(unsigned int codepoint, Hinp_Mods mods, int scancode)
{
    Hinp_Event ev = {
        .type = HINP_EVENT_CHARACTER,
        .character.unicode = codepoint,
    };
    ev.character.utf8_num_bytes = (uint8_t)unicode_to_utf8(codepoint, ev.character.utf8);
    event_push(&ev);
}

static void
process_window_resize(int width, int height)
{
    Hinp_Event ev = {
        .type = HINP_EVENT_WINDOW_RESIZE,
        .window.x = ctx.window_x,
        .window.y = ctx.window_y,
        .window.width = width,
        .window.height = height,
    };
    ctx.window_width = width;
    ctx.window_height = height;
    event_push(&ev);
}

static void
process_window_move(int xpos, int ypos)
{
    Hinp_Event ev = {
        .type = HINP_EVENT_WINDOW_MOVE,
        .window.x = xpos,
        .window.y = ypos,
        .window.width = ctx.window_width,
        .window.height = ctx.window_height,
    };
    ctx.window_x = xpos;
    ctx.window_y = ypos;
    event_push(&ev);
}

int
hinp_init(HWND window)
{
    ctx.window = window;

    // initialize mouse
    POINT pt = { 0 };
    GetCursorPos(&pt);
    ctx.mouse_x = pt.x;
    ctx.mouse_y = pt.y;

    // initialize window
    RECT rect = { 0 };
    GetClientRect(window, &rect);
    int ww = 0, wh = 0;
    ctx.window_width = (rect.right - rect.left);
    ctx.window_height = (rect.top - rect.bottom);

    return 0;
}

int
hinp_key_state(int key)
{
    if (key >= 0 && key <= sizeof(ctx.key_state))
    {
        return ctx.key_state[key];
    }
    return 0;
}

int
hinp_mouse_position(int* x, int* y)
{
    *x = ctx.mouse_x;
    *y = ctx.window_height - ctx.mouse_y;
    return 0;
}

int
hinp_window_size(int* w, int* h)
{
    *w = ctx.window_width;
    *h = ctx.window_height;
    return 0;
}

static Hinp_Mods
hinp_get_mods()
{
    Hinp_Mods mods = ((GetAsyncKeyState(VK_MENU)) ? HINP_MOD_ALT : 0);
    mods |= ((GetAsyncKeyState(VK_CONTROL)) ? HINP_MOD_CTRL : 0);
    mods |= ((GetAsyncKeyState(VK_SHIFT)) ? HINP_MOD_CTRL : 0);
    mods |= ((GetAsyncKeyState(VK_LWIN) || GetAsyncKeyState(VK_RWIN)) ? HINP_MOD_CTRL : 0);
    return mods;
}

int
hinp_clipboard_set(const char* text, int length)
{
    length++;

    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, length);

    char* dst = (char*)GlobalLock(hMem);
    memcpy(dst, text, length-1);
    GlobalUnlock(hMem);

    if (!OpenClipboard(0))
        return -1;
    EmptyClipboard();

    SetClipboardData(CF_TEXT, hMem);

    CloseClipboard();

    return 0;
}

LRESULT WINAPI
hinp_process_win_msg(HWND window, MSG msg, int translate_dispatch)
{
    __try
    {
        switch (msg.message)
        {
            case WM_MOUSELEAVE: {
            } break;
            case WM_MOUSEMOVE: {
                process_cursor_position(GET_X_LPARAM(msg.lParam), GET_Y_LPARAM(msg.lParam));
            } break;
            case WM_KEYDOWN: {
                process_keypress(1, (int)msg.wParam, hinp_get_mods(), (msg.lParam >> 16) & 0xff);
            } break;
            case WM_KEYUP: {
                process_keypress(0, (int)msg.wParam, hinp_get_mods(), (msg.lParam >> 16) & 0xff);
            } break;
            case WM_LBUTTONDOWN: {
                process_mouse_button(HINP_MOUSE_LBUTTON, MOUSE_ACTION_PRESS, GET_X_LPARAM(msg.lParam), GET_Y_LPARAM(msg.lParam), hinp_get_mods());
            } break;
            case WM_RBUTTONDOWN: {
                process_mouse_button(HINP_MOUSE_RBUTTON, MOUSE_ACTION_PRESS, GET_X_LPARAM(msg.lParam), GET_Y_LPARAM(msg.lParam), hinp_get_mods());
            } break;
            case WM_MBUTTONDOWN: {
                process_mouse_button(HINP_MOUSE_MBUTTON, MOUSE_ACTION_PRESS, GET_X_LPARAM(msg.lParam), GET_Y_LPARAM(msg.lParam), hinp_get_mods());
            } break;
            case WM_LBUTTONUP: {
                process_mouse_button(HINP_MOUSE_LBUTTON, MOUSE_ACTION_RELEASE, GET_X_LPARAM(msg.lParam), GET_Y_LPARAM(msg.lParam), hinp_get_mods());
            } break;
            case WM_RBUTTONUP: {
                process_mouse_button(HINP_MOUSE_RBUTTON, MOUSE_ACTION_RELEASE, GET_X_LPARAM(msg.lParam), GET_Y_LPARAM(msg.lParam), hinp_get_mods());
            } break;
            case WM_MBUTTONUP: {
                process_mouse_button(HINP_MOUSE_MBUTTON, MOUSE_ACTION_RELEASE, GET_X_LPARAM(msg.lParam), GET_Y_LPARAM(msg.lParam), hinp_get_mods());
            } break;
            case WM_MOUSEWHEEL: {
                process_mouse_wheel((int)(short)HIWORD(msg.wParam), GET_X_LPARAM(msg.lParam), GET_Y_LPARAM(msg.lParam), hinp_get_mods());
            } break;
            case WM_MOVE: {
                process_window_move(LOWORD(msg.lParam), HIWORD(msg.lParam));
            } break;
            case WM_SIZE: {
                process_window_resize(LOWORD(msg.lParam), HIWORD(msg.lParam));
            } break;
            case WM_CHAR: {
                process_char((unsigned int)msg.wParam, hinp_get_mods(), (msg.lParam >> 16) & 0xff);
            } break;
            case WM_CLOSE:
            case WM_QUIT: {
                exit(0);
            } break;
            case WM_DROPFILES: {
                char buffer[MAX_PATH];
                HDROP hDrop = (HDROP)msg.wParam;
                UINT ret = DragQueryFileA(hDrop, 0, buffer, 512);
                POINT mouse_loc;
                DragQueryPoint(hDrop, &mouse_loc);
                DragFinish(hDrop);
                process_drop_file(buffer, mouse_loc.x, mouse_loc.y);
            } break;
            default: {
                if (msg.message > WM_USER)
                {
                    process_user_event(msg);
                }
                else if (translate_dispatch)
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
                else
                {
                    return DefWindowProc(window, msg.message, msg.wParam, msg.lParam);
                }
            } break;
        }
    }
    __except(1)
    {
        printf("Exception in message %d\n", msg.message);
    }
    return 0;
}

// Clipboard support, read this!
// https://docs.microsoft.com/en-us/windows/win32/dataxchg/using-the-clipboard

LRESULT CALLBACK
hinp_callback(HWND window, UINT msg, WPARAM wparam, LPARAM lparam)
{
    MSG m = { .message = msg, .hwnd = window, .wParam = wparam, .lParam = lparam };
    return hinp_process_win_msg(window, m, 0);
}