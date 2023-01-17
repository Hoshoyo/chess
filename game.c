#include "game.h"
#include <light_array.h>

#define MAX(A, B) (((A) > (B)) ? (A) : (B))
#define MIN(A, B) (((A) < (B)) ? (A) : (B))

typedef struct {
    Chess_Move* move;
} Gen_Moves;

s32 generate_possible_moves(Game* game, Gen_Moves* moves);
bool check_sufficient_material(Game* game);
bool check_repetition(Game* game);

void
game_standard_board(Game* game)
{
    memset(game->board, 0, sizeof(game->board));
    game->move_draw_count = 0;

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

    game->last_move.start = true;
    game->white_turn = true;
    game->white_long_castle_valid = true;
    game->white_short_castle_valid = true;
    game->black_long_castle_valid = true;
    game->black_short_castle_valid = true;

    game->winner = PLAYER_NONE;
}

void
game_queen_checkmate_board(Game* game)
{
    memset(game->board, 0, sizeof(game->board));
    game->move_draw_count = 0;

    game->board[2][2] = CHESS_WHITE_KING;
    game->board[4][2] = CHESS_WHITE_QUEEN;
    game->board[2][6] = CHESS_BLACK_KING;

    game->winner = PLAYER_NONE;
}

void
game_new(Game* game)
{
    game_standard_board(game);
    //game_queen_checkmate_board(game);

    game->black_time_ms = 1000 * 60 * 5;
    game->white_time_ms = 1000 * 60 * 5;

    game->increment_ms = 1000;

    game->clock = 0;

    if(game->history) {
        array_free(((Game_History*)game->history)->game);
        free(game->history);
    }
    game->history = calloc(1, sizeof(Game_History));

    ((Game_History*)game->history)->game = array_new(Game);
    ((Game_History*)game->history)->repetition_index_check = 0;
    array_push(((Game_History*)game->history)->game, *game);
}

static bool
is_white(Chess_Piece piece)
{
    switch(piece)
    {
        case CHESS_WHITE_KING:   return true;
        case CHESS_WHITE_QUEEN:  return true;
        case CHESS_WHITE_ROOK:   return true;
        case CHESS_WHITE_KNIGHT: return true;
        case CHESS_WHITE_BISHOP: return true;
        case CHESS_WHITE_PAWN:   return true;
        case CHESS_BLACK_KING:   return false;
        case CHESS_BLACK_QUEEN:  return false;
        case CHESS_BLACK_ROOK:   return false;
        case CHESS_BLACK_KNIGHT: return false;
        case CHESS_BLACK_BISHOP: return false;
        case CHESS_BLACK_PAWN:   return false;
        default: return false; break;
    }
}
static bool
is_black(Chess_Piece piece)
{
    return !is_white(piece) && piece != CHESS_NONE;
}

#define BLACK_ATTACK (1 << 0)
#define WHITE_ATTACK (1 << 1)

static s32
flag_diag_attack(Chess_Piece piece, s32 distance, bool* searching)
{
    s32 result = 0;
    switch (piece) {
        case CHESS_WHITE_QUEEN:
        case CHESS_WHITE_BISHOP: {
            result |= WHITE_ATTACK;
            *searching = false;
        } break;
        case CHESS_WHITE_KING:
        case CHESS_WHITE_PAWN: {
            if (distance == 0) {
                result |= WHITE_ATTACK;
            }
            *searching = false;
        } break;
        case CHESS_BLACK_QUEEN:
        case CHESS_BLACK_BISHOP: {
            result |= BLACK_ATTACK;
            *searching = false;
        } break;
        case CHESS_BLACK_KING:
        case CHESS_BLACK_PAWN: {
            if (distance == 0) {
                result |= BLACK_ATTACK;
            }
            *searching = false;
        } break;
        case CHESS_NONE: break;
        default: {
            *searching = false;
        } break;
    }
    return result;
}

static s32
flag_horiz_attack(Chess_Piece piece, s32 distance, bool* searching)
{
    s32 result = 0;
    switch(piece) {
        case CHESS_WHITE_ROOK:
        case CHESS_WHITE_QUEEN: {
            result |= WHITE_ATTACK;
            *searching = false;
        } break;
        case CHESS_WHITE_KING: {
            if (distance == 0) {
                result |= WHITE_ATTACK;
            }
            *searching = false;
        } break;
        case CHESS_BLACK_ROOK:
        case CHESS_BLACK_QUEEN: {
            result |= BLACK_ATTACK;
            *searching = false;
        } break;
        case CHESS_BLACK_KING: {
            if (distance == 0) {
                result |= BLACK_ATTACK;
            }
            *searching = false;
        } break;
        case CHESS_NONE: break;
        default: {
            *searching = false;
        } break;
    }
    return result;
}

static bool
inside_board(s32 x, s32 y)
{
    return (x >= 0 && x < 8) && (y >= 0 && y < 8);
}

