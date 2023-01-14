#pragma once
#include "os.h"

typedef enum {
    CHESS_NONE = 0,

    CHESS_WHITE_KING,
    CHESS_WHITE_QUEEN,
    CHESS_WHITE_ROOK,
    CHESS_WHITE_KNIGHT,
    CHESS_WHITE_BISHOP,
    CHESS_WHITE_PAWN,

    CHESS_BLACK_KING,
    CHESS_BLACK_QUEEN,
    CHESS_BLACK_ROOK,
    CHESS_BLACK_KNIGHT,
    CHESS_BLACK_BISHOP,
    CHESS_BLACK_PAWN,
} Chess_Piece;

typedef struct {
    Chess_Piece board[8][8];
} Game;

void game_new(Game* game);
int  game_move(Game* game, s32 from_x, s32 from_y, s32 to_x, s32 to_y);