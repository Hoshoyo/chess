#include "batcher.h"
#include <stdlib.h>
#include <float.h>
#include <stdlib.h>
#include <string.h>

// https://www.khronos.org/opengl/wiki/Built-in_Variable_(GLSL)

#define BATCH_SIZE (1024 * 64)
#define ARRAY_LENGTH(A) (sizeof(A) / sizeof(*(A)))

typedef struct {
    vec3 position;           // 12
    vec2 text_coord;         // 20
    vec4 color;              // 36
    vec4 clipping;           // 52
    r32  blend_factor;       // 56
    r32  texture_index;      // 60
    r32  red_alpha_override; // 64
} Hobatch_Vertex;

const char vshader[] =
"#version 330 core\n"
"layout(location = 0) in vec3  v_position;\n"
"layout(location = 1) in vec2  v_text_coords;\n"
"layout(location = 2) in vec4  v_color;\n"
"layout(location = 3) in vec4  v_clipping;\n"
"layout(location = 4) in float v_blend_factor;\n"
"layout(location = 5) in float v_texture_index;\n"
"layout(location = 6) in float v_red_alpha_override;\n"

"out vec2 o_text_coords;\n"
"out vec4 o_color;\n"
"out vec4 clipping;\n"
"out vec2 position;\n"
"out float o_texture_index;\n"
"out float o_blend_factor;\n"
"out float red_alpha_override;\n"

"uniform mat4 u_projection = mat4(1.0);\n"

"void main()\n"
"{\n"
"    gl_Position = u_projection * vec4(v_position.xy, 0.0, 1.0);\n"
"    position = v_position.xy;\n"
"    o_text_coords  = v_text_coords;\n"
"    o_color = v_color;\n"
"    o_texture_index = v_texture_index;\n"
"    clipping = v_clipping;\n"
"    o_blend_factor = v_blend_factor;\n"
"    red_alpha_override = v_red_alpha_override;\n"
"}\n";

const char fshader[] =
"#version 330 core\n"
"in vec2 o_text_coords;\n"
"in vec4 o_color;\n"
"in float o_texture_index;\n"
"in float o_blend_factor;\n"
"in float red_alpha_override;\n"

"in vec4 clipping;\n"
"in vec2 position;\n"

"out vec4 color;\n"

"uniform sampler2D u_textures[gl_MaxTextureImageUnits];\n"

"vec4 sample_u_texture(int index) {\n"
"   if(index == 0) return texture(u_textures[0], o_text_coords);\n"
"   else if(index == 1) return texture(u_textures[1], o_text_coords);\n"
"   else if(index == 2) return texture(u_textures[2], o_text_coords);\n"
"   else if(index == 3) return texture(u_textures[3], o_text_coords);\n"
"   else if(index == 4) return texture(u_textures[4], o_text_coords);\n"
"   else if(index == 5) return texture(u_textures[5], o_text_coords);\n"
"   else if(index == 6) return texture(u_textures[6], o_text_coords);\n"
"   else if(index == 7) return texture(u_textures[7], o_text_coords);\n"
"   else if(index == 8) return texture(u_textures[8], o_text_coords);\n"
"   else if(index == 9) return texture(u_textures[9], o_text_coords);\n"
"   else if(index == 10) return texture(u_textures[10], o_text_coords);\n"
"   else if(index == 11) return texture(u_textures[11], o_text_coords);\n"
"   else if(index == 12) return texture(u_textures[12], o_text_coords);\n"
"   else if(index == 13) return texture(u_textures[13], o_text_coords);\n"
"   else if(index == 14) return texture(u_textures[14], o_text_coords);\n"
"   else if(index == 15) return texture(u_textures[15], o_text_coords);\n"
"   return vec4(0.0, 0.0, 0.0, 1.0);\n"
"}\n"

"void main()\n"
"{\n"
"    if(position.x < clipping.x || position.x > clipping.x + clipping.z) {\n"
"        discard;\n"
"    }\n"
"    if(position.y < clipping.y || position.y > clipping.y + clipping.w) {\n"
"        discard;\n"
"    }\n"
"    int index = int(o_texture_index);\n"
"    vec4 texture_color = sample_u_texture(index);\n"
"    color = mix(texture_color, o_color, o_blend_factor);\n"
"    color.a = mix(color.a, texture_color.r * o_color.a, red_alpha_override);\n"

//"color = vec4(1.0,1.0,1.0,0.6);\n"

"}\n";

extern FILE* trace;

static u32
shader_load_from_buffer(const s8* vert_shader, const s8* frag_shader, int vert_length, int frag_length)
{
    GLuint vs_id = glCreateShader(GL_VERTEX_SHADER);
    GLuint fs_id = glCreateShader(GL_FRAGMENT_SHADER);

    GLint compile_status;

    const GLchar* p_v[1] = { vert_shader };
    glShaderSource(vs_id, 1, p_v, &vert_length);

    const GLchar* p_f[1] = { frag_shader };
    glShaderSource(fs_id, 1, p_f, &frag_length);

    glCompileShader(vs_id);
    glGetShaderiv(vs_id, GL_COMPILE_STATUS, &compile_status);
    if (!compile_status) {
        char error_buffer[512] = { 0 };
        glGetShaderInfoLog(vs_id, sizeof(error_buffer), NULL, error_buffer);
        printf("shader_load: Error compiling vertex shader: %s\n", error_buffer);
        return -1;
    }

    glCompileShader(fs_id);
    glGetShaderiv(fs_id, GL_COMPILE_STATUS, &compile_status);
    if (!compile_status) {
        char error_buffer[512] = { 0 };
        glGetShaderInfoLog(fs_id, sizeof(error_buffer) - 1, NULL, error_buffer);
        printf("shader_load: Error compiling fragment shader: %s\n", error_buffer);
        return -1;
    }

    GLuint shader_id = glCreateProgram();
    glAttachShader(shader_id, vs_id);
    glAttachShader(shader_id, fs_id);
    glDeleteShader(vs_id);
    glDeleteShader(fs_id);
    glLinkProgram(shader_id);

    glGetProgramiv(shader_id, GL_LINK_STATUS, &compile_status);
    if (compile_status == 0) {
        GLchar error_buffer[512] = { 0 };
        glGetProgramInfoLog(shader_id, sizeof(error_buffer) - 1, NULL, error_buffer);
        printf("shader_load: Error linking program: %s", error_buffer);
        return -1;
    }

    glValidateProgram(shader_id);
    return shader_id;
}

