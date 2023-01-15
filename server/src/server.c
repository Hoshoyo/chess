// ------------------------------------------------------------------------
// ------------------------------ Server ----------------------------------

#define HOHT_IMPLEMENTATION
#include <stdio.h>
#include <assert.h>

#include <common.h>
#include <light_array.h>
#include <hoht.h>
#include <messages.h>

#include "network.h"
#include "timer.h"

uint64_t hash_address(SOCKADDR_IN* addr)
{
	u64 hash = 14695981039346656037ULL;
	u64 fnv_prime = 1099511628211ULL;
	for (u64 i = 0; i < sizeof(SOCKADDR_IN); ++i) {
		hash = hash * fnv_prime;
		hash = hash ^ ((char*)addr)[i];
	}
	return hash;
}

void
print_packet(UDP_Packet* packet)
{
	printf("Received %d bytes from %d.%d.%d.%d:%d: { %.2f, %.2f }\n", packet->length_bytes, 
		packet->sender_info.sin_addr.S_un.S_un_b.s_b1,
		packet->sender_info.sin_addr.S_un.S_un_b.s_b2,
		packet->sender_info.sin_addr.S_un.S_un_b.s_b3,
		packet->sender_info.sin_addr.S_un.S_un_b.s_b4,
		htons(packet->sender_info.sin_port),		
		*(float*)packet->data, *(float*)&packet->data[4]);
}

void
print_client_info(struct sockaddr_in* info)
{
	printf("%d.%d.%d.%d:%d",
		info->sin_addr.S_un.S_un_b.s_b1,
		info->sin_addr.S_un.S_un_b.s_b2,
		info->sin_addr.S_un.S_un_b.s_b3,
		info->sin_addr.S_un.S_un_b.s_b4,
		htons(info->sin_port));
}

#define CLIENT_CONNECTED (1 << 0)

#define MAX_CLIENT_DATA_COUNT 4
#define SECOND 1000000.0

typedef struct {
	s32 length_bytes;
	u8* data;
	r64 update_clock;
} Client_Data;

typedef struct {
	SOCKADDR_IN  address;
	u32          flags;
	s32          player_index;
} Client;

typedef struct {
	s64         id;
	Client      client;
	Client_Data client_data[MAX_CLIENT_DATA_COUNT];

	r64 last_alive;
	s32 loss_alive;
} Player;

static UDP_Connection connection;
static Player* players;
static Hoht_Table client_table;

s64
server_new_id()
{
	static s64 id = 1;
	return id++;
}

void
push_player(Player* player)
{
	// Respond with a player id
	Server_Message conn_response = {
		.type = SERVER_MESSAGE_CONNECTION,
		.connect.id = player->id
	};
	network_send_udp_packet(&connection, &player->client.address, (const char*)&conn_response, sizeof(conn_response));
	LOG_INFO("New player %lld created\n", player->id);

	// Notify the new player of everybody else and
	// notify every other player of the new one
	for (s32 i = 0; i < array_length(players); ++i)
	{
		Player* peer = players + i;

		// Dont notify the same player
		if (peer->id == player->id)
			continue;

		// If the player is connected, notify it
		if (peer->client.flags & CLIENT_CONNECTED)
		{
			// Notify the peer about this player
			Server_Message new_player_msg = {
				.type = SERVER_MESSAGE_NEW_PLAYER,
				.new_player.id = player->id
			};
			network_send_udp_packet(&connection, &peer->client.address, (const char*)&new_player_msg, sizeof(new_player_msg));
			LOG_INFO("Notifying peer %lld about player %lld\n", peer->id, player->id);

			// Notify this player about the peer
			new_player_msg.new_player.id = peer->id;
			network_send_udp_packet(&connection, &player->client.address, (const char*)&new_player_msg, sizeof(new_player_msg));
			LOG_INFO("Notifying player %lld about peer %lld\n", peer->id, player->id);
		}
	}
}

