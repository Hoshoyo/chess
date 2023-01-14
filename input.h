#pragma once
#include <stdint.h>
#include <windows.h>

#define HINP_MOUSE_LBUTTON 0
#define HINP_MOUSE_RBUTTON 1
#define HINP_MOUSE_MBUTTON 2

#define HINP_KEY_BACKSPACE VK_BACK
#define HINP_KEY_LEFT      VK_LEFT
#define HINP_KEY_RIGHT     VK_RIGHT
#define HINP_KEY_DELETE    VK_DELETE
#define HINP_KEY_END       VK_END
#define HINP_KEY_HOME      VK_HOME
#define HINP_KEY_ENTER     VK_RETURN
#define HINP_KEY_ESCAPE    VK_ESCAPE
#define HINP_KEY_TAB       VK_TAB
#define HINP_KEY_PAGE_DOWN VK_NEXT
#define HINP_KEY_PAGE_UP   VK_PRIOR

typedef enum {
    HINP_EVENT_NONE = 0,

    // mouse events
    HINP_EVENT_MOUSE_MOVE,
    HINP_EVENT_MOUSE_CLICK,
    HINP_EVENT_MOUSE_SCROLL,
    HINP_EVENT_MOUSE_LEAVE,
    HINP_EVENT_MOUSE_ENTER,

    // key events
    HINP_EVENT_KEYBOARD,

    // char events
    HINP_EVENT_CHARACTER,

    // window events
    HINP_EVENT_WINDOW_RESIZE,
    HINP_EVENT_WINDOW_MOVE,

    // drop file event
    HINP_EVENT_DROP_FILE,

    HINP_EVENT_USER,
} Hinp_Event_Type;

typedef enum {
    HINP_MOD_SHIFT = (1 << 0),
    HINP_MOD_CTRL = (1 << 1),
    HINP_MOD_ALT = (1 << 2),
    HINP_MOD_SUPER = (1 << 3),
} Hinp_Mods;

/* Window */
typedef struct {
    int x, y;
    int width, height;
} Hinp_Window;

/* Mouse */
typedef struct {
    int x, y;             // current x and y coords
    int px, py;           // previous x and y
    int deltax, deltay;   // difference from last frame position

    int scroll_delta_x;   // < 0 scroll down, > 0 scroll up, 0 = no scroll
    int scroll_delta_y;   // < 0 scroll down, > 0 scroll up, 0 = no scroll

    int       button;       // 0 Left, 1 Right, 2 Middle
    int       action;       // 1 down, 0 up
    Hinp_Mods mods;
} Hinp_Mouse;

/* Keyboard */
typedef struct {
    uint8_t  utf8[4];
    uint8_t  utf8_num_bytes;
    uint32_t unicode;
} Hinp_Character;

typedef struct {
    int       key;
    int       scancode;
    int       action;   /* 1 press, 0 release, 2 repeat */
    Hinp_Mods mods;
} Hinp_Event_Keyboard;

typedef struct {
    char filepath[MAX_PATH];
    int mouse_x;
    int mouse_y;
} Hinp_DropFile;

typedef struct {
    uint32_t msg;
    WPARAM   wparam;
    LPARAM   lparam;
    DWORD    time;
    SYSTEMTIME systime;
} Hinp_User;

typedef struct {
    Hinp_Event_Type     type;
    union {
        Hinp_Event_Keyboard keyboard;
        Hinp_Mouse          mouse;
        Hinp_Window         window;
        Hinp_Character      character;
        Hinp_DropFile       drop_file;
        Hinp_User           user;
    };
} Hinp_Event;

int  hinp_init(HWND window);
int  hinp_event_next(Hinp_Event* ev);
void hinp_event_clear();
void hinp_clear();

int hinp_window_size(int* w, int* h);

int hinp_key_state(int key);

// mouse
int   hinp_mouse_position(int* x, int* y);
int   hinp_mouse_button_released(int button);
int   hinp_mouse_button_pressed(int button);
int   hinp_mouse_button_was_pressed(int button);
int   hinp_mouse_button_was_released(int button);
int   hinp_mouse_button_down(int button);
void  hinp_mouse_delta(float* x, float* y);
float hinp_mouse_wheel();
int   hinp_clipboard_set(const char* text, int length);

LRESULT WINAPI hinp_process_win_msg(HWND window, MSG msg, int translate_dispatch);
LRESULT CALLBACK hinp_callback(HWND window, UINT msg, WPARAM wparam, LPARAM lparam);