static u32
shader_new_lines()
{
    char vshader[] = "#version 330 core\n"
        "layout(location = 0) in vec3 v_vertex;\n"
        "layout(location = 1) in vec4 v_color;\n"
        "layout(location = 2) in vec4 v_clipping;\n"
        "out vec4 o_color;\n"
        "out vec4 clipping;\n"
        "out vec2 position;\n"

        "uniform mat4 model_matrix = mat4(1.0);\n"
        "uniform mat4 view_matrix = mat4(1.0);\n"
        "uniform mat4 projection_matrix = mat4(1.0);\n"

        "void main() {\n"
        "   gl_Position = projection_matrix * view_matrix * model_matrix * vec4(v_vertex, 1.0);\n"
        "   position = v_vertex.xy;"
        "   o_color = v_color;\n"
        "   clipping = v_clipping;\n"
        "}";

    char fshader[] = "#version 330 core\n"
        "in vec2 position;\n"
        "in vec4 o_color;\n"
        "in vec4 clipping;\n"
        "out vec4 color;\n"
        "void main() {\n"
        "   if(position.x < clipping.x || position.x > clipping.x + clipping.z) {\n"
        "      discard;\n"
        "   }\n"
        "   if(position.y < clipping.y || position.y > clipping.y + clipping.w) {\n"
        "      discard;\n"
        "   }\n"
        "   color = o_color;\n"
        "}";

    return shader_load_from_buffer(vshader, fshader, sizeof(vshader), sizeof(fshader));
}

typedef struct {
    vec3 position;
    vec4 color;
    vec4 clipping;
} Vertex_3D;

static int
init_lines(Hobatch_Context* ctx, int batch_size)
{
    ctx->lshader = shader_new_lines();
    glGenVertexArrays(1, &ctx->lines_vao);
    glBindVertexArray(ctx->lines_vao);

    glGenBuffers(1, &ctx->lines_vbo);

    glBindBuffer(GL_ARRAY_BUFFER, ctx->lines_vbo);
    glBufferData(GL_ARRAY_BUFFER, 2 * sizeof(Vertex_3D) * batch_size, 0, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_3D), (void*)offsetof(Vertex_3D, position));
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex_3D), (void*)offsetof(Vertex_3D, color));
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex_3D), (void*)offsetof(Vertex_3D, clipping));

    ctx->uloc_lines_model = glGetUniformLocation(ctx->lshader, "model_matrix");
    ctx->uloc_lines_view = glGetUniformLocation(ctx->lshader, "view_matrix");
    ctx->uloc_lines_projection = glGetUniformLocation(ctx->lshader, "projection_matrix");

    return 0;
}

static void
render_lines(Hobatch_Context* ctx)
{
    if (ctx->enabled)
    {
        glUseProgram(ctx->lshader);

        glBindVertexArray(ctx->lines_vao);
        glBindBuffer(GL_ARRAY_BUFFER, ctx->lines_vbo);
        glUnmapBuffer(GL_ARRAY_BUFFER);

        mat4 projection = gm_mat4_ortho(0, (r32)ctx->window_width, 0, (r32)ctx->window_height);
        glUniformMatrix4fv(ctx->uniform_loc_projection_matrix, 1, GL_TRUE, (GLfloat*)projection.data);

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);

        glLineWidth(1.0f);
        glDrawArrays(GL_LINES, 0, 2 * ctx->lines_count);
    }

    ctx->lines_ptr = 0;
    ctx->lines_count = 0;
}

u32
batch_texture_create_from_data(const char* image_data, s32 width, s32 height, u32 filtering)
{
    u32 texture_id = 0;

    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filtering);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filtering);

    return texture_id;
}

u32
batch_texture_create_from_data_bgra(const char* image_data, s32 width, s32 height, u32 filtering)
{
    u32 texture_id = 0;

    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, image_data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filtering);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filtering);

    return texture_id;
}

