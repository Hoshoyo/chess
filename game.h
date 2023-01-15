#pragma once
#include "os.h"

#define LAST_RANK 7
#define FIRST_RANK 0

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

typedef enum {
    PLAYER_NONE = 0,
    PLAYER_WHITE,
    PLAYER_BLACK,
    PLAYER_DRAW_STALEMATE,
    PLAYER_DRAW_INSUFFICIENT_MATERIAL,
    PLAYER_DRAW_THREE_FOLD_REPETITION,
    PLAYER_DRAW_50_MOVE,
} Player;

typedef struct {
    bool start;
    s32 from_x;
    s32 from_y;
    s32 to_x;
    s32 to_y;
    Chess_Piece promotion_piece;
    Chess_Piece moved_piece;
} Chess_Move;

typedef struct {
    Chess_Piece board[8][8];
    Chess_Piece sim_board[8][8];
    Chess_Piece last_board[8][8];

    Player winner;
    bool white_turn;
    bool white_long_castle_valid;
    bool white_short_castle_valid;
    bool black_long_castle_valid;
    bool black_short_castle_valid;
    Chess_Move last_move;
    s32 move_count;
    s32 move_draw_count;
} Game;

void game_new(Game* game);
int  game_move(Game* game, s32 from_x, s32 from_y, s32 to_x, s32 to_y, Chess_Piece promotion_choice, bool simulate);