static s32
square_attacked(Chess_Piece* board, s32 x, s32 y)
{
    s32 result = 0;

    // Check diagonal top left
    bool searching = true;
    for (s32 ax = x - 1, ay = y + 1, c = 0; ax >= 0 && ay < 8 && searching; --ax, ++ay, ++c) {
        result |= flag_diag_attack(board[ay * 8 + ax], c, &searching);
    }

    // Check diagonal  right
    searching = true;
    for (s32 ax = x + 1, ay = y + 1, c = 0; ax < 8 && ay < 8 && searching; ++ax, ++ay, ++c) {
        result |= flag_diag_attack(board[ay * 8 + ax], c, &searching);
    }

    // Check diagonal bot left
    searching = true;
    for (s32 ax = x - 1, ay = y - 1, c = 0; ax >= 0 && ay >= 0 && searching; --ax, --ay, ++c) {
        result |= flag_diag_attack(board[ay * 8 + ax], c, &searching);
    }

    // Check diagonal bot right
    searching = true;
    for (s32 ax = x + 1, ay = y - 1, c = 0; ax < 8 && ay >= 0 && searching; ++ax, --ay, ++c) {
        result |= flag_diag_attack(board[ay * 8 + ax], c, &searching);
    }

    // Check horizontal right
    searching = true;
    for (s32 ax = x + 1, c = 0; ax < 8 && searching; ++ax, ++c) {
        result |= flag_horiz_attack(board[y * 8 + ax], c, &searching);
    }

    // Check horizontal left
    searching = true;
    for (s32 ax = x - 1, c = 0; ax >= 0 && searching; --ax, ++c) {
        result |= flag_horiz_attack(board[y * 8 + ax], c, &searching);
    }

    // Check vertical top
    searching = true;
    for (s32 ay = y + 1, c = 0; ay < 8 && searching; ++ay, ++c) {
        result |= flag_horiz_attack(board[ay * 8 + x], c, &searching);
    }

    // Check vertical bottom
    searching = true;
    for (s32 ay = y - 1, c = 0; ay >= 0 && searching; --ay, ++c) {
        result |= flag_horiz_attack(board[ay * 8 + x], c, &searching);
    }

    // Check knights
    if (inside_board(x + 1, y + 2) && board[(y + 2) * 8 + x + 1] == CHESS_WHITE_KNIGHT) {
        result |= WHITE_ATTACK;
    }
    if (inside_board(x - 1, y + 2) && board[(y + 2) * 8 + x - 1] == CHESS_WHITE_KNIGHT) {
        result |= WHITE_ATTACK;
    }
    if (inside_board(x + 1, y - 2) && board[(y - 2) * 8 + x + 1] == CHESS_WHITE_KNIGHT) {
        result |= WHITE_ATTACK;
    }
    if (inside_board(x - 1, y - 2) && board[(y - 2) * 8 + x - 1] == CHESS_WHITE_KNIGHT) {
        result |= WHITE_ATTACK;
    }
    if (inside_board(x + 2, y + 1) && board[(y + 1) * 8 + x + 2] == CHESS_WHITE_KNIGHT) {
        result |= WHITE_ATTACK;
    }
    if (inside_board(x - 2, y + 1) && board[(y + 1) * 8 + x - 2] == CHESS_WHITE_KNIGHT) {
        result |= WHITE_ATTACK;
    }
    if (inside_board(x + 2, y - 1) && board[(y - 1) * 8 + x + 2] == CHESS_WHITE_KNIGHT) {
        result |= WHITE_ATTACK;
    }
    if (inside_board(x - 2, y - 1) && board[(y - 1) * 8 + x - 2] == CHESS_WHITE_KNIGHT) {
        result |= WHITE_ATTACK;
    }

    if (inside_board(x + 1, y + 2) && board[(y + 2) * 8 + x + 1] == CHESS_BLACK_KNIGHT) {
        result |= BLACK_ATTACK;
    }
    if (inside_board(x - 1, y + 2) && board[(y + 2) * 8 + x - 1] == CHESS_BLACK_KNIGHT) {
        result |= BLACK_ATTACK;
    }
    if (inside_board(x + 1, y - 2) && board[(y - 2) * 8 + x + 1] == CHESS_BLACK_KNIGHT) {
        result |= BLACK_ATTACK;
    }
    if (inside_board(x - 1, y - 2) && board[(y - 2) * 8 + x - 1] == CHESS_BLACK_KNIGHT) {
        result |= BLACK_ATTACK;
    }
    if (inside_board(x + 2, y + 1) && board[(y + 1) * 8 + x + 2] == CHESS_BLACK_KNIGHT) {
        result |= BLACK_ATTACK;
    }
    if (inside_board(x - 2, y + 1) && board[(y + 1) * 8 + x - 2] == CHESS_BLACK_KNIGHT) {
        result |= BLACK_ATTACK;
    }
    if (inside_board(x + 2, y - 1) && board[(y - 1) * 8 + x + 2] == CHESS_BLACK_KNIGHT) {
        result |= BLACK_ATTACK;
    }
    if (inside_board(x - 2, y - 1) && board[(y - 1) * 8 + x - 2] == CHESS_BLACK_KNIGHT) {
        result |= BLACK_ATTACK;
    }

    return result;
}

static bool
white_in_check(Game* game, bool sim)
{
    Chess_Piece* board = (sim) ? (Chess_Piece*)game->sim_board : (Chess_Piece*)game->board;
    for (s32 y = 0; y < 8; ++y)
        for (s32 x = 0; x < 8; ++x)
        {
            if (board[y * 8 + x] == CHESS_WHITE_KING)
                return square_attacked(board, x, y) & BLACK_ATTACK;
        }
    return false;
}

static bool
black_in_check(Game* game, bool sim)
{
    Chess_Piece* board = (sim) ? (Chess_Piece*)game->sim_board : (Chess_Piece*)game->board;
    for (s32 y = 0; y < 8; ++y)
        for (s32 x = 0; x < 8; ++x)
        {
            if (board[y * 8 + x] == CHESS_BLACK_KING)
                return square_attacked(board, x, y) & WHITE_ATTACK;
        }
    return false;
}

