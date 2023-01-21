#include "interface.h"
#include "input.h"
#include "renderer.h"
#include "gm.h"
#include <stb_image.h>
#include <light_array.h>
#include <float.h>
#include "network/network.h"
#include "network/messages.h"

typedef struct {
	bool pressed;
	bool selected;
	s32 start_x;
	s32 start_y;
	s32 at_x;
	s32 at_y;
	s32 selected_x;
	s32 selected_y;

    s32 real_x;
    s32 real_y;

    s32 scroll_up_count;
    s32 scroll_down_count;
} AppInput;

typedef struct {
    AppInput input;

    s32 window_width;
    s32 window_height;

    u32 white_king;
    u32 white_queen;
    u32 white_bishop;
    u32 white_rook;
    u32 white_knight;
    u32 white_pawn;

    u32 black_king;
    u32 black_queen;
    u32 black_bishop;
    u32 black_rook;
    u32 black_knight;
    u32 black_pawn;

    u32 select_dot;

    vec4 white_bg;
    vec4 black_bg;

    Font* font;

    bool inverted_board;

    UDP_Connection connection;
    struct sockaddr_in server_info;

    s64 player_id;

    r64 timer;
    r64 clock;

    bool disable_both_move;

    Chess_Config config;
} AppInterface;

u32
load_image(const char* filename)
{
    s32 ix, iy, cmp;
    char* data = stbi_load(filename, &ix, &iy, &cmp, 4);
	u32 result = batch_texture_create_from_data(data, ix, iy, GL_LINEAR);
    stbi_image_free(data);
    return result;
}

void
interface_send_update(AppInterface* interf, u8* data, s32 size_bytes)
{
    s32 update_msg_length = size_bytes + sizeof(Client_Message);
    Client_Message* update_msg = (Client_Message*)calloc(1, update_msg_length);
    update_msg->type = CLIENT_MESSAGE_UPDATE;
    update_msg->update.index = 0;
    update_msg->update.player_id = interf->player_id;
    update_msg->update.version = 0;
    update_msg->update.size_bytes = size_bytes;
    memcpy(update_msg->data, data, update_msg->update.size_bytes);

    network_send_udp_packet(&interf->connection, &interf->server_info, (const char*)update_msg, update_msg_length);
    free(update_msg);
}

void
interface_connect_to_server(AppInterface* interf)
{
	Client_Message connect_msg = { .type = CLIENT_MESSAGE_CONNECTION };
	network_send_udp_packet(&interf->connection, &interf->server_info, (const char*)&connect_msg, sizeof(connect_msg));

	printf("Connect to server\n");
}

void
game_disconnect_from_server(AppInterface* game)
{
	Client_Message connect_msg = { .type = CLIENT_MESSAGE_DISCONNECT };
	network_send_udp_packet(&game->connection, &game->server_info, (const char*)&connect_msg, sizeof(connect_msg));

	printf("Disconnect from server\n");
}

void
game_process_connection(AppInterface* game, Server_Message* msg)
{
	game->player_id = msg->connect.id;
	printf("Connected with player id: %lld\n", game->player_id);
}

void
game_process_disconnect(AppInterface* game, Server_Message* msg)
{
	s64 id = msg->connect.id;
	printf("Player disconnected id: %lld\n", id);
}

void
game_process_new_player(AppInterface* interf, Game* game, Server_Message* msg)
{
	printf("New player with id: %lld\n", msg->new_player.id);
    if(msg->new_player.id != interf->player_id && game->move_count > 0)
        interface_send_update(interf, (u8*)game, sizeof(Game));
}

