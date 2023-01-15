#pragma once
#include "os.h"
#include "batcher.h"
#include "game.h"

typedef void* Chess_Interface;

Chess_Interface interface_init();
void interface_update_window(Chess_Interface interf, s32 width, s32 height);
void interface_input(Chess_Interface interf, Game* game);
void interface_render(Chess_Interface interf, Hobatch_Context* ctx, Game* game);
void interface_destroy(Chess_Interface interf);