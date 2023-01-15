#pragma once
#include <stdint.h>
#include <stdio.h>

#if defined(_WIN32) || defined(_WIN64)
#if defined (WINSOCK_INCLUDES)
#include <winsock2.h>
#include <ws2tcpip.h>
#endif
#include <windows.h>

typedef struct {
  SOCKET       socket;
  unsigned int port;
  unsigned int flags;
} UDP_Connection;

#elif defined(__linux__)
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

typedef struct {
  int          socket;
  unsigned int port;
  unsigned int flags;
} UDP_Connection;

#endif

typedef struct {
	uint8_t ip[16];
} ipv6_t;
typedef uint32_t ipv4_t;

typedef struct {
  uint8_t data[2048];
  int     length_bytes;
  struct sockaddr_in sender_info;
} UDP_Packet;

typedef enum {
  NETWORK_TIMEOUT           = -10,
  NETWORK_FORCED_CLOSED     = -9,
  NETWORK_UNREACHABLE       = -8,
  NETWORK_INVALID_ADDRESS   = -7,
  NETWORK_HOST_UNREACHEABLE = -6,
  NETWORK_PORT_UNREACHEABLE = -5,
  NETWORK_FORCED_SHUTDOWN   = -4,
  NETWORK_CONN_CLOSED       = -3,
  NETWORK_CONN_TIMEOUT      = -2,
  NETWORK_PACKET_ERROR      = -1,
  NETWORK_PACKET_NONE       = 0
} Net_Status;

#define NETWORK_FLAG_SOCKET_ASYNC (1 << 0)
#define NETWORK_FLAG_UDP_VALID_CONN (1 << 1)

// Initialize/create/destroy
int network_init(FILE* log_stream);
int network_destroy();
int network_create_udp_socket(UDP_Connection* out_conn, int async);
int network_create_udp_bound_socket(UDP_Connection* out_conn, unsigned short port, int async);
int network_close_connection(UDP_Connection* udp_conn);

// Configuration
int network_socket_set_async(UDP_Connection* out_conn);

// Send/Recv
Net_Status network_receive_udp_packets(UDP_Connection* udp_conn, UDP_Packet* out_packet);
Net_Status network_send_udp_packet(UDP_Connection* udp_conn, struct sockaddr_in* to, const char* data, int length);

// Utils
int network_dns_ipv4(const char* name_address, ipv4_t* net_ipv4);
int network_dns_ipv6(const char* name_address, ipv6_t* net_ipv6);
int network_dns_info_ipv4(const char* name_address, struct sockaddr_in* info);
int network_dns_info_ipv6(const char* name_address, struct sockaddr_in6* info);
int network_sockaddr_fill(struct sockaddr_in* out_addr, int port, const char* ip);

// Status
int  network_addr_equal(struct sockaddr_in* a1, struct sockaddr_in* a2);