void
batch_init(Hobatch_Context* ctx, const char* font_filepath)
{
    init_lines(ctx, 1024);
    ctx->qshader = shader_load_from_buffer(vshader, fshader, sizeof(vshader) - 1, sizeof(fshader) - 1);

    ctx->uniform_loc_texture_sampler = glGetUniformLocation(ctx->qshader, "u_textures");
    ctx->uniform_loc_projection_matrix = glGetUniformLocation(ctx->qshader, "u_projection");

    glUseProgram(ctx->qshader);

    // Query the max number of texture units
    int texture_units = 0;
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &texture_units);
    int texture_samplers[128] = { 0 };
    for (s32 i = 0; i < 128; ++i)
        texture_samplers[i] = i;
    glUniform1iv(ctx->uniform_loc_texture_sampler, texture_units, texture_samplers);
    ctx->max_texture_units = texture_units;

    glGenVertexArrays(1, &ctx->qvao);
    glBindVertexArray(ctx->qvao);

    glGenBuffers(1, &ctx->qvbo);
    glBindBuffer(GL_ARRAY_BUFFER, ctx->qvbo);

    glBufferData(GL_ARRAY_BUFFER, sizeof(Hobatch_Vertex) * BATCH_SIZE * 4, 0, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);
    glEnableVertexAttribArray(4);
    glEnableVertexAttribArray(5);
    glEnableVertexAttribArray(6);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Hobatch_Vertex), &((Hobatch_Vertex*)0)->position);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Hobatch_Vertex), &((Hobatch_Vertex*)0)->text_coord);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Hobatch_Vertex), &((Hobatch_Vertex*)0)->color);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Hobatch_Vertex), &((Hobatch_Vertex*)0)->clipping);
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(Hobatch_Vertex), &((Hobatch_Vertex*)0)->blend_factor);
    glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(Hobatch_Vertex), &((Hobatch_Vertex*)0)->texture_index);
    glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, sizeof(Hobatch_Vertex), &((Hobatch_Vertex*)0)->red_alpha_override);

    u32* indices = (u32*)calloc(1, BATCH_SIZE * 6 * sizeof(u32));
    for (u32 i = 0, j = 0; i < BATCH_SIZE * 6; i += 6, j += 4)
    {
        indices[i + 0] = j;
        indices[i + 1] = j + 1;
        indices[i + 2] = j + 2;
        indices[i + 3] = j + 2;
        indices[i + 4] = j + 1;
        indices[i + 5] = j + 3;
    }
    glGenBuffers(1, &ctx->qebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ctx->qebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(u32) * BATCH_SIZE, indices, GL_STATIC_DRAW);
    free(indices);

    batch_font_load(font_filepath, &ctx->font);

    ctx->enabled = true;
}

void
batch_set_window_size(Hobatch_Context* ctx, s32 width, s32 height)
{
    ctx->window_width = (r32)width;
    ctx->window_height = (r32)height;
}

void
batch_flush(Hobatch_Context* ctx)
{
    ctx->flush_count++;
    glUseProgram(ctx->qshader);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    glBindVertexArray(ctx->qvao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ctx->qebo);
    glBindBuffer(GL_ARRAY_BUFFER, ctx->qvbo);

    glUnmapBuffer(GL_ARRAY_BUFFER);

    for (int i = 0; i < sizeof(ctx->textures) / sizeof(*ctx->textures); ++i)
    {
        if (ctx->textures[i].valid)
        {
            if(glBindTextureUnit)
            {
                glBindTextureUnit(ctx->textures[i].unit, ctx->textures[i].id);
            }
            else
            {
                glActiveTexture(GL_TEXTURE0+ctx->textures[i].unit);
                glBindTexture(GL_TEXTURE_2D, ctx->textures[i].id);
            }
        }
    }

    mat4 projection = gm_mat4_ortho(0, (r32)ctx->window_width, 0, (r32)ctx->window_height);
    glUniformMatrix4fv(ctx->uniform_loc_projection_matrix, 1, GL_TRUE, (GLfloat*)projection.data);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);
    glEnableVertexAttribArray(4);
    glEnableVertexAttribArray(5);
    glEnableVertexAttribArray(6);

    if (ctx->enabled)
    {
        glDrawElements(GL_TRIANGLES, 6 * ctx->quad_count, GL_UNSIGNED_INT, 0);
    }
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    // reset
    ctx->qmem = 0;
    ctx->qmem_ptr = 0;
    ctx->quad_count = 0;
    ctx->tex_unit_next = 0;
    memset(ctx->textures, 0, sizeof(ctx->textures));

    render_lines(ctx);
}

void
batch_render_freequad(Hobatch_Context* ctx, vec3 bl, vec3 br, vec3 tl, vec3 tr,
    u32 texture_id, vec4 clipping, r32 blend_factor[4], vec4 color[4], vec2 texcoords[4], r32 red_alpha_override)
{
    if (((char*)ctx->qmem_ptr - (char*)ctx->qmem) >= BATCH_SIZE * sizeof(Hobatch_Vertex))
    {
        batch_flush(ctx);
    }

    s32 texture_unit_index = texture_id % ARRAY_LENGTH(ctx->textures);
    s32 texture_unit = -1;

    // If this is a new texture and we already have the max amount, flush it
    if (ctx->tex_unit_next >= ctx->max_texture_units &&
        !(ctx->textures[texture_unit_index].valid && ctx->textures[texture_unit_index].id != texture_id))
    {
        batch_flush(ctx);
    }

    // search for a texture unit slot
    while (ctx->textures[texture_unit_index].valid)
    {
        if (ctx->textures[texture_unit_index].id == texture_id)
        {
            texture_unit = ctx->textures[texture_unit_index].unit;
            break;
        }
        texture_unit_index = (texture_unit_index + 1) % ARRAY_LENGTH(ctx->textures);
    }

    // we did not find a slot with this texture already, so grab the next available
    if (texture_unit == -1)
    {
        texture_unit = ctx->tex_unit_next++;
    }
    ctx->textures[texture_unit_index].id = texture_id;
    ctx->textures[texture_unit_index].unit = texture_unit;
    ctx->textures[texture_unit_index].valid = true;

    if (!ctx->qmem)
    {
        glBindBuffer(GL_ARRAY_BUFFER, ctx->qvbo);
        ctx->qmem = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        ctx->qmem_ptr = ctx->qmem;
    }

    // 0 blend factor means full texture no color
    Hobatch_Vertex v[] =
    {
        {bl, texcoords[0], color[0], clipping, blend_factor[0], (r32)texture_unit, red_alpha_override},
        {br, texcoords[1], color[1], clipping, blend_factor[1], (r32)texture_unit, red_alpha_override},
        {tl, texcoords[2], color[2], clipping, blend_factor[2], (r32)texture_unit, red_alpha_override},
        {tr, texcoords[3], color[3], clipping, blend_factor[3], (r32)texture_unit, red_alpha_override},
    };

    memcpy(ctx->qmem_ptr, v, sizeof(v));
    ctx->qmem_ptr = ((char*)ctx->qmem_ptr) + sizeof(v);
    ctx->quad_count += 1;
}