static bool
white_en_passant(Game* game, s32 from_x, s32 from_y, s32 to_x, s32 to_y)
{
    Chess_Piece piece = game->board[from_y][from_x];
    if (piece == CHESS_WHITE_PAWN && game->last_move.moved_piece == CHESS_BLACK_PAWN && from_y == 4 && game->last_move.from_y == 6 && abs(game->last_move.from_y - game->last_move.to_y) == 2) {
        return (abs(from_x - game->last_move.from_x) == 1) && (to_x == game->last_move.to_x);
    }
    return false;
}

static bool
black_en_passant(Game* game, s32 from_x, s32 from_y, s32 to_x, s32 to_y)
{
    Chess_Piece piece = game->board[from_y][from_x];
    if (piece == CHESS_BLACK_PAWN && game->last_move.moved_piece == CHESS_WHITE_PAWN && from_y == 3 && game->last_move.from_y == 1 && abs(game->last_move.from_y - game->last_move.to_y) == 2) {
        return (abs(from_x - game->last_move.from_x) == 1) && (to_x == game->last_move.to_x);
    }
    return false;
}

static bool
is_valid_move(Game* game, s32 from_x, s32 from_y, s32 to_x, s32 to_y, Chess_Piece promotion_piece)
{
    if(!inside_board(from_x, from_y) || !inside_board(to_x, to_y))
        return false;

    Chess_Piece piece = game->board[from_y][from_x];
    Chess_Piece to_piece = game->board[to_y][to_x];

    switch (piece)
    {
        case CHESS_WHITE_PAWN: {
            if (to_y - from_y == 1 && to_x == from_x) {
                // Move 1 square forward
                return (to_piece == CHESS_NONE); // Can promote too
            } else if (to_y - from_y == 2 && to_x == from_x) {
                // Move 2 squares forward
                return (game->board[to_y - 1][to_x] == CHESS_NONE) && (to_piece == CHESS_NONE);
            } else if (to_y - from_y == 1 && to_x - from_x == 1 || to_y - from_y == 1 && to_x - from_x == -1) {
                if (to_y == LAST_RANK) {
                    // Promotion with capture
                    return is_black(to_piece);
                } else {
                    // Normal capture
                    return (is_black(to_piece)) || white_en_passant(game, from_x, from_y, to_x, to_y);
                }
            }
        } break;
        case CHESS_BLACK_PAWN: {
            if (from_y - to_y == 1 && to_x == from_x) {
                // Move 1 square forward
                return (to_piece == CHESS_NONE); // Can promote too
            } else if (from_y - to_y == 2 && to_x == from_x) {
                // Move 2 squares forward
                return (game->board[to_y + 1][to_x] == CHESS_NONE) && (to_piece == CHESS_NONE);
            } else if (from_y - to_y == 1 && to_x - from_x == 1 || from_y - to_y == 1 && to_x - from_x == -1) {
                if (to_y == FIRST_RANK) {
                    // Promotion with capture
                    return is_white(to_piece);
                } else {
                    // Normal capture
                    return (is_white(to_piece)) || black_en_passant(game, from_x, from_y, to_x, to_y);
                }
            }
        } break;
        case CHESS_BLACK_BISHOP:
        case CHESS_WHITE_BISHOP: {
            if (abs(from_x - to_x) != abs(from_y - to_y))
                return false;
            s32 ydir = (from_y < to_y) ? 1 : -1;
            s32 xdir = (from_x < to_x) ? 1 : -1;
            s32 count = abs(from_y - to_y) - 1;
            for (s32 y = from_y + ydir, x = from_x + xdir, c = 0; c < count; y += ydir, x += xdir, ++c)
            {
                if (game->board[y][x] != CHESS_NONE)
                    return false;
            }
            if(piece == CHESS_BLACK_BISHOP)
                return (is_white(to_piece) || to_piece == CHESS_NONE);
            else
                return (is_black(to_piece) || to_piece == CHESS_NONE);
        } break;

        case CHESS_BLACK_KNIGHT:
        case CHESS_WHITE_KNIGHT: {
            if (from_y + 2 == to_y) {
                if (!(from_x + 1 == to_x || from_x - 1 == to_x))
                    return false;
            } else if (from_y - 2 == to_y) {
                if (!(from_x + 1 == to_x || from_x - 1 == to_x))
                    return false;
            } else if (from_y + 1 == to_y) {
                if (!(from_x + 2 == to_x || from_x - 2 == to_x))
                    return false;
            } else if (from_y - 1 == to_y) {
                if (!(from_x + 2 == to_x || from_x - 2 == to_x))
                    return false;
            } else {
                return false;
            }
            if (piece == CHESS_BLACK_KNIGHT)
                return (is_white(to_piece) || to_piece == CHESS_NONE);
            else
                return (is_black(to_piece) || to_piece == CHESS_NONE);
        } break;
        case CHESS_WHITE_ROOK:
        case CHESS_BLACK_ROOK: {
            if (from_x == to_x && from_y != to_y) {
                s32 ydir = (from_y < to_y) ? 1 : -1;
                s32 count = abs(from_y - to_y) - 1;
                for (s32 y = from_y + ydir, c = 0; c < count; y += ydir, ++c)
                {
                    if (game->board[y][from_x] != CHESS_NONE)
                        return false;
                }
            } else if(from_y == to_y && from_x != to_x) {
                s32 xdir = (from_x < to_x) ? 1 : -1;
                s32 count = abs(from_x - to_x) - 1;
                for (s32 x = from_x + xdir, c = 0; c < count; x += xdir, ++c)
                {
                    if (game->board[from_y][x] != CHESS_NONE)
                        return false;
                }
            } else {
                return false;
            }
            if (piece == CHESS_BLACK_ROOK)
                return (is_white(to_piece) || to_piece == CHESS_NONE);
            else
                return (is_black(to_piece) || to_piece == CHESS_NONE);
        } break;
        case CHESS_WHITE_KING: {
            if (abs(from_y - to_y) == 1 || abs(from_x - to_x) == 1) {
                // Normal move
                return (is_black(to_piece) || to_piece == CHESS_NONE);
            } else if (from_x == 4 && to_x == 2) {
                // Castle long
                if (!game->white_long_castle_valid)
                    return false;
                if(!(game->board[from_y][from_x-1] == CHESS_NONE && game->board[from_y][from_x - 2] == CHESS_NONE))
                    return false;

                return !((square_attacked((Chess_Piece*)game->board, from_x - 1, from_y) & BLACK_ATTACK) || (square_attacked((Chess_Piece*)game->board, from_x - 2, from_y) & BLACK_ATTACK));
            } else if (from_x == 4 && to_x == 6) {
                // Casle short
                if (!game->white_short_castle_valid)
                    return false;
                if(!(game->board[from_y][from_x + 1] == CHESS_NONE && game->board[from_y][from_x + 2] == CHESS_NONE))
                    return false;

                return !((square_attacked((Chess_Piece*)game->board, from_x + 1, from_y) & BLACK_ATTACK) || (square_attacked((Chess_Piece*)game->board, from_x + 2, from_y) & BLACK_ATTACK));
            }
        } break;
        case CHESS_BLACK_KING: {
            if (abs(from_y - to_y) == 1 || abs(from_x - to_x) == 1) {
                // Normal move
                return (is_white(to_piece) || to_piece == CHESS_NONE);
            } else if (from_x == 4 && to_x == 2) {
                // Castle long
                if (!game->black_long_castle_valid)
                    return false;
                if(!(game->board[from_y][from_x - 1] == CHESS_NONE && game->board[from_y][from_x - 2] == CHESS_NONE))
                    return false;
                // TODO(psv): check if the squares are attacked
                return !((square_attacked((Chess_Piece*)game->board, from_x - 1, from_y) & WHITE_ATTACK) || (square_attacked((Chess_Piece*)game->board, from_x - 2, from_y) & WHITE_ATTACK));
            } else if (from_x == 4 && to_x == 6) {
                // Casle short
                if (!game->black_short_castle_valid)
                    return false;
                if(!(game->board[from_y][from_x + 1] == CHESS_NONE && game->board[from_y][from_x + 2] == CHESS_NONE))
                    return false;
                
                return !((square_attacked((Chess_Piece*)game->board, from_x + 1, from_y) & WHITE_ATTACK) || (square_attacked((Chess_Piece*)game->board, from_x + 2, from_y) & WHITE_ATTACK));
            }
        } break;
        case CHESS_BLACK_QUEEN:
        case CHESS_WHITE_QUEEN: {
            bool valid_bishop = true;
            bool valid_rook = true;

            // Bishop check
            if (abs(from_x - to_x) != abs(from_y - to_y))
                valid_bishop = false;
            s32 ydir = (from_y < to_y) ? 1 : -1;
            s32 xdir = (from_x < to_x) ? 1 : -1;
            s32 count = abs(from_y - to_y) - 1;
            for (s32 y = from_y + ydir, x = from_x + xdir, c = 0; c < count; y += ydir, x += xdir, ++c)
            {
                if (game->board[y][x] != CHESS_NONE)
                    valid_bishop = false;
            }

            // Rook check
            if (from_x == to_x && from_y != to_y) {
                s32 ydir = (from_y < to_y) ? 1 : -1;
                s32 count = abs(from_y - to_y) - 1;
                for (s32 y = from_y + ydir, c = 0; c < count; y += ydir, ++c)
                {
                    if (game->board[y][from_x] != CHESS_NONE)
                        valid_rook = false;
                }
            } else if(from_y == to_y && from_x != to_x) {
                s32 xdir = (from_x < to_x) ? 1 : -1;
                s32 count = abs(from_x - to_x) - 1;
                for (s32 x = from_x + xdir, c = 0; c < count; x += xdir, ++c)
                {
                    if (game->board[from_y][x] != CHESS_NONE)
                        valid_rook = false;
                }
            } else {
                valid_rook = false;
            }

            if (!valid_bishop && !valid_rook)
                return false;
            
            if (piece == CHESS_BLACK_QUEEN)
                return (is_white(to_piece) || to_piece == CHESS_NONE);
            else
                return (is_black(to_piece) || to_piece == CHESS_NONE);
        } break;
    }

    return false;
}