Player*
player_get(SOCKADDR_IN* client_info)
{
	Client client = { 0 };

	u64 hash = hash_address(client_info);
	if (hoht_get_hashed(&client_table, hash, &client) != -1)
		return players + client.player_index;
	return 0;
}

void
broadcast_message(Server_Message* message, s32 length_bytes)
{
	// Notify every other client of this update
	for (s32 i = 0; i < array_length(players); ++i)
	{
		Player* player = players + i;

		// If the player is connected, notify it
		if (player->client.flags & CLIENT_CONNECTED)
		{
			network_send_udp_packet(&connection, &player->client.address, (const char*)message, length_bytes);
		}
	}
}

void
player_disconnect(Player* player, bool force)
{
	player->loss_alive++;
	if (player->loss_alive > 3 || force)
	{
		u64 hash = hash_address(&player->client.address);
		s64 id = player->id;
		if (hoht_delete_hashed(&client_table, hash) != -1)
		{	
			s32 i = player->client.player_index;
			array_remove(players, player->client.player_index);
			for (; i < array_length(players); ++i)
			{
				players[i].client.player_index = i;
				u64 hash = hash_address(&players[i].client.address);
				Client* c = hoht_get_value_hashed(&client_table, hash);
				c->player_index = i;
			}
			Server_Message disc_msg = {
				.type = SERVER_MESSAGE_DISCONNECT,
				.connect.id = id
			};
			broadcast_message(&disc_msg, sizeof(disc_msg));

			if(force)
				LOG_INFO("Player disconnected forcibly: %lld\n", id);
			else
				LOG_INFO("Player disconnected: %lld\n", id);
		}
	}
}

void
maintain_alive_players()
{
	for (s32 i = 0; i < array_length(players); ++i)
	{
		Player* p = players + i;
		if (os_time_us() - p->last_alive > SECOND * 10)
		{
			p->loss_alive++;
			LOG_INFO("Player %lld not responding for %.2f ms\n", p->id, (os_time_us() - p->last_alive) / 1000.0);
		}
		
		if (p->loss_alive > 3)
		{
			player_disconnect(p, false);
			i--;
		}
	}
}

int
process_connect(SOCKADDR_IN* client_info)
{
	LOG_INFO("Connection from ");
	print_client_info(client_info);
	LOG_INFO("\n");

	Player player = { 0 };
	Client client = { 0 };
	u64 hash = hash_address(client_info);

	if (hoht_get_hashed(&client_table, hash, &client) == -1)
	{
		client = (Client){
			.address = *client_info,
			.flags = CLIENT_CONNECTED,
			.player_index = (s32)array_length(players),
		};
		player.client = client;
		player.id = server_new_id();
		player.last_alive = os_time_us();
		array_push(players, player);
		hoht_push_hashed(&client_table, hash, &client);

		push_player(&player);

		return RESULT_OK;
	}
	return RESULT_ERROR;
}

void
process_client_update(UDP_Packet* packet)
{
	Client_Message* msg = (Client_Message *)packet->data;
	assert(msg->type == CLIENT_MESSAGE_UPDATE);

	// Find the player to be updated
	Player* player = player_get(&packet->sender_info);

	if (player)
	{
		// Update the data internally
		s32 index = msg->update.index;
		if (index > 0 && index < MAX_CLIENT_DATA_COUNT)
		{
			if (player->client_data[index].length_bytes == 0)
				player->client_data[index].data = (u8*)calloc(1, msg->update.size_bytes);
			
			player->client_data[index].length_bytes = msg->update.size_bytes;

			memcpy(player->client_data[index].data, msg->data, msg->update.size_bytes);
			player->client_data[index].update_clock = os_time_us();
		}

		player->last_alive = os_time_us();
		player->loss_alive = 0;

		// Make an update packet to send to peer clients
		s32 update_msg_length = msg->update.size_bytes + sizeof(Server_Message);
		Server_Message* update_msg = (Server_Message*)calloc(1, update_msg_length);
		update_msg->type = SERVER_MESSAGE_UPDATE;
		update_msg->update.player_id = player->id;
		update_msg->update.index = index;
		update_msg->update.version = 0;
		update_msg->update.size_bytes = msg->update.size_bytes;
		memcpy(update_msg->data, msg->data, msg->update.size_bytes);

		// Notify every other client of this update
		for (s32 i = 0; i < array_length(players); ++i)
		{
			Player* peer = players + i;

			// Dont notify the same player
			if (peer->id == player->id)
				continue;

			// If the player is connected, notify it
			if (peer->client.flags & CLIENT_CONNECTED)
			{
				network_send_udp_packet(&connection, &peer->client.address, (const char*)update_msg, update_msg_length);
			}
		}

		free(update_msg);
	}
	else
	{
		// Just drop the packet, since the player updating doesn't even exist
	}
}