// bl, br, tl, tr
void
batch_render_quad(Hobatch_Context* ctx, vec3 position,
    r32 width, r32 height, u32 texture_id, vec4 clipping,
    r32 blend_factor[4], vec4 color[4], vec2 texcoords[4], r32 red_alpha_override)
{
    if (((char*)ctx->qmem_ptr - (char*)ctx->qmem) >= BATCH_SIZE * sizeof(Hobatch_Vertex))
    {
        batch_flush(ctx);
    }

    s32 texture_unit_index = texture_id % ARRAY_LENGTH(ctx->textures);
    s32 texture_unit = -1;

    // If this is a new texture and we already have the max amount, flush it
    if (ctx->tex_unit_next >= ctx->max_texture_units &&
        !(ctx->textures[texture_unit_index].valid && ctx->textures[texture_unit_index].id != texture_id))
    {
        batch_flush(ctx);
    }

    // search for a texture unit slot
    while (ctx->textures[texture_unit_index].valid)
    {
        if (ctx->textures[texture_unit_index].id == texture_id)
        {
            texture_unit = ctx->textures[texture_unit_index].unit;
            break;
        }
        texture_unit_index = (texture_unit_index + 1) % ARRAY_LENGTH(ctx->textures);
    }

    // we did not find a slot with this texture already, so grab the next available
    if (texture_unit == -1)
    {
        texture_unit = ctx->tex_unit_next++;
    }
    ctx->textures[texture_unit_index].id = texture_id;
    ctx->textures[texture_unit_index].unit = texture_unit;
    ctx->textures[texture_unit_index].valid = true;

    if (!ctx->qmem)
    {
        glBindBuffer(GL_ARRAY_BUFFER, ctx->qvbo);
        ctx->qmem = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        ctx->qmem_ptr = ctx->qmem;
    }

    // 0 blend factor means full texture no color
    Hobatch_Vertex v[] =
    {
        {(vec3) { position.x, position.y, position.z },                  texcoords[0], color[0], clipping, blend_factor[0], (r32)texture_unit, red_alpha_override},
        {(vec3){position.x + width, position.y, position.z},          texcoords[1], color[1], clipping, blend_factor[1], (r32)texture_unit, red_alpha_override},
        {(vec3){position.x, position.y + height, position.z},         texcoords[2], color[2], clipping, blend_factor[2], (r32)texture_unit, red_alpha_override},
        {(vec3){position.x + width, position.y + height, position.z}, texcoords[3], color[3], clipping, blend_factor[3], (r32)texture_unit, red_alpha_override},
    };

    memcpy(ctx->qmem_ptr, v, sizeof(v));
    ctx->qmem_ptr = ((char*)ctx->qmem_ptr) + sizeof(v);
    ctx->quad_count += 1;
}

void
batch_render_quad_textured_clipped(Hobatch_Context* ctx, vec3 position,
    r32 width, r32 height, u32 texture_id, vec4 clipping)
{
    r32 blend_factor[4] = { 0,0,0,0 };
    vec2 texcoords[4] =
    {
        (vec2) { 0.0f, 0.0f },
        (vec2) { 1.0f, 0.0f },
        (vec2) { 0.0f, 1.0f },
        (vec2) { 1.0f, 1.0f }
    };
    vec4 color[4] = { 0 };
    batch_render_quad(ctx, position, width, height, texture_id, clipping, blend_factor, color, texcoords, 0.0f);
}

void
batch_render_freequad_textured_clipped(Hobatch_Context* ctx,
    vec3 bl, vec3 br, vec3 tl, vec3 tr, u32 texture_id, vec4 clipping)
{
    r32 blend_factor[4] = { 0,0,0,0 };
    vec2 texcoords[4] =
    {
        (vec2) { 0.0f, 0.0f },
        (vec2) { 1.0f, 0.0f },
        (vec2) { 0.0f, 1.0f },
        (vec2) { 1.0f, 1.0f }
    };
    vec4 color[4] = { 0 };
    batch_render_freequad(ctx, bl, br, tl, tr, texture_id, clipping, blend_factor, color, texcoords, 0.0f);
}

void
batch_render_quad_textured(Hobatch_Context* ctx, vec3 position, r32 width, r32 height, u32 texture_id)
{
    vec4 clipping = (vec4){ 0,0,FLT_MAX,FLT_MAX };
    batch_render_quad_textured_clipped(ctx, position, width, height, texture_id, clipping);
}

void
batch_render_freequad_textured(Hobatch_Context* ctx, vec3 bl, vec3 br, vec3 tl, vec3 tr, u32 texture_id)
{
    vec4 clipping = (vec4){ 0,0,FLT_MAX,FLT_MAX };
    batch_render_freequad_textured_clipped(ctx, bl, br, tl, tr, texture_id, clipping);
}

