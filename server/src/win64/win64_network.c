#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <assert.h>
#define WINSOCK_INCLUDES
#include <network.h>

/*
  Network
*/

static FILE* net_log_stream;

static int
network_print_port(uint16_t port)
{
  return fprintf(net_log_stream, "%d", (port >> 8) | ((port & 0xff) << 8));
}

static int
network_print_ipv4(ipv4_t ip)
{
  return fprintf(net_log_stream, "%d.%d.%d.%d", (ip & 0xff), ((ip & 0xff00) >> 8), ((ip & 0xff0000) >> 16), ((ip & 0xff000000) >> 24));
}

static int
net_log_info(const char* fmt, ...)
{
  va_list args;
  va_start(args, fmt);

  int len = fprintf(net_log_stream, "[Network Info] ");
  len += vfprintf(net_log_stream, fmt, args);
  len += fprintf(net_log_stream, "\n");

  va_end(args);
  return len;
}

static int
net_log_error(int error, const char* fmt, ...)
{
  va_list args;
  va_start(args, fmt);

  char *s = NULL;
  FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 
    NULL, error,
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
    (LPSTR)&s, 0, NULL);

  int len = fprintf(net_log_stream, "[Network Error] ");
  len    += vfprintf(net_log_stream, fmt, args);
  len    += fprintf(net_log_stream, "%s\n", s);
  LocalFree(s);

  va_end(args);
  return len;
}

int
network_destroy()
{
  if(WSACleanup() != 0)
  {
    net_log_error(WSAGetLastError(), "Could not cleanup WSA: ");
    return -1;
  }
  return 0;
}

int
network_close_connection(UDP_Connection* udp_conn)
{
  if(closesocket(udp_conn->socket) == SOCKET_ERROR)
  {
    net_log_error(WSAGetLastError(), "Failed to close UDP socket: ");
    return -1;
  }
  return 0;
}

int
network_init(FILE* log_stream)
{
  WSADATA wsaData = {0};
  int status = WSAStartup(MAKEWORD(2, 2), &wsaData);
  net_log_stream = log_stream;
  if (status != 0)
  {
    net_log_error(WSAGetLastError(), "Failed to initialize: ");
    return -1;
  }
  net_log_info("Network initialized\n");
  return 0;
}

int
network_socket_set_async(UDP_Connection* out_conn)
{
  u_long i_mode = 0;
  i_mode = 1;
  if(ioctlsocket(out_conn->socket, FIONBIO, &i_mode) == SOCKET_ERROR)
  {
    net_log_error(WSAGetLastError(), "Failed to set socket as Async: ");
    return -1;
  }
  return 0;
}