void
game_process_update(AppInterface* chess, Game* game, Server_Message* msg)
{
	s64 id = msg->update.player_id;
	s32 index = msg->update.index;

    printf("Received update from %lld\n", id);
    Game* received_game = (Game*)msg->data;
    memcpy(game->board, received_game->board, sizeof(received_game->board));

    game->winner = received_game->winner;
    game->white_turn = received_game->white_turn;
    game->white_long_castle_valid = received_game->white_long_castle_valid;
    game->white_short_castle_valid = received_game->white_short_castle_valid;
    game->black_long_castle_valid = received_game->black_long_castle_valid;
    game->black_short_castle_valid = received_game->black_short_castle_valid;
    game->last_move = received_game->last_move;
    game->move_draw_count = received_game->move_draw_count;

    game->white_time_ms = received_game->white_time_ms;
    game->black_time_ms = received_game->black_time_ms;
    
    game->increment_ms = received_game->increment_ms;

    game->move_count = received_game->move_count;

    if (received_game->clock == 0) {
        printf("received clock 0\n");
        game->clock = 0;
    } else {
        game->clock = os_time_us() / 1000.0;
    }

    if (received_game->im_white) {
        game->im_white = false;
    }

}

void
game_process_network(AppInterface* chess, Game* game)
{
	UDP_Packet packet = { 0 };
	while (network_receive_udp_packets(&chess->connection, &packet) > 0)
	{
		Server_Message* msg = (Server_Message*)packet.data;
		switch (msg->type)
		{
			case SERVER_MESSAGE_CONNECTION: game_process_connection(chess, msg); break;
			case SERVER_MESSAGE_NEW_PLAYER: game_process_new_player(chess, game, msg); break;
			case SERVER_MESSAGE_UPDATE:     game_process_update(chess, game, msg); break;
			case SERVER_MESSAGE_DISCONNECT: game_process_disconnect(chess, msg); break;
			default: break;
		}
	}
}

Chess_Interface
interface_init()
{
    AppInterface* result = calloc(1, sizeof(AppInterface));
	stbi_set_flip_vertically_on_load(true);

    s32 config_file_length = 0;
    char* config_file_data = os_file_read("config.txt", &config_file_length, malloc);
    if(parse_config(config_file_data, &result->config) == 0) {
        result->white_bg = result->config.white_bg;
        result->black_bg = result->config.black_bg;
    } else {
        result->config.server = "localhost";
        result->config.port = 9999;
    }

	result->white_king   = load_image("res/WK.png");
    result->white_queen  = load_image("res/WQ.png");
    result->white_bishop = load_image("res/WB.png");
    result->white_rook   = load_image("res/WR.png");
    result->white_knight = load_image("res/WN.png");
    result->white_pawn   = load_image("res/WP.png");

    result->black_king   = load_image("res/BK.png");
    result->black_queen  = load_image("res/BQ.png");
    result->black_bishop = load_image("res/BB.png");
    result->black_rook   = load_image("res/BR.png");
    result->black_knight = load_image("res/BN.png");
    result->black_pawn   = load_image("res/BP.png");

    result->select_dot   = load_image("res/Select.png");

    result->inverted_board = false;

    result->disable_both_move = false;

    result->font = renderer_font_new(64, "C:/Windows/Fonts/arial.ttf");

	if (network_init(stdout) != -1)
	{
        UDP_Connection connection = { 0 };
        network_create_udp_socket(&connection, true);

        printf("Trying to perform DNS on %s\n", result->config.server);
        struct sockaddr_in server_address = { 0 };
        network_sockaddr_fill(&server_address, result->config.port, result->config.server);

        result->connection = connection;
        result->server_info = server_address;

        interface_connect_to_server(result);

        result->clock = os_time_us() / 1000.0;
        result->timer = 0;

        printf("Network initialized\n");
    }

    return result;
}

void
interface_destroy(Chess_Interface interf)
{
    AppInterface* chess = (AppInterface*)interf;
    network_close_connection(&chess->connection);
}

void
interface_update_window(Chess_Interface interf, s32 width, s32 height)
{
    AppInterface* chess = (AppInterface*)interf;
    chess->window_width = width;
    chess->window_height = height;
}