void
batch_render_quad_color(Hobatch_Context* ctx, vec3 position, r32 width, r32 height, vec4 color[4])
{
    vec4 clipping = (vec4){ 0,0,FLT_MAX,FLT_MAX };
    vec2 texcoords[4] =
    {
        (vec2) { 0.0f, 0.0f },
        (vec2) { 1.0f, 0.0f },
        (vec2) { 0.0f, 1.0f },
        (vec2) { 1.0f, 1.0f }
    };
    r32 blend_factor[4] = { 1,1,1,1 };
    batch_render_quad(ctx, position, width, height, 0, clipping, blend_factor, color, texcoords, 0.0f);
}

void
batch_render_freequad_color(Hobatch_Context* ctx, vec3 bl, vec3 br, vec3 tl, vec3 tr, vec4 color[4])
{
    vec4 clipping = (vec4){ 0,0,FLT_MAX,FLT_MAX };
    vec2 texcoords[4] =
    {
        (vec2) { 0.0f, 0.0f },
        (vec2) { 1.0f, 0.0f },
        (vec2) { 0.0f, 1.0f },
        (vec2) { 1.0f, 1.0f }
    };
    r32 blend_factor[4] = { 1,1,1,1 };
    batch_render_freequad(ctx, bl, br, tl, tr, 0, clipping, blend_factor, color, texcoords, 0.0f);
}

void
batch_render_quad_color_solid(Hobatch_Context* ctx, vec3 position, r32 width, r32 height, vec4 color)
{
    vec4 clipping = (vec4){ 0,0,FLT_MAX,FLT_MAX };
    r32 blend_factor[4] = { 1,1,1,1 };
    vec2 texcoords[4] =
    {
        (vec2) { 0.0f, 0.0f },
        (vec2) { 1.0f, 0.0f },
        (vec2) { 0.0f, 1.0f },
        (vec2) { 1.0f, 1.0f }
    };
    vec4 c[] = { color, color, color, color };
    batch_render_quad(ctx, position, width, height, 0, clipping, blend_factor, c, texcoords, 0.0f);
}

void
batch_render_freequad_color_solid(Hobatch_Context* ctx, vec3 bl, vec3 br, vec3 tl, vec3 tr, vec4 color)
{
    vec4 clipping = (vec4){ 0,0,FLT_MAX,FLT_MAX };
    r32 blend_factor[4] = { 1,1,1,1 };
    vec2 texcoords[4] =
    {
        (vec2) { 0.0f, 0.0f },
        (vec2) { 1.0f, 0.0f },
        (vec2) { 0.0f, 1.0f },
        (vec2) { 1.0f, 1.0f }
    };
    vec4 c[] = { color, color, color, color };
    batch_render_freequad(ctx, bl, br, tl, tr, 0, clipping, blend_factor, c, texcoords, 0.0f);
}

void
batch_render_quad_color_clipped(Hobatch_Context* ctx, vec3 position, r32 width, r32 height, vec4 color[4], vec4 clipping)
{
    r32 blend_factor[4] = { 1,1,1,1 };
    vec2 texcoords[4] =
    {
        (vec2) { 0.0f, 0.0f },
        (vec2) { 1.0f, 0.0f },
        (vec2) { 0.0f, 1.0f },
        (vec2) { 1.0f, 1.0f }
    };
    batch_render_quad(ctx, position, width, height, 0, clipping, blend_factor, color, texcoords, 0.0f);
}

void
batch_render_freequad_color_clipped(Hobatch_Context* ctx, vec3 bl, vec3 br, vec3 tl, vec3 tr, vec4 color[4], vec4 clipping)
{
    r32 blend_factor[4] = { 1,1,1,1 };
    vec2 texcoords[4] =
    {
        (vec2) { 0.0f, 0.0f },
        (vec2) { 1.0f, 0.0f },
        (vec2) { 0.0f, 1.0f },
        (vec2) { 1.0f, 1.0f }
    };
    batch_render_freequad(ctx, bl, br, tl, tr, 0, clipping, blend_factor, color, texcoords, 0.0f);
}

void
batch_render_quad_color_solid_clipped(Hobatch_Context* ctx, vec3 position, r32 width, r32 height, vec4 color, vec4 clipping)
{
    r32 blend_factor[4] = { 1,1,1,1 };
    vec4 c[] = { color, color, color, color };
    vec2 texcoords[4] =
    {
        (vec2) { 0.0f, 0.0f },
        (vec2) { 1.0f, 0.0f },
        (vec2) { 0.0f, 1.0f },
        (vec2) { 1.0f, 1.0f }
    };
    batch_render_quad(ctx, position, width, height, 0, clipping, blend_factor, c, texcoords, 0.0f);
}

void
batch_render_freequad_color_solid_clipped(Hobatch_Context* ctx, vec3 bl, vec3 br, vec3 tl, vec3 tr, vec4 color, vec4 clipping)
{
    r32 blend_factor[4] = { 1,1,1,1 };
    vec2 texcoords[4] =
    {
        (vec2) { 0.0f, 0.0f },
        (vec2) { 1.0f, 0.0f },
        (vec2) { 0.0f, 1.0f },
        (vec2) { 1.0f, 1.0f }
    };
    vec4 c[] = { color, color, color, color };
    batch_render_freequad(ctx, bl, br, tl, tr, 0, clipping, blend_factor, c, texcoords, 0.0f);
}

