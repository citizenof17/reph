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
#define DEFAULT_OSD_PORT (4242)

#define MAX_CLIENTS (100)

typedef struct sockaddr_in sockaddr_t;

typedef struct addr_port_t {
    char addr[16];
    int port;
} addr_port_t;

typedef struct net_config_t {
    addr_port_t self;
    addr_port_t monitor;
} net_config_t;

typedef enum message_type_e {
    BYE             = 0,
    HEALTH_CHECK    = 1,
    GET_MAP_VERSION = 2,
    GET_MAP         = 3,
    POST_OBJECT     = 4,
    GET_OBJECT      = 5,
    UPDATE_OBJECT   = 6,
    DELETE_OBJECT   = 7,
    OP_SUCCESS      = 8,
    OP_FAILURE      = 9,
    NEWER_VERSION_STORED = 10,
    NOT_FOUND       = 11,
} message_type_e;

typedef struct socket_transfer_t {
    int sock;
    pthread_mutex_t mutex;
} socket_transfer_t;

int send_bye(int sock);

sockaddr_t make_local_addr(addr_port_t config);
int prepare_reusable_listening_socket(int * rsock, sockaddr_t local_addr);
// rsock - where newly created socket will be placed (return value)
int connect_to_peer(int * rsock, addr_port_t peer_config);

// TODO: ensure there is no error in memory cleaning and returning local variable
sockaddr_t make_peer(addr_port_t peer_config);

void init_socket_transfer(socket_transfer_t * transfer, int sock);

int make_default_socket(int * sock);
int make_socket_reusable(const int * sock);

// Simple send/recv/accept, with error handling, to remove duplicating error handling
int ssend(int sock, void * message, size_t size);
int srecv(int sock, void * message, size_t size);
int saccept(int sock, int * new_sock);

int sconnect(int sock, sockaddr_t * peer);

int addr_cmp(addr_port_t * addr1, addr_port_t * addr2);

#endif //REPH_NETWRK_H