bool
texture_from_piece(AppInterface* chess, Chess_Piece piece, u32* texture)
{
    bool result = true;
    switch(piece)
    {
        case CHESS_WHITE_KING:   *texture = chess->white_king; break;
        case CHESS_WHITE_QUEEN:  *texture = chess->white_queen; break;
        case CHESS_WHITE_ROOK:   *texture = chess->white_rook; break;
        case CHESS_WHITE_KNIGHT: *texture = chess->white_knight; break;
        case CHESS_WHITE_BISHOP: *texture = chess->white_bishop; break;
        case CHESS_WHITE_PAWN:   *texture = chess->white_pawn; break;
        case CHESS_BLACK_KING:   *texture = chess->black_king; break;
        case CHESS_BLACK_QUEEN:  *texture = chess->black_queen; break;
        case CHESS_BLACK_ROOK:   *texture = chess->black_rook; break;
        case CHESS_BLACK_KNIGHT: *texture = chess->black_knight; break;
        case CHESS_BLACK_BISHOP: *texture = chess->black_bishop; break;
        case CHESS_BLACK_PAWN:   *texture = chess->black_pawn; break;
        default: result = false; break;
    }
    return result;
}

Chess_Piece
piece_from_scroll(AppInput* input, bool white)
{
    Chess_Piece result = CHESS_WHITE_QUEEN;
    s32 value = (input->scroll_up_count + input->scroll_down_count) % 4;
    if(white) {
        switch(value) {
            case 0: result = CHESS_WHITE_QUEEN; break;
            case 1: result = CHESS_WHITE_ROOK; break;
            case 2: result = CHESS_WHITE_KNIGHT; break;
            case 3: result = CHESS_WHITE_BISHOP; break;
            default: result = CHESS_WHITE_QUEEN; break;
        }
    } else {
        switch(value) {
            case 0: result = CHESS_BLACK_QUEEN; break;
            case 1: result = CHESS_BLACK_ROOK; break;
            case 2: result = CHESS_BLACK_KNIGHT; break;
            case 3: result = CHESS_BLACK_BISHOP; break;
            default: result = CHESS_BLACK_QUEEN; break;
        }
    }
    return result;
}

static s32
get_x(s32 x, bool inverted)
{
    return (inverted) ? (8 - x - 1) : x;
}

static s32
get_y(s32 y, bool inverted)
{
    return (inverted) ? (8 - y - 1) : y;
}

