#include "renderer.h"
#include "ho_gl.h"
#include <stdlib.h>
#include "batcher.h"
//#include "font_render.h"
#include <float.h>
#include <stdarg.h>

const vec4 color_red     = { 1.0f, 0.0f, 0.0f, 1.0f };
const vec4 color_green   = { 0.0f, 1.0f, 0.0f, 1.0f };
const vec4 color_blue    = { 0.0f, 0.0f, 1.0f, 1.0f };
const vec4 color_cyan    = { 0.0f, 1.0f, 1.0f, 1.0f };
const vec4 color_magenta = { 1.0f, 0.0f, 1.0f, 1.0f };
const vec4 color_yellow  = { 1.0f, 1.0f, 0.0f, 1.0f };
const vec4 color_grey    = { 0.5f, 0.5f, 0.5f, 1.0f };
const vec4 color_black   = { 0.0f, 0.0f, 0.0f, 1.0f };
const vec4 color_white   = { 1.0f, 1.0f, 1.0f, 1.0f };
const vec4 clipping_none = { 0.0f, 0.0f, FLT_MAX, FLT_MAX };

static Hobatch_Context batch_ctx;

// Resources
void 
renderer_init(const char* font_filepath)
{
	batch_init(&batch_ctx, font_filepath);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
}

Font*
renderer_font_new(s32 size, const char* filepath)
{
	Font* font = calloc(1, sizeof(Font));
	if (font_load(filepath, &font->data, size) != FONT_LOAD_OK)
	{
		printf("Could not load font %s", filepath);
		return 0;
	}
	return font;
}

void
renderer_font_free(Font* font)
{
	free(font);
}

// Render
void
render_begin(int ww, int wh)
{
	batch_ctx.window_width = (r32)ww;
	batch_ctx.window_height = (r32)wh;

	glViewport(0, 0, ww, wh);
}

void
render_flush()
{
	batch_flush(&batch_ctx);
}

void
render_clear(vec4 color)
{
	glClearColor(color.r, color.g, color.b, color.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void
render_text(Font* font, const char* text, int length, vec2 position, vec4 color)
{
	batch_render_text(&batch_ctx, &font->data, text, length, 0, position, clipping_none, color, 0, 0);
	//text_render(&batch_ctx, &font->data, text, length, 0, position, clipping_none, color);
}

void
render_fmt(Font* font, vec2 position, vec4 color, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	char buffer[4096] = { 0 };
	int len = vsprintf(buffer, fmt, args);
	batch_render_text(&batch_ctx, &font->data, buffer, len, 0, position, clipping_none, color, 0, 0);

	va_end(args);
}

void
render_line(vec3 start, vec3 end, vec4 color)
{
	batch_render_line(&batch_ctx, start, end, color);
}

Hobatch_Context*
renderer_get_batch_ctx()
{
	return &batch_ctx;
}