// create an udp socket with a port defined by the system (ephemeral)
int
network_create_udp_socket(UDP_Connection* out_conn, int async)
{
  SOCKET connection_socket = 0;
  if ((connection_socket = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)
  {
    net_log_error(WSAGetLastError(), "Failed to create socket: ");
    return -1;
  }
  
  out_conn->socket = connection_socket;
  out_conn->port = 0;

  if(async)
  {
    if(network_socket_set_async(out_conn) == -1) return -1;
  }

  net_log_info("Created net socket (%s)\n", (async) ? "async": "blocking");
  return 0;
}

// Create a socket bound to a predefined port
int
network_create_udp_bound_socket(UDP_Connection* out_conn, unsigned short port, int async)
{
  // create
  SOCKET connection_socket = 0;
  if ((connection_socket = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)
  {
    net_log_error(WSAGetLastError(), "Failed to create socket: ");
    return -1;
  }
  out_conn->socket = connection_socket;
  
  if(async)
  {
    if(network_socket_set_async(out_conn) == -1) return -1;
  }

  net_log_info("Created net socket (%s) bound to %d", (async) ? "async": "blocking", port);

  // bind it to port
  struct sockaddr_in server_info = {0};
  server_info.sin_family = AF_INET;
  server_info.sin_addr.s_addr = INADDR_ANY;
  server_info.sin_port = htons(port);

  if (bind(connection_socket, (struct sockaddr *)&server_info, sizeof(server_info)) == SOCKET_ERROR)
  {
    net_log_error(WSAGetLastError(), "Failed to bind socket to port %d: ");
    closesocket(connection_socket);
    return -1;
  }

  net_log_info("Bound net socket (%s) to %d", (async) ? "async": "blocking", port);
  
  if(out_conn)
  {
    out_conn->socket = connection_socket;
    out_conn->port = port;
  }
  return 0;
}

Net_Status
network_send_udp_packet(UDP_Connection* udp_conn, struct sockaddr_in* to, const char* data, int length)
{
  int bytes_sent = sendto(udp_conn->socket, data, length, 0, (struct sockaddr*)to, sizeof(struct sockaddr));
  if(bytes_sent == SOCKET_ERROR)
  {
    int err = WSAGetLastError();
    switch(err)
    {
      case WSAEWOULDBLOCK:   return NETWORK_PACKET_NONE;
      case WSAECONNABORTED:
      case WSAESHUTDOWN:     return NETWORK_FORCED_SHUTDOWN;
      case WSAECONNRESET:    return NETWORK_PORT_UNREACHEABLE;
      case WSAEHOSTUNREACH:  return NETWORK_HOST_UNREACHEABLE;
      case WSAEAFNOSUPPORT:  return NETWORK_INVALID_ADDRESS;
      case WSAEADDRNOTAVAIL: return NETWORK_INVALID_ADDRESS;
      case WSAENETUNREACH:   return NETWORK_UNREACHABLE;
      case WSAETIMEDOUT:     return NETWORK_CONN_TIMEOUT;
      default: return NETWORK_PACKET_ERROR;
    }
  }
  return bytes_sent;
}

Net_Status
network_receive_udp_packets(UDP_Connection* udp_conn, UDP_Packet* out_packet)
{
  #define BUFFER_SIZE 2048
  char buffer[BUFFER_SIZE] = {0};
  struct sockaddr_in sender_info = {0};
  int info_size_bytes = sizeof(sender_info);

  int status = recvfrom(udp_conn->socket, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&sender_info, &info_size_bytes);
  if (status == 0)
  {
    // Connection was gracefully closed
    return NETWORK_CONN_CLOSED;
  }
  else if (status == SOCKET_ERROR)
  {
    int err = WSAGetLastError();
    switch (err)
    {
        case WSAEWOULDBLOCK: return NETWORK_PACKET_NONE;
        case WSAETIMEDOUT:   return NETWORK_CONN_TIMEOUT;

        case WSAENETRESET:
        case WSAESHUTDOWN:   return NETWORK_FORCED_SHUTDOWN;
        case WSAECONNRESET:  return NETWORK_FORCED_CLOSED;

        // Errors that should not happen
        default: {
          net_log_error(WSAGetLastError(), "Unexpected error code %d: ", err);
          return NETWORK_PACKET_ERROR;
        }
    }
  }
  else
  {
    out_packet->sender_info = sender_info;
    out_packet->length_bytes = status;
    memcpy(out_packet->data, buffer, out_packet->length_bytes);
    assert(out_packet->length_bytes < BUFFER_SIZE);
    out_packet->data[out_packet->length_bytes] = 0;
    return out_packet->length_bytes;
  }
}

int
network_addr_equal(struct sockaddr_in* a1, struct sockaddr_in* a2)
{
  return (a1->sin_addr.S_un.S_addr == a2->sin_addr.S_un.S_addr && a1->sin_port == a2->sin_port);
}

int
network_dns_info_ipv6(const char* name_address, struct sockaddr_in6* info)
{
  struct addrinfo* result = 0;
  struct addrinfo* ptr = 0;
  struct addrinfo hints = {0};
  int status = getaddrinfo(name_address, "0", &hints, &result);
  if(status != 0)
  {
    net_log_error(WSAGetLastError(), "Failed DNS lookup: ");
    return -1;
  }

  for (ptr = result; ptr != NULL; ptr = ptr->ai_next)
  {
    if(ptr->ai_family == AF_INET6)
    {
      *info = *(struct sockaddr_in6 *)ptr->ai_addr;
      return 0;
    }
  }
  
  net_log_error(WSAGetLastError(), "Failed to find IPV6 for address %s: ", name_address);
  return -1;
}

int
network_dns_info_ipv4(const char* name_address, struct sockaddr_in* info)
{
  struct addrinfo* result = 0;
  struct addrinfo* ptr = 0;
  struct addrinfo hints = {0};
  int status = getaddrinfo(name_address, "0", &hints, &result);
  if(status != 0)
  {
    net_log_error(WSAGetLastError(), "Failed DNS lookup: ");
    return -1;
  }

  for (ptr = result; ptr != NULL; ptr = ptr->ai_next)
  {
    if(ptr->ai_family == AF_INET)
    {
      *info = *(struct sockaddr_in*)ptr->ai_addr;
      return 0;
    }
  }
  
  net_log_error(WSAGetLastError(), "Failed to find IPV4 for address %s: ", name_address);
  return -1;
}

int
network_dns_ipv4(const char* name_address, ipv4_t* net_ipv4)
{
  struct sockaddr_in info = {0};
  if(network_dns_info_ipv4(name_address, &info) == -1) return -1;
  *net_ipv4 = info.sin_addr.S_un.S_addr;
  return 0;
}

int
network_dns_ipv6(const char* name_address, ipv6_t* net_ipv6)
{
  struct sockaddr_in6 info = {0};
  if(network_dns_info_ipv6(name_address, &info) == -1) return -1;
  memcpy(net_ipv6, info.sin6_addr.u.Byte, sizeof(ipv6_t));
  return 0;
}

int
network_sockaddr_fill(struct sockaddr_in* out_addr, int port, const char* ip)
{
    memset(out_addr, 0, sizeof(*out_addr));

    int stat = network_dns_info_ipv4(ip, out_addr);

    out_addr->sin_family = AF_INET;
    out_addr->sin_port = htons(port);
    return stat;
}