void 
interface_input(Chess_Interface interf, Game* game)
{
    AppInterface* chess = (AppInterface*)interf;
	AppInput* input = &chess->input;

	s32 xx = -1, yy = -1;
	Hinp_Event ev = {0};
	while (hinp_event_next(&ev)) {
		if(ev.type == HINP_EVENT_MOUSE_CLICK) {
			xx = floorf(((r32)ev.mouse.x / (r32)chess->window_height) * 8.0f);
			yy = floorf(8.0f - (((r32)ev.mouse.y / (r32)chess->window_height) * 8.0f));

			if(ev.mouse.action == 1) {
				input->pressed = true;
				input->start_x = xx;
				input->start_y = yy;
			} else {
				bool was_selected = (input->selected);
				input->selected = false;
                bool my_turn = (game->clock == 0) || (game->im_white && game->white_turn) || (!game->im_white && !game->white_turn);

				if(input->pressed && xx == input->start_x && yy == input->start_y) {
					input->selected = !was_selected;
                    if (was_selected)
                        if((my_turn || !chess->disable_both_move) && game_move(game, get_x(input->selected_x, chess->inverted_board), get_y(input->selected_y, chess->inverted_board), get_x(xx, chess->inverted_board), get_y(yy, chess->inverted_board), piece_from_scroll(input, get_y(yy, chess->inverted_board) == 7), false)) {
                            if (game->clock == 0) {
                                game->clock = os_time_us() / 1000.0;
                                game->im_white = true;
                            }
                            interface_send_update(chess, (u8*)game, sizeof(Game));
                        }
					if (input->selected) {
						input->selected_x = xx;
						input->selected_y = yy;
					}
				} else {
                    if((my_turn || !chess->disable_both_move) && game_move(game, get_x(input->start_x, chess->inverted_board), get_y(input->start_y, chess->inverted_board), get_x(xx, chess->inverted_board), get_y(yy, chess->inverted_board), piece_from_scroll(input, get_y(yy, chess->inverted_board) == 7), false)) {
                        if (game->clock == 0) {
                            game->clock = os_time_us() / 1000.0;
                            game->im_white = true;
                        }
                        interface_send_update(chess, (u8*)game, sizeof(Game));
                    }
                }
				input->pressed = false;
                input->scroll_up_count = 0;
                input->scroll_down_count = 0;
			}
		} else if(ev.type == HINP_EVENT_MOUSE_MOVE) {
			xx = floorf(((r32)ev.mouse.x / (r32)chess->window_height) * 8.0f);
			yy = floorf(8.0f - (((r32)ev.mouse.y / (r32)chess->window_height) * 8.0f));
			input->at_x = xx;
			input->at_y = yy;
            input->real_x = ev.mouse.x;
            input->real_y = chess->window_height - ev.mouse.y;
		} else if(ev.type == HINP_EVENT_MOUSE_SCROLL) {
            if(ev.mouse.scroll_delta_y > 0) {
                input->scroll_up_count++;
            } else if (ev.mouse.scroll_delta_y < 0) {
                input->scroll_down_count++;
            }
        } else if(ev.type == HINP_EVENT_KEYBOARD) {
            if(ev.keyboard.action == 1) {
                switch (ev.keyboard.key) {
                    case 'R': game_new(game); interface_send_update(chess, (u8*)game, sizeof(Game)); break;
                    case 'T': chess->inverted_board = !chess->inverted_board; break;
                    case 'D': chess->disable_both_move = !chess->disable_both_move;
                    case VK_DOWN: game->white_time_ms -= (1000.0 * 60); game->black_time_ms -= (1000.0 * 60); break;
                    case VK_UP: game->white_time_ms += (1000.0 * 60); game->black_time_ms += (1000.0 * 60); break;
                    case VK_LEFT: game_undo(game); interface_send_update(chess, (u8*)game, sizeof(Game)); break;
                    default: break;
                }
            }
        }
	}
}

static const char*
winner_text(Player p) 
{
    switch (p) {
        case PLAYER_WHITE: return "White won by checkmate!"; break;
        case PLAYER_BLACK: return "Black won by checkmate!"; break;
        case PLAYER_WHITE_TIME: return "White won on time"; break;
        case PLAYER_BLACK_TIME: return "Black won on time"; break;
        case PLAYER_DRAW_STALEMATE: return "Draw by stalemate."; break;
        case PLAYER_DRAW_50_MOVE: return "Draw by 50 move."; break;
        case PLAYER_DRAW_INSUFFICIENT_MATERIAL: return "Draw by insufficient material."; break;
        case PLAYER_DRAW_THREE_FOLD_REPETITION: return "Draw by repetition."; break;
        default: return "";
    }
}

void
timer_to_text(char* buffer, r64 timer)
{
    r64 sec = timer / 1000;
    r64 min = sec / 60;
    
    int s = (int)sec % 60;
    int m = (int)min;
    sprintf(buffer, "%02d:%02d", m, s);
}

