#include <stdio.h>
#include <windows.h>
#include <windowsx.h>
#define GRAPHICS_MATH_IMPLEMENT
#include "gm.h"
#define HOGL_IMPLEMENT
#include "ho_gl.h"
#include "input.h"
#include "os.h"
#include "batcher.h"
#include "renderer.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include "interface.h"
#include "game.h"

typedef struct {
	s32    width;
	s32    height;
	HANDLE handle;
	HDC    dc;
	HGLRC  rc;
} Window;

void
opengl_init(Window* window)
{
	window->dc = GetDC(window->handle);
	PIXELFORMATDESCRIPTOR pfd;
	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion = 1;
	pfd.iLayerType = PFD_MAIN_PLANE;
	pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
	pfd.cDepthBits = 32;
	pfd.cColorBits = 24;
	pfd.iPixelType = PFD_TYPE_RGBA;

	int pixel_format = ChoosePixelFormat(window->dc, &pfd);
	if (!SetPixelFormat(window->dc, pixel_format, &pfd))
	{
		printf("Could not set a pixel format");
	}

	HGLRC temp_context = wglCreateContext(window->dc);
	if (!wglMakeCurrent(window->dc, temp_context))
	{
		printf("Could not create an OpenGL context.\n Make sure OpenGL compatible drivers are installed.");
	}

	int attribs[] =
	{
		WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
		WGL_CONTEXT_MINOR_VERSION_ARB, 3,
		WGL_CONTEXT_FLAGS_ARB, 0,
		0
	};
	HGLRC(WINAPI * wglCreateContextAttribsARB)(HDC hDC, HGLRC hShareContext, int* attribList);
	wglCreateContextAttribsARB = (HGLRC(WINAPI*)(HDC, HGLRC, int*))wglGetProcAddress("wglCreateContextAttribsARB");
	if (wglCreateContextAttribsARB)
	{
		window->rc = wglCreateContextAttribsARB(window->dc, 0, attribs);
		if (window->rc)
		{
			wglMakeCurrent(NULL, NULL);
			wglDeleteContext(temp_context);
			wglMakeCurrent(window->dc, window->rc);
		}
		else
		{
			printf("Could not create OpenGL core context");
		}
	}
	else
	{
		printf("Could not create OpenGL core context");
	}

	if (hogl_init_gl_extensions() == -1)
	{
		printf("Could not load OpenGL extensions");
	}
}

void
window_new_opengl(Window* window, WNDPROC window_callback, const char* title, int width, int height)
{
	WNDCLASSEX window_class = { 0 };
	window_class.cbSize = sizeof(WNDCLASSEX);
	window_class.style = 0;
	window_class.lpfnWndProc = window_callback;
	window_class.cbClsExtra = 0;
	window_class.cbWndExtra = 0;
	window_class.hInstance = 0;
	window_class.hIcon = 0;
	window_class.hCursor = LoadCursor(0, IDC_ARROW);
	window_class.hbrBackground = (HBRUSH)COLOR_WINDOW;
	window_class.lpszMenuName = 0;
	window_class.lpszClassName = title;
	window_class.hIconSm = 0;

	if (!RegisterClassEx(&window_class))
	{
		printf("Could not register a window class");
	}

	HWND handle = CreateWindowEx(
		WS_EX_ACCEPTFILES | WS_EX_APPWINDOW | WS_EX_WINDOWEDGE,
		window_class.lpszClassName, title,
		WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0, 0, width, height, 0, 0, 0, 0);

	if (!window)
	{
		printf("Could not create a window");
	}

	window->handle = handle;
	window->width = width;
	window->height = height;

	opengl_init(window);
	glViewport(0, 0, window->width, window->height);
}

typedef BOOL WINAPI wgl_swap_interval_ext(int interval);
#if 0
int CALLBACK
WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR command_line, int cmd_show)
#else
int main(int argc, char** argv)
#endif
{
	Window window = { 0 };
	window_new_opengl(&window, hinp_callback, "NeloChess", 800 + 16 + 200, 800 + 39);

	wgl_swap_interval_ext* wglSwapIntervalEXT = (wgl_swap_interval_ext*)wglGetProcAddress("wglSwapIntervalEXT");
	if(wglSwapIntervalEXT) wglSwapIntervalEXT(1);

    hinp_init(window.handle);

    renderer_init("C:\\Windows\\Fonts\\consolas.ttf");
	Chess_Interface interf = interface_init();
	Game game = {0};
	game_new(&game);

    bool running = true;
	while (running)
	{
		r64 start = os_time_us();
		MSG msg = { 0 };
		while (PeekMessageA(&msg, window.handle, 0, 0, TRUE))
		{
			hinp_process_win_msg(window.handle, msg, 1);
		}

        RECT rect = {0};
        GetClientRect(window.handle, &rect);
		s32 ww = rect.right - rect.left, wh = rect.bottom - rect.top;
        glViewport(0, 0, ww, wh);

        batch_set_window_size(renderer_get_batch_ctx(), ww, wh);
		interface_update_window(interf, ww, wh);
		interface_input(interf, &game);
        interface_render(interf, renderer_get_batch_ctx(), &game);

        batch_flush(renderer_get_batch_ctx());
		SwapBuffers(window.dc);

		hinp_clear();
	}

	interface_destroy(interf);
	ExitProcess(0);
}

