#pragma once
#include "os.h"
#include "batcher.h"
#include "game.h"
#include "gm.h"

typedef void* Chess_Interface;

typedef struct {
    char* server;
    int port;

    vec4 black_bg;
    vec4 white_bg;
} Chess_Config;

Chess_Interface interface_init();
void interface_update_window(Chess_Interface interf, s32 width, s32 height);
void interface_input(Chess_Interface interf, Game* game);
void interface_render(Chess_Interface interf, Hobatch_Context* ctx, Game* game);
void interface_destroy(Chess_Interface interf);

int parse_config(const char* data, Chess_Config* config);