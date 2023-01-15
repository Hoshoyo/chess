#pragma once
#include "os.h"

typedef enum {
	SERVER_MESSAGE_CONNECTION,
	SERVER_MESSAGE_DISCONNECT,
	SERVER_MESSAGE_UPDATE,
	SERVER_MESSAGE_ALIVE,
	SERVER_MESSAGE_NEW_PLAYER,
} Server_Message_Type;

typedef enum {
	CLIENT_MESSAGE_CONNECTION,
	CLIENT_MESSAGE_DISCONNECT,
	CLIENT_MESSAGE_UPDATE,
	CLIENT_MESSAGE_ALIVE,
} Client_Message_Type;

#pragma pack(push)
typedef struct {
	s16 index;
	s16 version;
	s32 size_bytes;
	s64 player_id;
} Update_Data;

// Client

typedef struct {
	Client_Message_Type type;

	union {
		Update_Data update;
	};

	u8 data[0];
} Client_Message;

// Server

typedef struct {
	s64 id;
} Server_Connect;

typedef struct {
	s64 id;
} Server_New_Player;

typedef struct {
	Server_Message_Type type;

	union {
		Update_Data       update;
		Server_Connect    connect;
		Server_New_Player new_player;
	};

	u8 data[0];
} Server_Message;

#pragma pack(pop)