void
process_alive(UDP_Packet* packet)
{
	Client_Message* msg = (Client_Message *)packet->data;
	assert(msg->type == CLIENT_MESSAGE_ALIVE);

	Player* player = player_get(&packet->sender_info);

	if (player)
	{
		Server_Message alive_msg = { .type = SERVER_MESSAGE_ALIVE };
		Net_Status status = network_send_udp_packet(&connection, &player->client.address, (const char*)&alive_msg, sizeof(alive_msg));
		if (status <= 0)
			player_disconnect(player, false);
		else
		{
			player->last_alive = os_time_us();
			player->loss_alive = 0;
		}
	}
}

void
process_disconnect(UDP_Packet* packet, bool forced)
{
	Player* player = player_get(&packet->sender_info);
	if (player)
	{
		if (forced)
			LOG_INFO("Player %lld forced a disconnection\n", player->id);
		player_disconnect(player, true);
	}
}

void
process_client_packet(UDP_Packet* packet)
{
	Client_Message* msg = (Client_Message *)packet->data;
	if (packet->length_bytes < sizeof(Client_Message))
		LOG("Received bad packet %d bytes\n", packet->length_bytes);

	switch (msg->type)
	{
		case CLIENT_MESSAGE_CONNECTION: process_connect(&packet->sender_info); break;
		case CLIENT_MESSAGE_DISCONNECT: process_disconnect(packet, false); break;
		case CLIENT_MESSAGE_ALIVE:      process_alive(packet); break;
		case CLIENT_MESSAGE_UPDATE:     process_client_update(packet); break;
		default: break;
	}
}

typedef struct {
	int64_t total_bytes_received;
	int64_t bytes_received;

	int32_t disconnect_count;
} Server_Statistics;
static Server_Statistics stats;

DWORD WINAPI thread_receive(void* param)
{
	while (true)
	{
		UDP_Packet packet = { 0 };
		Net_Status status = network_receive_udp_packets(&connection, &packet);

		switch (status)
		{
			case NETWORK_PACKET_NONE: break;
			case NETWORK_FORCED_CLOSED: {
				stats.disconnect_count++;
				process_disconnect(&packet, true);
			} break;
			default: {
				if (status > 0)
				{
					process_client_packet(&packet);
					stats.bytes_received += status;
					stats.total_bytes_received += status;
				}
				else
				{
					LOG_INFO("Network error: %d\n", status);
				}
			} break;
		}		
	}
	return 0;
}

int main()
{
	if (network_init(stdout) == -1)
		return -1;

	network_create_udp_bound_socket(&connection, 9999, false);

	os_time_init();

	Timer timer = { 0 };
	timer_start(&timer);

	s64 bytes_received = 0;
	hoht_new(&client_table, 256, sizeof(Client), 0.5f, malloc, free);
	players = array_new(Player);

	u32 receive_thread_id;
	HANDLE receive_thread_handle = CreateThread(0, 0, thread_receive, 0, 0, &receive_thread_id);

	while (true)
	{
		if (timer_has_elapsed_ms(&timer, 1000.0, true))
		{
			stats.bytes_received = 0;
			maintain_alive_players();
		}
		Sleep(100);
	}

	return 0;
}