bool
game_move(Game* game, s32 from_x, s32 from_y, s32 to_x, s32 to_y, Chess_Piece promotion_choice, bool simulate)
{
    // Copy the state so we can simulate
    memcpy(game->sim_board, game->board, sizeof(game->board));

    Chess_Piece from_piece = game->board[from_y][from_x];
    Chess_Piece to_piece = game->board[to_y][to_x];

    if (from_piece == CHESS_NONE) {
        return false;
    }

    if (game->winner != PLAYER_NONE)
        return false;

    Chess_Piece new_piece = from_piece;
    if(from_piece == CHESS_WHITE_PAWN && to_y == LAST_RANK) {
        if (promotion_choice == CHESS_WHITE_QUEEN || promotion_choice == CHESS_WHITE_ROOK || promotion_choice == CHESS_WHITE_KNIGHT || promotion_choice == CHESS_WHITE_BISHOP)
            new_piece = promotion_choice;
        else
            return false; // Invalid promotion of black piece
    }
    if(from_piece == CHESS_BLACK_PAWN && to_y == FIRST_RANK) {
        if (promotion_choice == CHESS_BLACK_QUEEN || promotion_choice == CHESS_BLACK_ROOK || promotion_choice == CHESS_BLACK_KNIGHT || promotion_choice == CHESS_BLACK_BISHOP)
            new_piece = promotion_choice;
        else
            return false; // Invalid promotion of black piece
    }

    // Check if who is moving is in fact who's turn it is
    if(!(game->white_turn && is_white(from_piece) || !game->white_turn && is_black(from_piece)))
        return false; // Invalid move, not the pieces turn

    bool valid = is_valid_move(game, from_x, from_y, to_x, to_y, promotion_choice);
    if (!valid) return false;

    // Check castle while in check
    if (from_piece == CHESS_WHITE_KING && abs(from_x - to_x) == 2 && white_in_check(game, false))
        return false;
    if (from_piece == CHESS_BLACK_KING && abs(from_x - to_x) == 2 && black_in_check(game, false))
        return false;

    // Simulate the move
    game->sim_board[to_y][to_x] = new_piece;
    game->sim_board[from_y][from_x] = CHESS_NONE;

    // Do all the checks in the simulation
    if (game->white_turn && white_in_check(game, true))
        return false;
    if (!game->white_turn && black_in_check(game, true))
        return false;

    if(!simulate) 
    {
        bool captured = false;

        // After all checks, perform the move
        if (game->white_turn && white_en_passant(game, from_x, from_y, to_x, to_y)){
            game->board[to_y - 1][to_x] = CHESS_NONE;
            captured = true;
        }
        if (!game->white_turn && black_en_passant(game, from_x, from_y, to_x, to_y)) {
            game->board[to_y + 1][to_x] = CHESS_NONE;
            captured = true;
        }

        if(game->board[to_y][to_x] != CHESS_NONE)
            captured = true;

        game->board[to_y][to_x] = new_piece;
        game->board[from_y][from_x] = CHESS_NONE;

        // Saves the last move to check en passant
        game->last_move.start = false;
        game->last_move.promotion_piece = promotion_choice;
        game->last_move.from_x = from_x;
        game->last_move.from_y = from_y;
        game->last_move.to_x = to_x;
        game->last_move.to_y = to_y;
        game->last_move.moved_piece = from_piece;

        if(from_piece == CHESS_WHITE_KING) {
            if(from_x - to_x == 2) {
                // Castle long
                game->board[to_y][to_x + 1] = CHESS_WHITE_ROOK;
                game->board[to_y][0] = CHESS_NONE;
            } else if (from_x - to_x == -2) {
                // Castle short
                game->board[to_y][to_x - 1] = CHESS_WHITE_ROOK;
                game->board[to_y][7] = CHESS_NONE;
            }
            game->white_long_castle_valid = false;
            game->white_short_castle_valid = false;
        }
        if(from_piece == CHESS_BLACK_KING) {
            if(from_x - to_x == 2) {
                // Castle long
                game->board[to_y][to_x + 1] = CHESS_BLACK_ROOK;
                game->board[to_y][0] = CHESS_NONE;
            } else if (from_x - to_x == -2) {
                // Castle short
                game->board[to_y][to_x - 1] = CHESS_BLACK_ROOK;
                game->board[to_y][7] = CHESS_NONE;
            }
            game->black_long_castle_valid = false;
            game->black_short_castle_valid = false;
        }
        if (from_piece == CHESS_WHITE_ROOK) {
            if (game->white_short_castle_valid && from_x == 7 && from_y == 0)
                game->white_short_castle_valid = false;
            if (game->white_long_castle_valid && from_x == 0 && from_y == 0)
                game->white_long_castle_valid = false;
        }
        if (from_piece == CHESS_BLACK_ROOK) {
            if (game->black_short_castle_valid && from_x == 7 && from_y == 7)
                game->black_short_castle_valid = false;
            if (game->black_long_castle_valid && from_x == 0 && from_y == 7)
                game->black_long_castle_valid = false;
        }

        if(from_piece == CHESS_WHITE_PAWN || from_piece == CHESS_BLACK_PAWN || captured)
            game->move_draw_count = 0;
        else
            game->move_draw_count++;

        if(game->white_turn)
            game->white_time_ms += game->increment_ms;
        else
            game->black_time_ms += game->increment_ms;

        game->move_count++;

        // Pass the turn
        game->white_turn = !game->white_turn;

        Gen_Moves moves = {0};
        s32 count_moves = generate_possible_moves(game, &moves);
        s32 mv_count = 0;
        for(int i = 0; i < array_length(moves.move); ++i) {
            if(game_move(game, moves.move[i].from_x, moves.move[i].from_y, moves.move[i].to_x, moves.move[i].to_y, moves.move[i].promotion_piece, true)) {
                mv_count++;
            }
        }
        array_free(moves.move);

        if(mv_count == 0) {
            if(!game->white_turn) {
                if(black_in_check(game, false)) {
                    printf("Checkmate, white wins by checkmate\n");
                    game->winner = PLAYER_WHITE;
                } else {
                    printf("Draw by stalemate\n");
                    game->winner = PLAYER_DRAW_STALEMATE;
                }
            }
            else {
                if(white_in_check(game, false)) {
                    printf("Checkmate, black wins by checkmate\n");
                    game->winner = PLAYER_BLACK;
                } else {
                    printf("Draw by stalemate\n");
                    game->winner = PLAYER_DRAW_STALEMATE;
                }
            }
        } else if(game->move_draw_count == 50 * 2) {
            printf("Draw by 50 move rule\n");
            game->winner = PLAYER_DRAW_50_MOVE;
        } else if(!check_sufficient_material(game)) {
            printf("Draw by insufficient material\n");
            game->winner = PLAYER_DRAW_INSUFFICIENT_MATERIAL;
        }

        // Save history
        array_push(((Game_History*)game->history)->game, *game);
        if(game->move_draw_count == 0)
            ((Game_History*)game->history)->repetition_index_check = array_length(((Game_History*)game->history)->game) - 1;

        if(check_repetition(game)) {
            printf("Draw by repetition\n");
            game->winner = PLAYER_DRAW_THREE_FOLD_REPETITION;
        }
    }

    return true;
}

