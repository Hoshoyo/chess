#pragma once
#include <stdint.h>
#include "gm.h"
#include "ho_gl.h"
#include "font_load.h"

typedef struct {
    bool valid;
    uint32_t id;
    s32 unit;
} Hobatch_Texture;

typedef enum {
    HOBATCH_FONT_PX_8 = 0,
    HOBATCH_FONT_PX_10,
    HOBATCH_FONT_PX_12,
    HOBATCH_FONT_PX_13,
    HOBATCH_FONT_PX_14,
    HOBATCH_FONT_PX_16,
} Hobatch_Font_Size;

typedef struct {
    s8* name;
    Font_Info* font_info; // 8, 10, 12, 13, 14, 16
} Hobatch_Font;

typedef struct {
    bool enabled;

    GLuint qshader;
    GLuint qvao, qvbo, qebo;

    void* qmem, * qmem_ptr;
    u32 uniform_loc_texture_sampler;
    u32 uniform_loc_projection_matrix;

    s32 quad_count;

    r32 window_width, window_height;

    Hobatch_Texture textures[128];
    s32 tex_unit_next;

    s32 flush_count;
    s32 max_texture_units;

    // Lines
    GLuint lines_vao, lines_vbo;
    GLuint lshader;
    u32 uloc_lines_model;
    u32 uloc_lines_view;
    u32 uloc_lines_projection;

    void* lines_ptr;
    int lines_count;

    Hobatch_Font font;
} Hobatch_Context;

u32 batch_texture_create_from_data(const char* image_data, s32 width, s32 height, u32 filtering);
u32 batch_texture_create_from_data_bgra(const char* image_data, s32 width, s32 height, u32 filtering);

void batch_init(Hobatch_Context* ctx, const char* font_filepath);
void batch_flush(Hobatch_Context* ctx);
void batch_set_window_size(Hobatch_Context* ctx, s32 width, s32 height);
void batch_render_quad(Hobatch_Context* ctx, vec3 position, r32 width, r32 height, u32 texture_id, vec4 clipping, r32 blend_factor[4], vec4 color[4], vec2 texcoords[4], r32 red_alpha_override);
void batch_render_quad_textured_clipped(Hobatch_Context* ctx, vec3 position, r32 width, r32 height, u32 texture_id, vec4 clipping);
void batch_render_quad_textured(Hobatch_Context* ctx, vec3 position, r32 width, r32 height, u32 texture_id);
void batch_render_quad_color(Hobatch_Context* ctx, vec3 position, r32 width, r32 height, vec4 color[4]);
void batch_render_quad_color_solid(Hobatch_Context* ctx, vec3 position, r32 width, r32 height, vec4 color);
void batch_render_quad_color_clipped(Hobatch_Context* ctx, vec3 position, r32 width, r32 height, vec4 color[4], vec4 clipping);
void batch_render_quad_color_solid_clipped(Hobatch_Context* ctx, vec3 position, r32 width, r32 height, vec4 color, vec4 clipping);

void batch_render_border_simple(Hobatch_Context* ctx, r32 border_width, vec3 position, r32 width, r32 height, vec4 color);
void batch_render_border_simple_clipped(Hobatch_Context* ctx, r32 border_width, vec3 position, r32 width, r32 height, vec4 color, vec4 clipping);
void batch_render_freeborder(Hobatch_Context* ctx, r32 border_width[4], vec3 bl, vec3 br, vec3 tl, vec3 tr, vec4 color[4], vec4 clipping);
void batch_render_border_color_solid_clipped(Hobatch_Context* ctx, r32 border_width[4], vec3 position, r32 width, r32 height, vec4 color, vec4 clipping);
void batch_render_border_color_clipped(Hobatch_Context* ctx, r32 border_width[4], vec3 position, r32 width, r32 height, vec4 color[4], vec4 clipping);
void batch_render_outborder_color_solid_clipped(Hobatch_Context* ctx, r32 border_width[4], vec3 position, r32 width, r32 height, vec4 color, vec4 clipping);
void batch_render_outborder(Hobatch_Context* ctx, r32 border_width[4], vec3 position, r32 width, r32 height, vec4 color[4]);
void batch_render_outborder_color_solid(Hobatch_Context* ctx, r32 border_width[4], vec3 position, r32 width, r32 height, vec4 color);
void batch_render_outborder_color_clipped(Hobatch_Context* ctx, r32 border_width[4], vec3 position, r32 width, r32 height, vec4 color[4], vec4 clipping);

vec4 batch_render_color_from_hex(uint32_t hex);

void batch_render_line(Hobatch_Context* ctx, vec3 start, vec3 end, vec4 color);
void batch_render_line_clipped(Hobatch_Context* ctx, vec3 start, vec3 end, vec4 color, vec4 clipping);

vec4 batch_pre_render_text(Hobatch_Context* ctx, Font_Info* font_info, const char* text, int length, int start_index, vec2 position, int cursor_index, vec2* cursor_pos);
int  batch_render_text(Hobatch_Context* ctx, Font_Info* font_info, const char* text, int length, int start_index, vec2 position, vec4 clipping, vec4 color, int cursor_index, vec2* cursor_pos);
int  batch_render_fmt(Hobatch_Context* ctx, Font_Info* font_info, vec2 position, vec4 color, const char* fmt, ...);

int batch_font_load(const char* filepath, Hobatch_Font* out_batch_font);
void batch_font_free(Hobatch_Font* batch_font);