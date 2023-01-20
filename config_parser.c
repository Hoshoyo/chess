#include <string.h>
#include <stdlib.h>
#include "os.h"
#include <stdint.h>
#include <light_array.h>
#include "interface.h"

typedef enum {
	TOKEN_END_OF_STREAM = -1,
	TOKEN_NONE = 0,

	TOKEN_COLOR_BLACK_BACKGROUND,
	TOKEN_COLOR_WHITE_BACKGROUND,
	TOKEN_SERVER_ADDRESS,
    TOKEN_SERVER_PORT,

	TOKEN_COUNT,
} Token_Type;

// ----------------------------------------------------------------------------
// Utils

static uint8_t
hexdigit_to_u8(uint8_t d) {
	if (d >= 'A' && d <= 'F')
		return d - 'A' + 10;
	if (d >= 'a' && d <= 'f')
		return d - 'a' + 10;
	return d - '0';
}

static int
is_number(char c)
{
	return (c >= '0' && c <= '9');
}

static int
is_hex_digit(char c)
{
	return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

static int
is_space(char c)
{
	return (c == ' ' || c == '\v' || c == '\f' || c == '\t' || c == '\r');
}

static int
is_alpha(char c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static const char*
has_substring(const char* at, int length, const char* substr, int substr_length)
{
	if (length < substr_length) return 0;
	int start = 0;
	for (int i = 0; i < length; ++i)
	{
		// EEEEEEEXCEEEXCEEDED
		if (at[i] == substr[0])
		{
			if (strncmp(at + i, substr, substr_length) == 0)
				return at + i;
		}
	}
	return 0;
}

static int
string_match(const char* at, const char* start, int length)
{
	for (int i = 0; i < length; ++i)
	{
		if (at[i] == 0) return 0;
		if (at[i] != start[i]) return 0;
	}
	return 1;
}

// ----------------------------------------------------------------------------
// Parsing Low level

int64_t
parse_int64(const char* text, int length)
{
	int64_t negative = 0;
	if (*text == '-') {
		text++;
		negative = 1;
	}
	int64_t result = 0;
	int64_t tenths = 1;
	for (int64_t i = length - 1; i >= 0; --i, tenths *= 10)
		result += (text[i] - 0x30) * tenths;
	return result * ((negative) ? -1 : 1);
}

int
parse_int(const char* text, int length)
{
	int negative = 0;
	if (*text == '-') {
		text++;
		negative = 1;
	}
	int result = 0;
	int tenths = 1;
	for (int i = length - 1; i >= 0; --i, tenths *= 10)
		result += (text[i] - 0x30) * tenths;
	return result * ((negative) ? -1 : 1);
}

static unsigned int
parse_hex(const char* text, int length) {
	unsigned int res = 0;
	unsigned int count = 0;
	for (int i = length - 1; i >= 0; --i, ++count) {
		if (text[i] == 'x') break;
		char c = hexdigit_to_u8(text[i]);
		res += (unsigned int)c << (count * 4);
	}
	return res;
}

static int
parse_token(const char* at, int* length)
{
	const char* start = at;
	int len = 0;
	if (!is_alpha(*at)) return 0;
	while (*at && (is_alpha(*at) || is_number(*at) || *at == '_'))
	{
		at++;
	}
	return (int)(at - start);
}

static const char*
eat_whitespace(const char* at)
{
	while (is_space(*at)) at++;
	return at;
}

static const char*
eat_until_eol(const char* at, int* line_count)
{
	// skip everything until the end of the line
	while (*at && *at != '\n')
		at++;
	if (*at == '\n') at++;
	if(line_count) (*line_count)++;
	return at;
}

float str_to_float(const char* text, int length)
{
	float result = 0.0f;
	float tenths = 1.0f;
	float frac_tenths = 0.1f;
	int point_index = 0;

	for (; point_index < length; ++point_index)
	{
		if (text[point_index] == '.') break;
	}

	for (int i = point_index - 1; i >= 0; --i, tenths *= 10.0f)
		result += (text[i] - 0x30) * tenths;
	for (int i = point_index + 1; i < length; ++i, frac_tenths *= 0.1f)
		result += (text[i] - 0x30) * frac_tenths;
	return result;
}

static const char*
parse_float(const char* at, float* out)
{
	const char* start = at;
	bool found_dot = false;
	while (*at)
	{
		if (*at == '.') { found_dot = true; at++; }
		if (!is_number(*at)) break;
		at++;
	}
	if (out) *out = str_to_float(start, (int)(at - start));
	return at;
}

static Token_Type
next_token(const char** stream)
{
	const char* at = *stream;
	Token_Type result = TOKEN_NONE;
	if (*at == 0) 
		result = TOKEN_END_OF_STREAM;
	else if (strncmp(at, "server", sizeof("server") - 1) == 0)
	{
		at += (sizeof("server") - 1);
		result = TOKEN_SERVER_ADDRESS;
	}
	else if (strncmp(at, "port", sizeof("port") - 1) == 0)
	{
		at += (sizeof("port") - 1);
		result = TOKEN_SERVER_PORT;
	}
	else if (strncmp(at, "black_background", sizeof("black_background") - 1) == 0)
	{
		at += (sizeof("black_background") - 1);
		result = TOKEN_COLOR_BLACK_BACKGROUND;
	}
	else if (strncmp(at, "white_background", sizeof("white_background") - 1) == 0)
	{
		at += (sizeof("white_background") - 1);
		result = TOKEN_COLOR_WHITE_BACKGROUND;
	}
	*stream = at;
	
	return result;
}

// { 0.2, 0.2, 0.2, 1.0 }
static const char*
parse_config_color(const char* at, vec4* color, bool* valid)
{
	if (valid) *valid = true;
	if (*at != '{')
	{
		if(valid) *valid = false;
		return at;
	}
	at++;
	at = eat_whitespace(at);
	if (!is_number(*at))
	{
		if(valid) *valid = false;
		return at;
	}
	at = parse_float(at, &color->r);
	at = eat_whitespace(at);
	if (*at != ',')
	{
		if (valid) *valid = false;
		return at;
	}
	at++;
	at = eat_whitespace(at);
	if (!is_number(*at))
	{
		if (valid) *valid = false;
		return at;
	}
	at = parse_float(at, &color->g);
	at = eat_whitespace(at);
	if (*at != ',')
	{
		if (valid) *valid = false;
		return at;
	}
	at++;
	at = eat_whitespace(at);
	if (!is_number(*at))
	{
		if (valid) *valid = false;
		return at;
	}
	at = parse_float(at, &color->b);
	at = eat_whitespace(at);
	if (*at != ',')
	{
		if (valid) *valid = false;
		return at;
	}
	at++;
	at = eat_whitespace(at);
	if (!is_number(*at))
	{
		if (valid) *valid = false;
		return at;
	}
	at = parse_float(at, &color->a);
	at = eat_whitespace(at);
	if (*at != '}')
	{
		if (valid) *valid = false;
		return at;
	}
	at++;
	return at;
}

static const char*
parse_config_filepath(const char* at, char** out, bool* valid)
{
	const char* start = at;
	while(!is_space(*at) && *at != '\n' && *at != 0)
	{
		at++;
	}
	if (out)
	{
		*out = calloc(1, at - start + 1);
		memcpy(*out, start, at - start);
		if(valid) *valid = true;
	}
	return at;
}

static const char*
parse_config_strat_extension(const char* at, char** out, bool* valid)
{
	const char* start = at;
	if (*at != '.')
	{
		if (valid) *valid = false;
		return at;
	}
	at++;
	while (*at && !is_space(*at) && *at != '\n')
	{
		at++;
	}

	if (valid) *valid = true;
	if (out)
	{
		*out = calloc(1, at - start + 1);
		memcpy(*out, start, at - start);
	}

	return at;
}

static const char*
parse_config_int(const char* at, int* out, bool* valid)
{
	if (valid) *valid = true;
	const char* start = at;
	while (*at && is_number(*at)) at++;
	if (at - start == 0)
	{
		if (valid) *valid = false;
		return at;
	}
	if(out) *out = parse_int(start, (int)(at - start));
	return at;
}

static const char*
parse_config_float(const char* at, float* out, bool* valid)
{
	if (valid) *valid = true;
	const char* start = at;
	if (!is_number(*at))
	{
		if (valid) *valid = false;
		return at;
	}
	at = parse_float(start, out);
	return at;
}

static const char*
parse_config_bool(const char* at, bool* out, bool* valid)
{
	if (valid) *valid = true;
	if (at[0] == 't' && at[1] == 'r' && at[2] == 'u' && at[3] == 'e')
	{
		if (out) *out = true;
		return at + 4;
	}
	else if (at[0] == 'f' && at[1] == 'a' && at[2] == 'l' && at[3] == 's' && at[4] == 'e')
	{
		if (out) *out = false;
		return at + 5;
	}
	else
	{
		if (valid) *valid = false;
	}
	return at;
}

int
parse_config(const char* data, Chess_Config* config)
{
	const char* at = data;
	const char* start = at;
	
	int line = 1;
	bool parsing = true;
	while (parsing)
	{
		at = eat_whitespace(at);

		Token_Type type = next_token(&at);
		if (type == TOKEN_NONE)
		{
			printf("Invalid token at line %d, ignoring line...\n", line);
			at = eat_until_eol(at, &line);
		}
		else if (type == TOKEN_END_OF_STREAM)
		{
			parsing = false;
		}
		else
		{
			at = eat_whitespace(at);
			if (*at != '=')
				at = eat_until_eol(at, &line);
			else
				at++;
			at = eat_whitespace(at);

            bool valid = false;
			switch (type)
			{
				case TOKEN_COLOR_WHITE_BACKGROUND:
					at = parse_config_color(at, &config->white_bg, &valid);
                    if(!valid)
                        config->black_bg = (vec4) { 238.0f / 255.0f, 238.0f / 255.0f , 210.0f / 255.0f, 1.0f };
					break;
				case TOKEN_COLOR_BLACK_BACKGROUND:
					at = parse_config_color(at, &config->black_bg, &valid);
                    if(!valid)
                        config->black_bg = (vec4) { 118.0f / 255.0f, 150.0f / 255.0f , 86.0f / 255.0f, 1.0f };
					break;
				case TOKEN_SERVER_ADDRESS:
					at = parse_config_filepath(at, &config->server, &valid);
                    if(!valid)
                        config->server = "localhost";
					break;
                case TOKEN_SERVER_PORT:
                    at = parse_config_int(at, &config->port, &valid);
                    if(!valid)
                        config->port = 9999;
                    break;

				default: break;
			}
		}

		at = eat_until_eol(at, &line);
	}
	
	return 0;
}