static void 
generate_pawn_moves(Game* game, s32 x, s32 y, Gen_Moves* moves) 
{
	Chess_Move mv = {0};
	mv.from_y = y;
	mv.from_x = x;

	s8 move_direction = 1;
	s8 black_piece_flag = 0;
	s8 seventh_rank = 6;
	if (!game->white_turn) {
		move_direction = -1;
		black_piece_flag = 8;
		seventh_rank = 2;
	}

	// advance once
	{
		mv.to_y = y;
		mv.to_x = y + move_direction;

		// advance once promote
		if (y == seventh_rank) {
			mv.promotion_piece = (move_direction == 1) ? CHESS_WHITE_QUEEN : CHESS_BLACK_QUEEN;
            if(is_valid_move(game, mv.from_x, mv.from_y, mv.to_x, mv.to_y, mv.promotion_piece))
            {
                array_push(moves->move, mv);
                mv.promotion_piece = (move_direction == 1) ? CHESS_WHITE_ROOK : CHESS_BLACK_ROOK;
                array_push(moves->move, mv);
                mv.promotion_piece = (move_direction == 1) ? CHESS_WHITE_BISHOP : CHESS_BLACK_BISHOP;
                array_push(moves->move, mv);
                mv.promotion_piece = (move_direction == 1) ? CHESS_WHITE_KNIGHT : CHESS_BLACK_KNIGHT;
                array_push(moves->move, mv);
            }
		} else {
			if(is_valid_move(game, mv.from_x, mv.from_y, mv.to_x, mv.to_y, mv.promotion_piece))
                array_push(moves->move, mv);
		}
	}

	// advance twice
	{
		mv.promotion_piece = CHESS_NONE;
		mv.to_x = x;
		mv.to_y = y + 2 * move_direction;
		if(is_valid_move(game, mv.from_x, mv.from_y, mv.to_x, mv.to_y, mv.promotion_piece))
			array_push(moves->move, mv);
	}

	// capture normal - en passant
	{
		mv.to_y = y + move_direction;
		if (y == seventh_rank) {
			// capture promote
			mv.to_x = x + 1;
			mv.promotion_piece = (move_direction == 1) ? CHESS_WHITE_KNIGHT : CHESS_BLACK_KNIGHT;
			if(is_valid_move(game, mv.from_x, mv.from_y, mv.to_x, mv.to_y, mv.promotion_piece)) {
				array_push(moves->move, mv);
                mv.promotion_piece = (move_direction == 1) ? CHESS_WHITE_ROOK : CHESS_BLACK_ROOK;
                array_push(moves->move, mv);
                mv.promotion_piece = (move_direction == 1) ? CHESS_WHITE_BISHOP : CHESS_BLACK_BISHOP;
                array_push(moves->move, mv);
                mv.promotion_piece = (move_direction == 1) ? CHESS_WHITE_KNIGHT : CHESS_BLACK_KNIGHT;
                array_push(moves->move, mv);
			}

            mv.to_x = x - 1;
			mv.promotion_piece = (move_direction == 1) ? CHESS_WHITE_KNIGHT : CHESS_BLACK_KNIGHT;
			if(is_valid_move(game, mv.from_x, mv.from_y, mv.to_x, mv.to_y, mv.promotion_piece)) {
				array_push(moves->move, mv);
                mv.promotion_piece = (move_direction == 1) ? CHESS_WHITE_ROOK : CHESS_BLACK_ROOK;
                array_push(moves->move, mv);
                mv.promotion_piece = (move_direction == 1) ? CHESS_WHITE_BISHOP : CHESS_BLACK_BISHOP;
                array_push(moves->move, mv);
                mv.promotion_piece = (move_direction == 1) ? CHESS_WHITE_KNIGHT : CHESS_BLACK_KNIGHT;
                array_push(moves->move, mv);
			}
		} else {
			mv.promotion_piece = CHESS_NONE;

			mv.to_x = x + 1;
            if(is_valid_move(game, mv.from_x, mv.from_y, mv.to_x, mv.to_y, mv.promotion_piece)) {
                array_push(moves->move, mv);
            }

			mv.to_x = x - 1;
            if(is_valid_move(game, mv.from_x, mv.from_y, mv.to_x, mv.to_y, mv.promotion_piece)) {
                array_push(moves->move, mv);
            }
		}
	}
}

