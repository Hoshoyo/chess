#pragma once
#include "os.h"
#include "gm.h"
#include <ft2build.h>
#include FT_FREETYPE_H

#if defined(_WIN32) || defined(_WIN64)
#define OS_DEFAULT_FONT "C:\\Windows\\Fonts\\consola.ttf"
#elif defined(__linux__) || defined(__APPLE__)
#define OS_DEFAULT_FONT "./res/fonts/LiberationMono-Regular.ttf"
#endif

typedef struct {
  vec2 botl, botr, topl, topr;
  s32 size[2];
  s32 bearing[2];
  u32 advance;
  bool renderable;
} Character;

typedef enum {
  FONT_LOAD_OK = 0,
  FONT_LOAD_ERROR_INIT_FREETYPE,
  FONT_LOAD_ERROR_FILE_FORMAT,
  FONT_LOAD_ERROR_SETTING_PIXEL_SIZE,
  FONT_LOAD_ERROR_LOADING,
} Font_Load_Status;

typedef struct {
  const s8* name;

  s64 font_size;
  s64 atlas_size;
  s64 max_height;
  s64 max_width;

  u32 flags;

  u32 atlas_full_id;

  u32 vao;
  u32 vbo;
  u32 ebo;

  u8* atlas_data;

  FT_Face face;
  Character characters[65536];
} Font_Info;

Font_Load_Status font_load(const s8* filepath, Font_Info* font, s32 pixel_point);
