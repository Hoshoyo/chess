#include <network.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>

/*
  Network
*/

int
network_destroy()
{
    return 0;
}

int
network_init(FILE* stream)
{
    printf("linux sockets library initialized\n");
    return 0;
}

int
network_socket_set_timeout(UDP_Connection* out_conn, double time_ms)
{
    struct timeval timeout = { 0 };
    timeout.tv_sec = (long int)(time_ms / 1000.0);
    timeout.tv_usec = (long int)(1000.0 * (time_ms - (timeout.tv_sec * 1000.0)));
    int s = setsockopt(out_conn->socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
    return s;
}

int
network_socket_set_async(UDP_Connection* out_conn)
{
    int flags = fcntl(out_conn->socket, F_GETFL, 0);
    flags &= ~(O_NONBLOCK);
    if (fcntl(out_conn->socket, F_SETFL, flags) != 0) {
        fprintf(stderr, "Could not set peer socket to non blocking: %s\n", strerror(errno));
        return -1;
    }
    out_conn->flags |= NETWORK_FLAG_SOCKET_ASYNC;
    return 0;
}

int
network_create_udp_socket(UDP_Connection* out_conn, int async)
{
    // create socket
    int connection_socket = 0;
    if ((connection_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        fprintf(stderr, "failed to create socket: %s\n", strerror(errno));
        return -1;
    }
    if (async) network_socket_set_async(out_conn);
    printf("successfully created socket\n");

    out_conn->socket = connection_socket;
    out_conn->port = 0;

    return 0;
}

int
network_create_udp_bound_socket(UDP_Connection* out_conn, unsigned short port, int async)
{
    // create socket
    int connection_socket = 0;
    if ((connection_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        fprintf(stderr, "failed to create socket: %s\n", strerror(errno));
        return -1;
    }
    if (async) network_socket_set_async(out_conn);
    printf("successfully created socket\n");

    // bind it to port
    struct sockaddr_in server_info = { 0 };
    server_info.sin_family = AF_INET;
    server_info.sin_addr.s_addr = INADDR_ANY;
    server_info.sin_port = htons(port);

    if (bind(connection_socket, (struct sockaddr*)&server_info, sizeof(server_info)) < 0)
    {
        fprintf(stderr, "failed to bind udp socket to port %d: %s\n", port, strerror(errno));
        close(connection_socket);
        return -1;
    }
    printf("successfully bound udp socket to port %d\n", port);
    if (out_conn)
    {
        out_conn->socket = connection_socket;
        out_conn->port = port;
    }
    return 0;
}

Net_Status
network_receive_udp_packets_from_peer(UDP_Connection* udp_conn, struct sockaddr_in* peer, UDP_Packet* out_packet)
{
    int len = sizeof(out_packet->data);
    int n = recvfrom(udp_conn->socket, (char*)out_packet->data, len,
        (udp_conn->flags & NETWORK_FLAG_SOCKET_ASYNC) ? MSG_DONTWAIT : 0, (struct sockaddr*)peer, &len);

    switch (n)
    {
    case -1: {
        if (errno == EAGAIN)
        {
            // this is an async socket and no data is available
            if (udp_conn->flags & NETWORK_FLAG_SOCKET_ASYNC)
                return NETWORK_PACKET_NONE;
            else
                return NETWORK_TIMEOUT;
        }
        else return NETWORK_PACKET_ERROR;
    } break;
    case 0:  return NETWORK_FORCED_SHUTDOWN;
    default: {
        out_packet->length_bytes = n;
        return n;
    }
    }
}

Net_Status
network_receive_udp_packets_from_addr(UDP_Connection* udp_conn, const char* ip, int port, UDP_Packet* out_packet)
{
    struct sockaddr_in servaddr = { 0 };

    // Filling server information 
    servaddr.sin_family = AF_INET; // IPv4 
    servaddr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &servaddr.sin_addr.s_addr);

    int len = sizeof(out_packet->data);
    int n = recvfrom(udp_conn->socket, (char*)out_packet->data, len,
        (udp_conn->flags & NETWORK_FLAG_SOCKET_ASYNC) ? MSG_DONTWAIT : 0, (struct sockaddr*)&servaddr, &len);

    switch (n)
    {
    case -1: {
        if (errno == EAGAIN)
        {
            // this is an async socket and no data is available
            if (udp_conn->flags & NETWORK_FLAG_SOCKET_ASYNC)
                return NETWORK_PACKET_NONE;
            else
                return NETWORK_TIMEOUT;
        }
        else return NETWORK_PACKET_ERROR;
    } break;
    case 0:  return NETWORK_FORCED_SHUTDOWN;
    default: return n;
    }
}

int
network_send_udp_packet(UDP_Connection* udp_conn, struct sockaddr_in* to, const char* data, int length)
{
    return sendto(udp_conn->socket, data, length, MSG_DONTWAIT, (struct sockaddr*)to, sizeof(struct sockaddr));
}

Net_Status
network_receive_udp_packets(UDP_Connection* udp_conn, UDP_Packet* out_packet)
{
#define BUFFER_SIZE 1024
    char buffer[BUFFER_SIZE] = { 0 };
    struct sockaddr_in client_info = { 0 };
    int client_info_size_bytes = sizeof(client_info);

    int flags = (udp_conn->flags & NETWORK_FLAG_SOCKET_ASYNC) ? MSG_DONTWAIT : 0;
    int status = recvfrom(udp_conn->socket, buffer, BUFFER_SIZE, flags, (struct sockaddr*)&client_info, &client_info_size_bytes);

    switch (status)
    {
    case -1: {
        if (errno == EAGAIN)
        {
            // this is an async socket and no data is available
            if (udp_conn->flags & NETWORK_FLAG_SOCKET_ASYNC)
                return NETWORK_PACKET_NONE;
            else
                return NETWORK_TIMEOUT;
        }
        else return NETWORK_PACKET_ERROR;
    } break;
    case 0:  return NETWORK_FORCED_SHUTDOWN;
    default: {
        printf("received message(%d bytes) from %s:%d\n", status, inet_ntoa(client_info.sin_addr), ntohs(client_info.sin_port));
        out_packet->sender_info = client_info;
        out_packet->length_bytes = status;
        memcpy(out_packet->data, buffer, out_packet->length_bytes);
        out_packet->data[out_packet->length_bytes] = 0;
        return out_packet->length_bytes;
    } break;
    }
}

int
network_close_connection(UDP_Connection* udp_conn)
{
    return close(udp_conn->socket);
}

void
network_print_ipv4(unsigned int ip)
{
    printf("%d.%d.%d.%d", (ip & 0xff), ((ip & 0xff00) >> 8), ((ip & 0xff0000) >> 16), ((ip & 0xff000000) >> 24));
}

void
network_print_port(unsigned short port)
{
    printf("%d", (port >> 8) | ((port & 0xff) << 8));
}

// 255.255.255.255
// requires out_ip to be 15 bytes long at least
int
network_resolve_dns(const char* server_addr, char* out_ip)
{
    struct sockaddr_in result = { 0 };
    struct hostent* h = gethostbyname(server_addr);
    if (!h)
    {
        fprintf(stderr, "could not perform dns lookup: %s\n", strerror(errno));
        return -1;
    }
    char** aux = h->h_addr_list;
    if (*aux)
    {
        unsigned int ip = *(unsigned int*)*aux;
        sprintf(out_ip, "%d.%d.%d.%d", (ip & 0xff), ((ip & 0xff00) >> 8), ((ip & 0xff0000) >> 16), ((ip & 0xff000000) >> 24));
    }
    return 0;
}

const char*
network_host_string(struct sockaddr_in* host)
{
    static char buffer[256];
    unsigned int ip = host->sin_addr.s_addr;
    unsigned short port = host->sin_port;
    sprintf(buffer, "%d.%d.%d.%d:%d\0",
        (ip & 0xff), ((ip & 0xff00) >> 8), ((ip & 0xff0000) >> 16), ((ip & 0xff000000) >> 24),
        (port >> 8) | ((port & 0xff) << 8));
    return buffer;
}

int
network_addr_equal(struct sockaddr_in* a1, struct sockaddr_in* a2)
{
    return (a1->sin_addr.s_addr == a2->sin_addr.s_addr && a1->sin_port == a2->sin_port);
}