vec4
batch_render_color_from_hex(uint32_t hex)
{
    return (vec4) {
        (float)((hex >> 24) & 0xff) / 255.0f,
            (float)((hex >> 16) & 0xff) / 255.0f,
            (float)((hex >> 8) & 0xff) / 255.0f,
            (float)((hex) & 0xff) / 255.0f,
    };
}

// border width = left right top bottom
void
batch_render_freeborder(Hobatch_Context* ctx, r32 border_width[4], vec3 bl, vec3 br, vec3 tl, vec3 tr, vec4 color[4], vec4 clipping)
{
    const int left = 0, right = 1, top = 2, bottom = 3;

    vec4 cc[] = { color[0],color[0], color[0], color[0] };
    // left
    batch_render_freequad_color_clipped(ctx, bl, (vec3) { bl.x + border_width[left], bl.y + border_width[bottom], 0 }, tl, (vec3) { tl.x + border_width[left], tl.y - border_width[top], 0 }, cc, clipping);

    // right
    cc[0] = cc[1] = cc[2] = cc[3] = color[1];
    batch_render_freequad_color_clipped(ctx, (vec3) { br.x - border_width[right], br.y + border_width[bottom], 0 }, br, (vec3) { tr.x - border_width[right], tr.y - border_width[top], 0 }, tr, cc, clipping);

    // top
    cc[0] = cc[1] = cc[2] = cc[3] = color[2];
    batch_render_freequad_color_clipped(ctx, (vec3) { tl.x + border_width[left], tl.y - border_width[top], 0 }, (vec3) { tr.x - border_width[right], tl.y - border_width[top], 0 }, tl, tr, cc, clipping);

    // bottom
    cc[0] = cc[1] = cc[2] = cc[3] = color[3];
    batch_render_freequad_color_clipped(ctx, bl, br, (vec3) { bl.x + border_width[left], bl.y + border_width[bottom], 0 }, (vec3) { br.x - border_width[right], br.y + border_width[bottom], 0 }, cc, clipping);
}

void
batch_render_border_color_solid_clipped(Hobatch_Context* ctx, r32 border_width[4], vec3 position, r32 width, r32 height, vec4 color, vec4 clipping)
{
    vec3 bl = (vec3){ position.x, position.y, 0 };
    vec3 br = (vec3){ position.x + width, position.y, 0 };
    vec3 tl = (vec3){ position.x, position.y + height, 0 };
    vec3 tr = (vec3){ position.x + width, position.y + height, 0 };
    vec4 colors[] = { color, color, color, color };
    batch_render_freeborder(ctx, border_width, bl, br, tl, tr, colors, clipping);
}

void
batch_render_border_simple(Hobatch_Context* ctx, r32 border_width, vec3 position, r32 width, r32 height, vec4 color)
{
    vec3 bl = (vec3){ position.x, position.y, 0 };
    vec3 br = (vec3){ position.x + width, position.y, 0 };
    vec3 tl = (vec3){ position.x, position.y + height, 0 };
    vec3 tr = (vec3){ position.x + width, position.y + height, 0 };
    vec4 colors[] = { color, color, color, color };
    r32 bw[] = { border_width, border_width, border_width, border_width };
    batch_render_freeborder(ctx, bw, bl, br, tl, tr, colors, (vec4) { 0, 0, FLT_MAX, FLT_MAX });
}

void
batch_render_border_simple_clipped(Hobatch_Context* ctx, r32 border_width, vec3 position, r32 width, r32 height, vec4 color, vec4 clipping)
{
    vec3 bl = (vec3){ position.x, position.y, 0 };
    vec3 br = (vec3){ position.x + width, position.y, 0 };
    vec3 tl = (vec3){ position.x, position.y + height, 0 };
    vec3 tr = (vec3){ position.x + width, position.y + height, 0 };
    vec4 colors[] = { color, color, color, color };
    r32 bw[] = { border_width, border_width, border_width, border_width };
    batch_render_freeborder(ctx, bw, bl, br, tl, tr, colors, clipping);
}

void
batch_render_border_color_clipped(Hobatch_Context* ctx, r32 border_width[4], vec3 position, r32 width, r32 height, vec4 color[4], vec4 clipping)
{
    vec3 bl = (vec3){ position.x, position.y, 0 };
    vec3 br = (vec3){ position.x + width, position.y, 0 };
    vec3 tl = (vec3){ position.x, position.y + height, 0 };
    vec3 tr = (vec3){ position.x + width, position.y + height, 0 };
    batch_render_freeborder(ctx, border_width, bl, br, tl, tr, color, clipping);
}

void
batch_render_outborder_color_solid_clipped(Hobatch_Context* ctx, r32 border_width[4], vec3 position, r32 width, r32 height, vec4 color, vec4 clipping)
{
    const int left = 0, right = 1, top = 2, bottom = 3;
    vec3 bl = (vec3){ position.x - border_width[left], position.y - border_width[bottom], 0 };
    vec3 br = (vec3){ position.x + width + border_width[right], position.y - border_width[bottom], 0 };
    vec3 tl = (vec3){ position.x - border_width[left], position.y + height + border_width[top], 0 };
    vec3 tr = (vec3){ position.x + width + border_width[right], position.y + height + border_width[top], 0 };
    vec4 colors[] = { color, color, color, color };
    batch_render_freeborder(ctx, border_width, bl, br, tl, tr, colors, clipping);
}