static void 
generate_bishop_moves(Game* game, s32 x, s32 y, Gen_Moves* moves) 
{
	Chess_Move mv = {0};
	mv.from_x = x;
	mv.from_y = y;

	// top left
	for (s32 i = y + 1, j = x -1 ; i < 8 && j >= 0; ++i, --j) {
		mv.to_x = j;
		mv.to_y = i;
		if(is_valid_move(game, mv.from_x, mv.from_y, mv.to_x, mv.to_y, mv.promotion_piece)) {
			array_push(moves->move, mv);
		} else
			break;
	}

	// top right
	for (s32 i = y + 1, j = x + 1; i < 8 && j < 8; ++i, ++j) {
		mv.to_x = j;
		mv.to_y = i;
		if(is_valid_move(game, mv.from_x, mv.from_y, mv.to_x, mv.to_y, mv.promotion_piece)) {
			array_push(moves->move, mv);
		} else
			break;
	}

	// bot left
	for (s32 i = y - 1, j = x - 1; i >= 0 && j >= 0; --i, --j) {
		mv.to_x = j;
		mv.to_y = i;
		if(is_valid_move(game, mv.from_x, mv.from_y, mv.to_x, mv.to_y, mv.promotion_piece)) {
			array_push(moves->move, mv);
		}
		else
			break;
	}

	// bot right
	for (s32 i = y - 1, j = x + 1; i >= 0, j < 8; --i, ++j) {
		mv.to_x = j;
		mv.to_y = i;
		if(is_valid_move(game, mv.from_x, mv.from_y, mv.to_x, mv.to_y, mv.promotion_piece))  {
			array_push(moves->move, mv);
		} else
			break;
	}
}

