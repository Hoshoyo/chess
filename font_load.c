#include "font_load.h"
#include "ho_gl.h"
#include <math.h>

const u32 FONT_FLAG_HAS_KERNING = 1 << 0;
const u32 FONT_FLAG_HAS_VERTICAL_METRICS = 1 << 1;

static u32
next_2_pow(u32 v)
{
  v--;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  v++;
  return v;
}

Font_Load_Status
font_load(const s8* filepath, Font_Info* font, s32 pixel_point)
{
  FT_Library library;

  s32 error = FT_Init_FreeType(&library);
  if (error)
    return FONT_LOAD_ERROR_INIT_FREETYPE;

  error = FT_New_Face(library, filepath, 0, &font->face);
  if (error == FT_Err_Unknown_File_Format)
    return FONT_LOAD_ERROR_FILE_FORMAT;
  else if (error)
    return FONT_LOAD_ERROR_LOADING;

  font->flags = 0;
  if (FT_HAS_VERTICAL(font->face))
  {
    // TODO(psv): vertical metrics
    font->flags |= FONT_FLAG_HAS_VERTICAL_METRICS;
  }
  if (FT_HAS_KERNING(font->face))
  {
    // TODO(psv): use this for something
    font->flags |= FONT_FLAG_HAS_KERNING;
  }

  error = FT_Set_Pixel_Sizes(font->face, 0, pixel_point);
  if (error)
  {
    return FONT_LOAD_ERROR_SETTING_PIXEL_SIZE;
  }

  s32 x_adv = 0, y_adv = 0;
  s32 previous_max_height = 0;

  s32 max_height = (font->face->size->metrics.ascender - font->face->size->metrics.descender) >> 6;
  s32 max_width = font->face->size->metrics.max_advance >> 6;
  s32 num_glyphs = font->face->num_glyphs;
  s32 num_glyphs_loaded = 0;
  s32 size = (s32) next_2_pow((s32) sqrtf((r32) (max_width * num_glyphs)));
  r32 atlasf_size = (r32) size;
  font->atlas_size = size;

  font->font_size = pixel_point;
  font->max_height = max_height;
  font->max_width = max_width;

  // allocate memory for the texture atlas of the font
  font->atlas_data = (u8 *) calloc(1, size * size * 4);

  for (u32 i = 0; i < 1024; ++i)
  {
    u32 index = FT_Get_Char_Index(font->face, i);
    error = FT_Load_Glyph(font->face, index, FT_LOAD_RENDER);

    if (index == 0)
    {
      continue;
    }

    s32 width = font->face->glyph->bitmap.width;
    s32 height = font->face->glyph->bitmap.rows;

    if (width && height)
    {
      // if got to the end of the first row, reset x_advance and sum y
      if (x_adv + width >= size)
      {
        y_adv += previous_max_height;
        x_adv = 0;
      }
      // copy from the FT bitmap to atlas in the correct position
      for (s32 h = 0; h < height; ++h)
      {
        u8 *b = font->face->glyph->bitmap.buffer;
        memcpy(font->atlas_data + (size * (h + y_adv)) + x_adv, b + width * h, width);
      }
      font->characters[i].botl = (vec2) {x_adv / atlasf_size, y_adv / atlasf_size};
      font->characters[i].botr = (vec2) {(x_adv + width) / atlasf_size, y_adv / atlasf_size};
      font->characters[i].topl = (vec2) {x_adv / atlasf_size, (y_adv + height) / atlasf_size};
      font->characters[i].topr = (vec2) {(x_adv + width) / atlasf_size, (y_adv + height) / atlasf_size};

      // keep the packing by getting the max height of the previous row of packing
      if (height > previous_max_height)
        previous_max_height = height;
      x_adv += width;
    }
    // this glyph exists
    font->characters[i].renderable = 1;
    font->characters[i].advance = font->face->glyph->advance.x;
    font->characters[i].size[0] = font->face->glyph->bitmap.width;
    font->characters[i].size[1] = font->face->glyph->bitmap.rows;
    font->characters[i].bearing[0] = font->face->glyph->bitmap_left;
    font->characters[i].bearing[1] = font->face->glyph->bitmap_top;

    num_glyphs_loaded += 1;
  }

  FT_Done_Face(font->face);
  FT_Done_FreeType(library);

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glGenTextures(1, &font->atlas_full_id);
  glBindTexture(GL_TEXTURE_2D, font->atlas_full_id);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, size, size, 0, GL_RED, GL_UNSIGNED_BYTE, font->atlas_data);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

  free(font->atlas_data);

  return FONT_LOAD_OK;
}