void
batch_render_outborder_color_solid(Hobatch_Context* ctx, r32 border_width[4], vec3 position, r32 width, r32 height, vec4 color)
{
    vec4 clipping = (vec4){ 0,0,FLT_MAX,FLT_MAX };
    batch_render_outborder_color_solid_clipped(ctx, border_width, position, width, height, color, clipping);
}

void
batch_render_outborder(Hobatch_Context* ctx, r32 border_width[4], vec3 position, r32 width, r32 height, vec4 color[4])
{
    vec4 clipping = (vec4){ 0,0,FLT_MAX,FLT_MAX };
    const int left = 0, right = 1, top = 2, bottom = 3;
    vec3 bl = (vec3){ position.x - border_width[left], position.y - border_width[bottom], 0 };
    vec3 br = (vec3){ position.x + width + border_width[right], position.y - border_width[bottom], 0 };
    vec3 tl = (vec3){ position.x - border_width[left], position.y + height + border_width[top], 0 };
    vec3 tr = (vec3){ position.x + width + border_width[right], position.y + height + border_width[top], 0 };
    batch_render_freeborder(ctx, border_width, bl, br, tl, tr, color, clipping);
}

void
batch_render_outborder_color_clipped(Hobatch_Context* ctx, r32 border_width[4], vec3 position, r32 width, r32 height, vec4 color[4], vec4 clipping)
{
    const int left = 0, right = 1, top = 2, bottom = 3;
    vec3 bl = (vec3){ position.x - border_width[left], position.y - border_width[bottom], 0 };
    vec3 br = (vec3){ position.x + width + border_width[right], position.y - border_width[bottom], 0 };
    vec3 tl = (vec3){ position.x - border_width[left], position.y + height + border_width[top], 0 };
    vec3 tr = (vec3){ position.x + width + border_width[right], position.y + height + border_width[top], 0 };
    batch_render_freeborder(ctx, border_width, bl, br, tl, tr, color, clipping);
}

/*
    Lines
*/

void
batch_render_line_clipped(Hobatch_Context* ctx, vec3 start, vec3 end, vec4 color, vec4 clipping)
{
    if (ctx->lines_ptr == 0) {
        glBindVertexArray(ctx->lines_vao);
        glBindBuffer(GL_ARRAY_BUFFER, ctx->lines_vbo);
        ctx->lines_ptr = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
    }

    Vertex_3D* lines = ((Vertex_3D*)ctx->lines_ptr) + (2 * ctx->lines_count);
    lines[0].position = start;
    lines[0].color = color;
    lines[0].clipping = clipping;

    lines[1].position = end;
    lines[1].color = color;
    lines[1].clipping = clipping;

    ctx->lines_count++;
}

void
batch_render_line(Hobatch_Context* ctx, vec3 start, vec3 end, vec4 color)
{
    batch_render_line_clipped(ctx, start, end, color, (vec4) { 0.0f, 0.0f, FLT_MAX, FLT_MAX });
}

/*
    Text
*/

static u8
hexdigit_to_u8(u8 d) {
    if (d >= 'A' && d <= 'F')
        return d - 'A' + 10;
    if (d >= 'a' && d <= 'f')
        return d - 'a' + 10;
    return d - '0';
}
u32
str_hex_to_u64(char* text, int length) {
    u32 res = 0;
    u32 count = 0;
    for (s32 i = length - 1; i >= 0; --i, ++count) {
        if (text[i] == 'x') break;
        char c = hexdigit_to_u8(text[i]);
        res += (u32)c << (count * 4);
    }
    return res;
}

static u32
ustring_get_unicode(u8* text, u32* advance)
{
    u32 result = 0;
    if (text[0] & 128)
    {
        // 1xxx xxxx
        if (text[0] >= 0xF0)
        {
            // 4 bytes
            *advance = 4;
            result = ((text[0] & 0x07) << 18) | ((text[1] & 0x3F) << 12) | ((text[2] & 0x3F) << 6) | (text[3] & 0x3F);
        }
        else if (text[0] >= 0xE0)
        {
            // 3 bytes
            *advance = 3;
            result = ((text[0] & 0x0F) << 12) | ((text[1] & 0x3F) << 6) | (text[2] & 0x3F);
        }
        else if (text[0] >= 0xC0)
        {
            // 2 bytes
            *advance = 2;
            result = ((text[0] & 0x1F) << 6) | (text[1] & 0x3F);
        }
        else
        {
            // continuation byte
            *advance = 1;
            result = text[0];
        }
    }
    else
    {
        *advance = 1;
        result = (u32)text[0];
    }
    return result;
}

