#pragma once
#include "os.h"
#include "gm.h"
#include "font_load.h"
#include "batcher.h"

const vec4 color_red;
const vec4 color_green;
const vec4 color_blue;
const vec4 color_cyan;
const vec4 color_magenta;
const vec4 color_yellow;
const vec4 color_grey;
const vec4 color_black;
const vec4 color_white;

typedef struct {
	Font_Info data;
} Font;

// Resources functions
void  renderer_init(const char* font_filepath);
Font* renderer_font_new(s32 size, const char* filepath);
void  renderer_font_free(Font* font);

Hobatch_Context* renderer_get_batch_ctx();

// Render functions
void render_begin(s32 ww, s32 wh);
void render_flush();
void render_clear(vec4 color);
void render_text(Font* font, const char* text, int length, vec2 position, vec4 color);
void render_fmt(Font* font, vec2 position, vec4 color, const char* fmt, ...);
void render_line(vec3 start, vec3 end, vec4 color);