static void 
generate_knight_moves(Game* game, s32 x, s32 y, Gen_Moves* moves)
{
	Chess_Move mv = {0};
	mv.from_x = x;
	mv.from_y = y;

	// L up left
	mv.to_x = x - 1;
	mv.to_y = y + 2;
	if(is_valid_move(game, mv.from_x, mv.from_y, mv.to_x, mv.to_y, mv.promotion_piece)) {
		array_push(moves->move, mv);
	}

	// L up right
	mv.to_x = x + 1;
	mv.to_y = y + 2;
	if(is_valid_move(game, mv.from_x, mv.from_y, mv.to_x, mv.to_y, mv.promotion_piece)) {
		array_push(moves->move, mv);
	}

	// L down left
	mv.to_x = x - 1;
	mv.to_y = y - 2;
	if(is_valid_move(game, mv.from_x, mv.from_y, mv.to_x, mv.to_y, mv.promotion_piece)) {
		array_push(moves->move, mv);
	}

	// L down right
	mv.to_x = x + 1;
	mv.to_y = y - 2;
	if(is_valid_move(game, mv.from_x, mv.from_y, mv.to_x, mv.to_y, mv.promotion_piece)) {
		array_push(moves->move, mv);
	}

	// L right up
	mv.to_x = x + 2;
	mv.to_y = y + 1;
	if(is_valid_move(game, mv.from_x, mv.from_y, mv.to_x, mv.to_y, mv.promotion_piece)) {
		array_push(moves->move, mv);
	}

	// L right down
	mv.to_x = x + 2;
	mv.to_y = y - 1;
	if(is_valid_move(game, mv.from_x, mv.from_y, mv.to_x, mv.to_y, mv.promotion_piece)) {
		array_push(moves->move, mv);
	}

	// L left up
	mv.to_x = x - 2;
	mv.to_y = y + 1;
	if(is_valid_move(game, mv.from_x, mv.from_y, mv.to_x, mv.to_y, mv.promotion_piece)) {
		array_push(moves->move, mv);
	}

	// L left down
	mv.to_x = x - 2;
	mv.to_y = y - 1;
	if(is_valid_move(game, mv.from_x, mv.from_y, mv.to_x, mv.to_y, mv.promotion_piece)) {
		array_push(moves->move, mv);
	}
}

static void 
generate_rook_moves(Game* game, s32 x, s32 y, Gen_Moves* moves)
{
	Chess_Move mv = {0};
	mv.from_x = x;
	mv.from_y = y;

	// up
	mv.to_x = x;
	for (s32 i = y + 1; i < 8; ++i) {
		mv.to_y = i;
		if(is_valid_move(game, mv.from_x, mv.from_y, mv.to_x, mv.to_y, mv.promotion_piece)) {
			array_push(moves->move, mv);
		} else
			break;
	}

	// down
	for (s32 i = y - 1; i >= 0; --i) {
		mv.to_y = i;
		if(is_valid_move(game, mv.from_x, mv.from_y, mv.to_x, mv.to_y, mv.promotion_piece)) {
			array_push(moves->move, mv);
		} else
			break;
	}

	// left
	mv.to_y = y;
	for (s32 i = x - 1; i >= 0; --i) {
		mv.to_x = i;
		if(is_valid_move(game, mv.from_x, mv.from_y, mv.to_x, mv.to_y, mv.promotion_piece)) {
			array_push(moves->move, mv);
		} else
			break;
	}

	// right
	for (s32 i = x + 1; i < 8; ++i) {
		mv.to_x = i;
		if(is_valid_move(game, mv.from_x, mv.from_y, mv.to_x, mv.to_y, mv.promotion_piece)) {
			array_push(moves->move, mv);
		} else
			break;
	}
}

static void
generate_queen_moves(Game* game, s32 x, s32 y, Gen_Moves* moves)
{
	generate_rook_moves(game, x, y, moves);
	generate_bishop_moves(game, x, y, moves);
}

