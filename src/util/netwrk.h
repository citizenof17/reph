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
    GET_MAP_VERSION,
    GET_MAP
} message_type_e;

typedef struct socket_transfer_t {
    int sock;
    pthread_mutex_t mutex;
} socket_transfer_t;


#endif //REPH_NETWRK_H