void 
interface_render_clock(AppInterface* chess, Hobatch_Context* ctx, Game* game)
{
    // update the clock
    if(game->clock != 0) {
        r64 elapsed = (os_time_us() / 1000.0) - game->clock;
        game->clock = (os_time_us() / 1000.0);

        if(game->white_turn)
            game->white_time_ms -= elapsed;
        else
            game->black_time_ms -= elapsed;
        if(game->white_time_ms < 0) {
            game->white_time_ms = 0;
            game->winner = PLAYER_BLACK_TIME;
        }
        if(game->black_time_ms < 0) {
            game->black_time_ms = 0;
            game->winner = PLAYER_WHITE_TIME;
        }
    }

    // background
    if(!chess->inverted_board) {
        batch_render_quad_color_solid(ctx, (vec3) { chess->window_height, chess->window_height / 2.0f, 0 }, chess->window_width - chess->window_height, chess->window_height / 2.0f, (vec4){0.3f, 0.33f, 0.3f, 1.0f});
        batch_render_quad_color_solid(ctx, (vec3) { chess->window_height, 0, 0 }, chess->window_width - chess->window_height, chess->window_height / 2.0f, gm_vec4_subtract(chess->white_bg, (vec4){0.3f, 0.3f, 0.3f, 0.0f}));
    } else {
        batch_render_quad_color_solid(ctx, (vec3) { chess->window_height, chess->window_height / 2.0f, 0 }, chess->window_width - chess->window_height, chess->window_height / 2.0f, gm_vec4_subtract(chess->white_bg, (vec4){0.3f, 0.3f, 0.3f, 0.0f}));
        batch_render_quad_color_solid(ctx, (vec3) { chess->window_height, 0, 0 }, chess->window_width - chess->window_height, chess->window_height / 2.0f, (vec4){0.3f, 0.33f, 0.3f, 1.0f});
    }

    // black timer
    char text[32] = {0};
    if(!chess->inverted_board)
        timer_to_text((char*)text, game->black_time_ms);
    else
        timer_to_text((char*)text, game->white_time_ms);
    vec4 btext_pos = batch_pre_render_text(ctx, &chess->font->data, text, strlen(text), 0, (vec2) { 0, 0 }, 0, 0);
    r32 ff = (chess->window_width - chess->window_height - btext_pos.z) / 2.0f;
    batch_render_text(ctx, &chess->font->data, text, strlen(text), 0, (vec2) { chess->window_width - btext_pos.z - ff, chess->window_height / 2.0f + 20.0f }, (vec4) { 0, 0, FLT_MAX, FLT_MAX }, (vec4) { 1, 1, 1, 1 }, 0, 0);

    // white timer
    if(!chess->inverted_board)
        timer_to_text((char*)text, game->white_time_ms);
    else
        timer_to_text((char*)text, game->black_time_ms);
    vec4 wtext_pos = batch_pre_render_text(ctx, &chess->font->data, text, strlen(text), 0, (vec2) { 0, 0 }, 0, 0);
    ff = (chess->window_width - chess->window_height - wtext_pos.z) / 2.0f;
    batch_render_text(ctx, &chess->font->data, text, strlen(text), 0, (vec2) { chess->window_width - wtext_pos.z - ff, chess->window_height / 2.0f - 20.0f - wtext_pos.w }, (vec4) { 0, 0, FLT_MAX, FLT_MAX }, (vec4) { 1, 1, 1, 1 }, 0, 0);
}

