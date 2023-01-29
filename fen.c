#include "os.h"
#include "game.h"

typedef enum {
	FEN_BOARD,
	FEN_TO_MOVE,
	FEN_CASTLING,
	FEN_EN_PASSANT,
	FEN_HALFMOVE,
	FEN_FULLMOVE,
	FEN_END,
} Fen_Format;

static int
is_number(char c)
{
	return (c >= '0' && c <= '9');
}

static int
is_whitespace(char c)
{
	return (c == ' ' || c == '\v' || c == '\f' || c == '\t' || c == '\r');
}

s32 parse_fen(s8* fen, Game* game) {
	// startpos
	// rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1

	memset(game->board, CHESS_NONE, sizeof(game->board));
    game->move_draw_count = 0;

	Fen_Format parsing_state = FEN_BOARD;

	s32 rank = 7;
	s32 file = 0;
	while (*fen != 0) {
		s8 c = *fen;
		if (is_whitespace(c)) {
			fen++;
			continue;
		}

		switch (parsing_state) 
		{
			case FEN_BOARD: {
				switch (c) 
				{
					case 'r':	game->board[rank][file] = CHESS_BLACK_ROOK; break;
					case 'n':	game->board[rank][file] = CHESS_BLACK_KNIGHT; break;
					case 'b':	game->board[rank][file] = CHESS_BLACK_BISHOP; break;
					case 'q':	game->board[rank][file] = CHESS_BLACK_QUEEN; break;
					case 'k':	game->board[rank][file] = CHESS_BLACK_KING; break;
					case 'p':	game->board[rank][file] = CHESS_BLACK_PAWN; break;

					case 'R':	game->board[rank][file] = CHESS_WHITE_ROOK; break;
					case 'N':	game->board[rank][file] = CHESS_WHITE_KNIGHT; break;
					case 'B':	game->board[rank][file] = CHESS_WHITE_BISHOP; break;
					case 'Q':	game->board[rank][file] = CHESS_WHITE_QUEEN; break;
					case 'K':	game->board[rank][file] = CHESS_WHITE_KING; break;
					case 'P':	game->board[rank][file] = CHESS_WHITE_PAWN; break;

					case '/': {
						rank -= 1;
						file = -1;
					} break;

					default: {
						if (is_number(c)) {
							s32 start = 0;
							while (start < (c - 0x30) - 1) {
								game->board[rank][file] = CHESS_NONE;
								start++;
								file++;
							}
						}
					}break;
				}

				if (file >= 7 && rank == 0) {
					parsing_state = FEN_TO_MOVE;
				} else {
					file += 1;
				}
			} break;

			case FEN_TO_MOVE: {
				if (c == 'w')
                    game->white_turn = true;
				else if (c == 'b')
					game->white_turn = false;
				else
					return -1;
				parsing_state = FEN_CASTLING;
			}break;

			case FEN_CASTLING: {
				while (!is_whitespace(*fen)) {
					s8 c = *fen;
					if (c == '-')
						break;
                    
					switch (c) {
						case 'K': game->white_short_castle_valid; break;
						case 'Q': game->white_long_castle_valid; break;
						case 'k': game->black_short_castle_valid; break;
						case 'q': game->black_long_castle_valid; break;
						default: return -1;
					}
					++fen;
				}
				parsing_state = FEN_EN_PASSANT;
			}break;

			case FEN_EN_PASSANT: {
				if (c == '-')
					parsing_state = FEN_HALFMOVE;
				else {
					if (c >= 'a' && c <= 'h') {
						//state->en_passant_file = c - 0x61;
                        int file = c - 0x61;
                        if(game->white_turn) {
                            game->last_move.from_x = file;
                            game->last_move.from_y = 6;
                            game->last_move.to_x = file;
                            game->last_move.to_y = 4;
                            game->last_move.moved_piece = CHESS_BLACK_PAWN;
                        } else {
                            game->last_move.from_x = file;
                            game->last_move.from_y = 1;
                            game->last_move.to_x = file;
                            game->last_move.to_y = 3;
                            game->last_move.moved_piece = CHESS_WHITE_PAWN;
                        }
					}
					fen++;	// skip rank
					parsing_state = FEN_HALFMOVE;
				}
			}break;

			case FEN_HALFMOVE: {
				s32 length = 0;
				while (!is_whitespace(*(fen + length))) length++;
                game->move_draw_count = parse_int(fen, length);
				parsing_state = FEN_FULLMOVE;
			}break;

			case FEN_FULLMOVE: {
				s32 length = 0;
				while (!is_whitespace(*(fen + length)) && *(fen + length)) length++;
                // Ignore, we dont use it                
				parsing_state = FEN_END;
			}break;

			case FEN_END: return 0;
		}
		
		++fen;
	}

	return 0;
}