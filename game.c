#include "game.h"

void
game_new(Game* game)
{
    for(s32 y = 0; y < 8; ++y)
        for(s32 x = 0; x < 8; ++x)
        {
            switch(y)
            {
                case 1:  game->board[y][x] = CHESS_WHITE_PAWN; break;
                case 6:  game->board[y][x] = CHESS_BLACK_PAWN; break;
                default: game->board[y][x] = CHESS_NONE; break;
            }
        }

    game->board[0][0] = CHESS_WHITE_ROOK;
    game->board[0][1] = CHESS_WHITE_KNIGHT;
    game->board[0][2] = CHESS_WHITE_BISHOP;
    game->board[0][3] = CHESS_WHITE_QUEEN;
    game->board[0][4] = CHESS_WHITE_KING;
    game->board[0][5] = CHESS_WHITE_BISHOP;
    game->board[0][6] = CHESS_WHITE_KNIGHT;
    game->board[0][7] = CHESS_WHITE_ROOK;

    game->board[7][0] = CHESS_BLACK_ROOK;
    game->board[7][1] = CHESS_BLACK_KNIGHT;
    game->board[7][2] = CHESS_BLACK_BISHOP;
    game->board[7][3] = CHESS_BLACK_QUEEN;
    game->board[7][4] = CHESS_BLACK_KING;
    game->board[7][5] = CHESS_BLACK_BISHOP;
    game->board[7][6] = CHESS_BLACK_KNIGHT;
    game->board[7][7] = CHESS_BLACK_ROOK;
}

int
game_move(Game* game, s32 from_x, s32 from_y, s32 to_x, s32 to_y)
{
    game->board[to_y][to_x] = game->board[from_y][from_x];
    game->board[from_y][from_x] = CHESS_NONE;

    return 1;
}