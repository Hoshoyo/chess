#include "game.h"

#define MAX(A, B) (((A) > (B)) ? (A) : (B))
#define MIN(A, B) (((A) < (B)) ? (A) : (B))

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

    game->last_move.start = true;
    game->white_turn = true;
    game->white_long_castle_valid = true;
    game->white_short_castle_valid = true;
    game->black_long_castle_valid = true;
    game->black_short_castle_valid = true;
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
                *searching = false;
            }
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
                *searching = false;
            }
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
                *searching = false;
            }
        } break;
        case CHESS_BLACK_ROOK:
        case CHESS_BLACK_QUEEN: {
            result |= BLACK_ATTACK;
            *searching = false;
        } break;
        case CHESS_BLACK_KING: {
            if (distance == 0) {
                result |= BLACK_ATTACK;
                *searching = false;
            }
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
            if (game->sim_board[y][x] == CHESS_WHITE_KING)
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
            if (game->sim_board[y][x] == CHESS_BLACK_KING)
                return square_attacked(board, x, y) & WHITE_ATTACK;
        }
    return false;
}

static bool
white_in_checkmate(Game* game)
{
    return false;
}

static bool
black_in_checkmate(Game* game)
{
    return false;
}

static bool
is_valid_move(Game* game, s32 from_x, s32 from_y, s32 to_x, s32 to_y, Chess_Piece promotion_piece)
{
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
                    // TODO(psv): en passant
                    // Normal capture
                    return (is_black(to_piece));
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
                    // TODO(psv): en passant
                    // Normal capture
                    return (is_white(to_piece));
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

int
game_move(Game* game, s32 from_x, s32 from_y, s32 to_x, s32 to_y, Chess_Piece promotion_choice)
{
    // Copy the state so we can simulate
    memcpy(game->sim_board, game->board, sizeof(game->board));

    Chess_Piece from_piece = game->board[from_y][from_x];
    Chess_Piece to_piece = game->board[to_y][to_x];

    if (from_piece == CHESS_NONE) {
        return -1;
    }

    Chess_Piece new_piece = from_piece;
    if(from_piece == CHESS_WHITE_PAWN && to_y == LAST_RANK) {
        if (promotion_choice == CHESS_WHITE_QUEEN || promotion_choice == CHESS_WHITE_ROOK || promotion_choice == CHESS_WHITE_KNIGHT || promotion_choice == CHESS_WHITE_BISHOP)
            new_piece = promotion_choice;
        else
            return -1; // Invalid promotion of black piece
    }
    if(from_piece == CHESS_BLACK_PAWN && to_y == FIRST_RANK) {
        if (promotion_choice == CHESS_BLACK_QUEEN || promotion_choice == CHESS_BLACK_ROOK || promotion_choice == CHESS_BLACK_KNIGHT || promotion_choice == CHESS_BLACK_BISHOP)
            new_piece = promotion_choice;
        else
            return -1; // Invalid promotion of black piece
    }

    // Check if who is moving is in fact who's turn it is
    if(!(game->white_turn && is_white(from_piece) || !game->white_turn && is_black(from_piece)))
        return -1; // Invalid move, not the pieces turn

    bool valid = is_valid_move(game, from_x, from_y, to_x, to_y, promotion_choice);
    if (!valid) return -1;

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

    // After all checks, perform the move
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

    // Pass the turn
    game->white_turn = !game->white_turn;

    return 0;
}