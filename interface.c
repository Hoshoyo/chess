#include "interface.h"
#include "input.h"
#include "renderer.h"
#include <stb_image.h>

const vec4 black_bg = { 118.0f / 255.0f, 150.0f / 255.0f , 86.0f / 255.0f, 1.0f };
const vec4 white_bg = { 238.0f / 255.0f, 238.0f / 255.0f , 210.0f / 255.0f, 1.0f };

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

Chess_Interface
interface_init()
{
    AppInterface* result = calloc(1, sizeof(AppInterface));
	stbi_set_flip_vertically_on_load(true);

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

    return result;
}

void
interface_update_window(Chess_Interface interf, s32 width, s32 height)
{
    AppInterface* chess = (AppInterface*)interf;
    chess->window_width = width;
    chess->window_height = height;
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
			xx = floorf(((r32)ev.mouse.x / (r32)chess->window_width) * 8.0f);
			yy = floorf(8.0f - (((r32)ev.mouse.y / (r32)chess->window_height) * 8.0f));

			if(ev.mouse.action == 1) {
				input->pressed = true;
				input->start_x = xx;
				input->start_y = yy;
			} else {
				bool was_selected = (input->selected);
				input->selected = false;
				if(input->pressed && xx == input->start_x && yy == input->start_y) {
					input->selected = !was_selected;
                    if(was_selected)
                        game_move(game, input->selected_x, input->selected_y, xx, yy);
					if (input->selected) {
						input->selected_x = xx;
						input->selected_y = yy;
					}
				} else {
                    game_move(game, input->start_x, input->start_y, xx, yy);
                }
				input->pressed = false;
			}
		} else if(ev.type == HINP_EVENT_MOUSE_MOVE) {
			xx = floorf(((r32)ev.mouse.x / (r32)chess->window_width) * 8.0f);
			yy = floorf(8.0f - (((r32)ev.mouse.y / (r32)chess->window_height) * 8.0f));
			input->at_x = xx;
			input->at_y = yy;
            input->real_x = ev.mouse.x;
            input->real_y = chess->window_height - ev.mouse.y;
		}
	}
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

void 
interface_render(Chess_Interface interf, Hobatch_Context* ctx, Game* game)
{
    AppInterface* chess = (AppInterface*)interf;
	AppInput* input = &chess->input;

    r32 w = chess->window_width / 8.0f;
    r32 h = chess->window_height / 8.0f;

    Chess_Piece piece_selected = CHESS_NONE;

    if(input->pressed) {
        piece_selected = game->board[input->start_y][input->start_x];
    }

    for(int y = 0; y < 8; ++y)
    {
        for(int x = 0; x < 8; ++x)
        {
			vec4 color;
			if(((x + y) % 2) == 0)
				color = black_bg;
			else
				color = white_bg;

			if(input->pressed && input->start_x == x && input->start_y == y) {
				color = color_red;
			}
			if(input->pressed && input->at_x == x && input->at_y == y) {
				color = color_blue;
			}
			if(input->selected && input->start_x == x && input->start_y == y) {
				color = color_yellow;
			}

            batch_render_quad_color_solid(ctx, (vec3){w * x, h * y, 0}, w, h, color);

            if(!(piece_selected != CHESS_NONE && input->start_x == x && input->start_y == y))
            {
                u32 texture = 0;
                bool render_piece = texture_from_piece(chess, game->board[y][x], &texture);
                if (render_piece)
                    batch_render_quad_textured(ctx, (vec3){w * x, h * y, 0}, w, h, texture);
            }
        }
    }
    if (input->pressed) {
        u32 texture = 0;
        if(texture_from_piece(chess, piece_selected, &texture))
            batch_render_quad_textured(ctx, (vec3){input->real_x - w / 2, input->real_y - h / 2, 0}, w, h, texture);
    }
}