static void 
generate_king_moves(Game* game, s32 x, s32 y, Gen_Moves* moves)
{
	Chess_Move mv = {0};
	mv.from_x = x;
	mv.from_y = y;

    // Normal moves
    mv.to_x = x + 1; // right
    mv.to_y = y;
    if(is_valid_move(game, mv.from_x, mv.from_y, mv.to_x, mv.to_y, mv.promotion_piece)) 
        array_push(moves->move, mv);

    mv.to_x = x - 1; // left
    mv.to_y = y;
    if(is_valid_move(game, mv.from_x, mv.from_y, mv.to_x, mv.to_y, mv.promotion_piece)) 
        array_push(moves->move, mv);

    mv.to_x = x; // top
    mv.to_y = y + 1;
    if(is_valid_move(game, mv.from_x, mv.from_y, mv.to_x, mv.to_y, mv.promotion_piece)) 
        array_push(moves->move, mv);
    
    mv.to_x = x; // bottom
    mv.to_y = y - 1;
    if(is_valid_move(game, mv.from_x, mv.from_y, mv.to_x, mv.to_y, mv.promotion_piece)) 
        array_push(moves->move, mv);

    mv.to_x = x - 1; // top left
    mv.to_y = y + 1;
    if(is_valid_move(game, mv.from_x, mv.from_y, mv.to_x, mv.to_y, mv.promotion_piece)) 
        array_push(moves->move, mv);
    
    mv.to_x = x + 1; // top right
    mv.to_y = y + 1;
    if(is_valid_move(game, mv.from_x, mv.from_y, mv.to_x, mv.to_y, mv.promotion_piece)) 
        array_push(moves->move, mv);

    mv.to_x = x - 1; // bot left
    mv.to_y = y - 1;
    if(is_valid_move(game, mv.from_x, mv.from_y, mv.to_x, mv.to_y, mv.promotion_piece)) 
        array_push(moves->move, mv);
    
    mv.to_x = x + 1; // bot right
    mv.to_y = y - 1;
    if(is_valid_move(game, mv.from_x, mv.from_y, mv.to_x, mv.to_y, mv.promotion_piece)) 
        array_push(moves->move, mv);

    mv.to_x = x + 2; // castle
    mv.to_y = y;
    if(is_valid_move(game, mv.from_x, mv.from_y, mv.to_x, mv.to_y, mv.promotion_piece)) 
        array_push(moves->move, mv);

    mv.to_x = x - 2; // castle
    mv.to_y = y;
    if(is_valid_move(game, mv.from_x, mv.from_y, mv.to_x, mv.to_y, mv.promotion_piece)) 
        array_push(moves->move, mv);
}

s32 
generate_possible_moves(Game* game, Gen_Moves* moves) 
{
    moves->move = array_new(Chess_Move);

    for (s32 y = 0; y < 8; ++y) {
        for (s32 x = 0; x < 8; ++x) {
            Chess_Piece p = game->board[y][x];
            if (game->white_turn && is_black(p))
                continue;
            if (!game->white_turn && is_white(p))
                continue;
            switch (p) 
            {
                case CHESS_BLACK_PAWN:
                case CHESS_WHITE_PAWN:		generate_pawn_moves(game, x, y, moves); break;
                case CHESS_BLACK_BISHOP:
                case CHESS_WHITE_BISHOP:	generate_bishop_moves(game, x, y, moves); break;
                case CHESS_BLACK_KNIGHT:
                case CHESS_WHITE_KNIGHT:	generate_knight_moves(game, x, y, moves); break;
                case CHESS_BLACK_ROOK:
                case CHESS_WHITE_ROOK:		generate_rook_moves(game, x, y, moves); break;
                case CHESS_BLACK_QUEEN:
                case CHESS_WHITE_QUEEN:		generate_queen_moves(game, x, y, moves); break;
                case CHESS_BLACK_KING:
                case CHESS_WHITE_KING:		generate_king_moves(game, x, y, moves); break;
                default: break;
            }
        }
    }
    return array_length(moves->move);
}

bool
check_sufficient_material(Game* game)
{
    Chess_Piece hist[CHESS_COUNT] = {0};
    s32 total = 0;
    s32 dark_square_bishop = 0;
    s32 light_square_bishop = 0;
    for(s32 y = 0; y < 8; ++y) {
        for(s32 x = 0; x < 8; ++x) {
            Chess_Piece p = game->board[y][x];
            hist[p]++;
            if(p != CHESS_NONE)
                total++;
            if(p == CHESS_BLACK_BISHOP || p == CHESS_WHITE_BISHOP) {
                if((x + y) % 2 == 0)
                    dark_square_bishop++;
                else
                    light_square_bishop++;
            }
        }
    }

    // Two kings
    if(total == 2)
        return false;

    // King and bishop
    if(total == 3 && hist[CHESS_WHITE_BISHOP] == 1 || total == 3 && hist[CHESS_BLACK_BISHOP] == 1)
        return false;

    // King and knight
    if(total == 3 && hist[CHESS_WHITE_KNIGHT] == 1 || total == 3 && hist[CHESS_BLACK_KNIGHT] == 1)
        return false;

    // King and bishop vs king bishop of the same color
    if(total == 4 && hist[CHESS_WHITE_BISHOP] == 1 && hist[CHESS_BLACK_BISHOP] == 1 && (light_square_bishop == 2 || dark_square_bishop == 2))
        return false;

    return true;
}

bool
check_repetition(Game* game)
{
    s32 sum = 0;
    Game_History* history = ((Game_History*)game->history);
    for(s32 i = history->repetition_index_check; i < array_length(history->game) - 1; ++i) {
        Game* gm = &history->game[i]; 
        if(memcmp(gm->board, game->board, sizeof(game->board)) == 0) {
            if (
                gm->white_long_castle_valid == game->white_long_castle_valid &&
                gm->white_short_castle_valid == game->white_short_castle_valid &&
                gm->black_long_castle_valid == game->black_long_castle_valid &&
                gm->black_short_castle_valid == game->black_short_castle_valid)
            {
                sum++;
            }
        }
    }

    return sum >= 2;
}

void
game_undo(Game* game)
{
    Game_History* history = ((Game_History*)game->history);
    if (history && array_length(history->game) > 1) {
        *game = history->game[array_length(history->game) - 2];
        array_length(history->game)--;
        history->repetition_index_check = 0;
    }
}