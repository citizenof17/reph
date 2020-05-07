//
// Created by pavel on 05.05.2020.
//

#ifndef REPH_NETWRK_H
#define REPH_NETWRK_H

#include <arpa/inet.h>
#include <unistd.h>

#define DEFAULT_ADDR ("127.0.0.1")
#define DEFAULT_CLIENT_PORT (1337)
#define DEFAULT_MONITOR_PORT (7331)
#define MAX_CLIENTS (100)

typedef struct sockaddr_in sockaddr_t;

typedef struct addr_port_t {
    char addr[16];
    int port;
} addr_port_t;

typedef struct message_t {

} message_t;

typedef enum message_type_e {
    BYE,
    GET_MAP_VERSION,
    GET_MAP
} message_type_e;

typedef struct socket_transfer_t {
    int sock;
    pthread_mutex_t mutex;
} socket_transfer_t;

void init_socket_transfer(socket_transfer_t * transfer, int sock);

int make_default_socket(int * sock);
int make_socket_reusable(const int * sock);

// Simple XXXX, with error handling, to remove duplicating code (error handling)
int ssend(int sock, void * message, size_t size);
int srecv(int sock, void * message, size_t size);
int saccept(int sock, int * new_sock);

int sconnect(int sock, struct sockaddr_in * peer);

#endif //REPH_NETWRK_H