void 
interface_render(Chess_Interface interf, Hobatch_Context* ctx, Game* game)
{
    AppInterface* chess = (AppInterface*)interf;
	AppInput* input = &chess->input;

    game_process_network(chess, game);

    if(chess->timer >= 1000.0) {
        chess->timer = 0;
        Client_Message alive_msg = { .type = CLIENT_MESSAGE_ALIVE };
		network_send_udp_packet(&chess->connection, &chess->server_info, (const char*)&alive_msg, sizeof(alive_msg));
    } else {
        r64 now = (os_time_us() / 1000.0);
        chess->timer += (now - chess->clock);
        chess->clock = now;
    }

    r32 w = chess->window_height / 8.0f;
    r32 h = chess->window_height / 8.0f;

    Chess_Piece piece_selected = CHESS_NONE;

    if(input->pressed) {
        piece_selected = game->board[get_y(input->start_y, chess->inverted_board)][get_x(input->start_x, chess->inverted_board)];
        if(piece_selected == CHESS_WHITE_PAWN && get_y(input->start_y, chess->inverted_board) == 6) {
            piece_selected = piece_from_scroll(input, true);
        }
        else if (piece_selected == CHESS_BLACK_PAWN && get_y(input->start_y, chess->inverted_board) == 1) {
            piece_selected = piece_from_scroll(input, false);
        }
    }

    // Render board
    for(int y = 0; y < 8; ++y)
    {
        for(int x = 0; x < 8; ++x)
        {
			vec4 color = (((x + y) % 2) == 0) ? chess->black_bg : chess->white_bg;
            vec4 selcolor = (((x + y) % 2) == 0) ? gm_vec4_add(color, (vec4) { 0.2f, 0.2f, 0.2f, 0.0f }) : gm_vec4_add(gm_vec4_scalar_product(0.2f, chess->black_bg), gm_vec4_subtract(color, (vec4) { 0.2f, 0.2f, 0.2f, 0.0f }));

			if(input->pressed && input->start_x == x && input->start_y == y) {
                color = selcolor;
			}
			if(input->pressed && input->at_x == x && input->at_y == y) {
                color = selcolor;
			}
			if(input->selected && input->start_x == x && input->start_y == y) {
                color = gm_vec4_add(color, (vec4) { 0.3f, 0.2f, 0.2f, 0.0f });
			}

            if(!game->last_move.start && game->last_move.from_x == x && game->last_move.from_y == y)
                color = gm_vec4_add(gm_vec4_subtract(color, (vec4){0.2f, 0.2f, 0.2f, 0.0f}), (vec4){0.2f, 0.2f, 0.1f, 0.0f});
            if(!game->last_move.start && game->last_move.to_x == x && game->last_move.to_y == y)
                color = gm_vec4_add(gm_vec4_subtract(color, (vec4){0.2f, 0.2f, 0.2f, 0.0f}), (vec4){0.3f, 0.3f, 0.1f, 0.0f});

            batch_render_quad_color_solid(ctx, (vec3){w * x, h * y, 0}, w, h, color);

            if(!(piece_selected != CHESS_NONE && input->start_x == x && input->start_y == y))
            {
                u32 texture = 0;
                bool render_piece = texture_from_piece(chess, game->board[get_y(y, chess->inverted_board)][get_x(x, chess->inverted_board)], &texture);
                if (render_piece)
                    batch_render_quad_textured(ctx, (vec3){w * x, h * y, 0}, w, h, texture);
            }
        }
    }

    // Render possible move preview
    if (piece_selected != CHESS_NONE || (input->selected)) {
        Gen_Moves moves = {0};
        s32 count_moves = generate_all_valid_moves_from_square(game, &moves, input->start_x, input->start_y);
        if(count_moves > 0) {
            for(s32 i = 0; i < count_moves; ++i) {                
                batch_render_quad_textured(ctx, (vec3){w * moves.move[i].to_x, h * moves.move[i].to_y, 0}, w, h, chess->select_dot);
            }
        }
        array_free(moves.move);
    }

    interface_render_clock(chess, ctx, game);

    if (input->pressed) {
        u32 texture = 0;
        if(texture_from_piece(chess, piece_selected, &texture))
            batch_render_quad_textured(ctx, (vec3){input->real_x - w / 2, input->real_y - h / 2, 0}, w, h, texture);
    }

    if (game->winner != PLAYER_NONE) {
        const char* text = winner_text(game->winner);
        vec4 text_pos = batch_pre_render_text(ctx, &chess->font->data, text, strlen(text), 0, (vec2) { 0, 0 }, 0, 0);
        batch_render_quad_color_solid(ctx, (vec3) { chess->window_width / 2 - text_pos.z / 2.0f - 10.0f, chess->window_height / 2 - text_pos.w / 2.0f - 5.0f, 0 }, text_pos.z + 20.0f, text_pos.w + 10.0f, (vec4) { 0.5f, 0.5f, 0.5f, 1.0f });
        batch_render_text(ctx, &chess->font->data, text, strlen(text), 0, (vec2) { chess->window_width / 2.0f - text_pos.z / 2.0f, chess->window_height / 2 - text_pos.w / 2.0f }, (vec4) { 0, 0, FLT_MAX, FLT_MAX }, (vec4) { 1, 1, 1, 1 }, 0, 0);
    }
}