int
batch_render_text(Hobatch_Context* ctx, Font_Info* font_info, const char* text, int length, int start_index,
    vec2 position, vec4 clipping, vec4 color, int cursor_index, vec2* cursor_pos)
{
    Character* characters = font_info->characters;

    int idx = 0;
    vec2 start_position = position;
    bool cursor_filled = false;

    bool escaping = 0;

    for (s32 i = 0, c = 0; c < length; ++i)
    {
        u32 advance = 1;
        u32 unicode = *(u8*)(text + ((idx + start_index) % length));
        idx += advance;

        s32 repeat = 1;
        c += (s32)advance;

        bool new_line = 0;

        if (unicode == '\\' && !escaping)
        {
            escaping = 1;
            continue;
        }

        if (escaping) escaping = 0;

        if (unicode == '\t')
        {
            unicode = ' ';
            repeat = 3;
        }
        else if (!characters[unicode].renderable)
        {
            unicode = ' ';
            new_line = 1;
        }

        for (int r = 0; r < repeat; ++r)
        {
            GLfloat xpos = position.x + characters[unicode].bearing[0];
            GLfloat ypos = position.y - (characters[unicode].size[1] - characters[unicode].bearing[1]);
            GLfloat w = (GLfloat)characters[unicode].size[0];
            GLfloat h = (GLfloat)characters[unicode].size[1];

            if (cursor_pos && (c - advance) == cursor_index)
            {
                *cursor_pos = (vec2){ roundf(xpos), roundf(ypos) };
                cursor_filled = true;
            }
#if 1
            if (unicode != '\n' && unicode != '\t')
            {
                r32 blend_factor[4] = { 1,1,1,1 };
                vec4 color_arr[4] = { color, color, color, color };
                vec2 texcoords[4] =
                {
                  characters[unicode].topl,
                  characters[unicode].topr,
                  characters[unicode].botl,
                  characters[unicode].botr,
                };

                vec3 bl = (vec3){ roundf(xpos), roundf(ypos), 0.0f };
                vec3 br = (vec3){ roundf(xpos + w), roundf(ypos), 0.0f };
                vec3 tl = (vec3){ roundf(xpos), roundf(ypos + h), 0.0f };
                vec3 tr = (vec3){ roundf(xpos + w), roundf(ypos + h), 0.0f };
                batch_render_freequad(ctx, bl, br, tl, tr, font_info->atlas_full_id, clipping, blend_factor, color_arr, texcoords, 1.0f);

            }
#endif
            if (new_line)
            {
                position.y -= font_info->font_size;
                position.x = start_position.x;
            }
            else
            {
                position.x += characters[unicode].advance >> 6;
            }
        }
    }
    if (!cursor_filled && cursor_pos)
    {
        *cursor_pos = (vec2){ position.x, position.y };
    }

    return 0;
}

// out rect (x, y, width, height)
vec4
batch_pre_render_text(Hobatch_Context* ctx, Font_Info* font_info, const char* text, int length, int start_index,
    vec2 position, int cursor_index, vec2* cursor_pos)
{
    Character* characters = font_info->characters;

    int idx = 0;
    vec2 start_position = position;
    bool cursor_filled = false;

    r32 min_x = FLT_MAX;
    r32 max_x = -FLT_MAX;
    r32 min_y = FLT_MAX;
    r32 max_y = -FLT_MAX;

    bool escaping = 0;

    for (s32 i = 0, c = 0; c < length; ++i)
    {
        u32 advance = 0;
        u32 unicode = ustring_get_unicode((u8*)text + ((idx + start_index) % length), &advance);
        idx += advance;

        s32 repeat = 1;
        c += (s32)advance;

        bool new_line = 0;

        if (unicode == '\\' && !escaping)
        {
            escaping = 1;
            continue;
        }

        if (escaping) escaping = 0;

        if (unicode == '\t')
        {
            unicode = ' ';
            repeat = 3;
        }
        else if (!characters[unicode].renderable)
        {
            unicode = ' ';
            new_line = 1;
        }

        for (int r = 0; r < repeat; ++r)
        {
            GLfloat xpos = position.x + characters[unicode].bearing[0];
            GLfloat ypos = position.y - (characters[unicode].size[1] - characters[unicode].bearing[1]);
            GLfloat w = (GLfloat)characters[unicode].size[0];
            GLfloat h = (GLfloat)characters[unicode].size[1];

            if (cursor_pos && (c - advance) == cursor_index)
            {
                *cursor_pos = (vec2){ roundf(xpos), roundf(ypos) };
                cursor_filled = true;
            }
#if 1
            if (unicode != '\n' && unicode != '\t')
            {
                if (roundf(xpos) < min_x) min_x = roundf(xpos);
                if (roundf(xpos + w) > max_x) max_x = roundf(xpos + w);
                if (roundf(ypos) < min_y) min_y = roundf(ypos);
                if (roundf(ypos + h) > max_y) max_y = roundf(ypos + h);
            }
#endif
            if (new_line)
            {
                position.y -= font_info->font_size;
                position.x = start_position.x;
            }
            else
            {
                position.x += characters[unicode].advance >> 6;
            }
        }
    }
    if (!cursor_filled && cursor_pos)
    {
        *cursor_pos = (vec2){ position.x, position.y };
    }

    return (vec4) { min_x, min_y, max_x - min_x, max_y - min_y };
}

int
batch_render_fmt(Hobatch_Context* ctx, Font_Info* font_info, vec2 position, vec4 color, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    char buffer[1024] = { 0 };
    int len = vsnprintf(buffer, 1024, fmt, args);
    batch_render_text(ctx, font_info, buffer, len, 0, position, (vec4) { 0.0f, 0.0f, FLT_MAX, FLT_MAX }, color, 0, 0);

    va_end(args);

    return len;
}

int
batch_font_load(const char* filepath, Hobatch_Font* out_batch_font)
{
    const s32 px_sizes[] = { 8, 10, 12, 13, 14, 16 };
    s32 count = sizeof(px_sizes) / sizeof(*px_sizes);

    if (!out_batch_font) return -1;

    out_batch_font->name = 0;//strdup(filepath);
    out_batch_font->font_info = calloc(count, sizeof(Font_Info));
    for (s32 i = 0; i < count; ++i)
    {
        if (font_load(filepath, out_batch_font->font_info + i, px_sizes[i]) != FONT_LOAD_OK)
        {
            fprintf(stderr, "Failed to load font px: %d\n", px_sizes[i]);
            return -1;
        }
    }
    return 0;
}

void
batch_font_free(Hobatch_Font* batch_font)
{
    free(batch_font->name);
    free(batch_font->font_info);
}