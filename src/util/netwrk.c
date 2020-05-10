//
// Created by pavel on 07.05.2020.
//



#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include "netwrk.h"
#include "general.h"
#include "src/util/log.h"

// TODO: ensure there is not error in memory cleaning and returning local variable
sockaddr_t make_local_addr(addr_port_t config){
    sockaddr_t local_addr;
    memset(&local_addr, 0, sizeof(sockaddr_t));
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(config.port);
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    return local_addr;
}

int send_bye(int sock){
    message_type_e message = BYE;
    int rc = ssend(sock, &message, sizeof(message));
    RETURN_ON_FAILURE(rc);

    LOG("Send bye");

    return (EXIT_SUCCESS);
}

int prepare_reusable_listening_socket(int * rsock, sockaddr_t local_addr){
    int sock, rc;

    rc = make_default_socket(&sock);
    RETURN_ON_FAILURE(rc);

    rc = make_socket_reusable(&sock);
    RETURN_ON_FAILURE(rc);

    rc = bind(sock, (struct sockaddr *)&local_addr, sizeof(local_addr));
    if (rc != 0) {
        perror("Error calling bind(..)");
        return (EXIT_FAILURE);
    }

    // setting maximum number of connections
    rc = listen(sock, MAX_CLIENTS);
    if (rc != 0) {
        perror("Error calling listen(..)");
        return (EXIT_FAILURE);
    }

    *rsock = sock;
    printf("RSOCK VALUE %d\n", *rsock);

    return (EXIT_SUCCESS);
}

sockaddr_t make_peer(addr_port_t peer_config){
    sockaddr_t peer;
    peer.sin_family = AF_INET;
    peer.sin_port = htons(peer_config.port);
    peer.sin_addr.s_addr = inet_addr(peer_config.addr);

    return peer;
}

int connect_to_peer(int * rsock, addr_port_t peer_config){
    sockaddr_t peer = make_peer(peer_config);
    int sock, rc;

    rc = make_default_socket(&sock);
    RETURN_ON_FAILURE(rc);

    rc = make_socket_reusable(&sock);
    RETURN_ON_FAILURE(rc);

    rc = sconnect(sock, &peer);
    RETURN_ON_FAILURE(rc);

    *rsock = sock;
    return (EXIT_SUCCESS);
}

int make_default_socket(int * sock){
    if ((*sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error calling socket(..)");
        return (EXIT_FAILURE);
    }
    return (EXIT_SUCCESS);
}

int make_socket_reusable(const int * sock){
    int enable = 1;
    if (setsockopt(*sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) != 0){
        perror("Error calling setsockopt(SO_REUSEADDR)");
        return (EXIT_FAILURE);
    }
    return (EXIT_SUCCESS);
}

void init_socket_transfer(socket_transfer_t * transfer, int sock){
    transfer->sock = sock;
    pthread_mutex_init(&transfer->mutex, NULL);
    pthread_mutex_lock(&transfer->mutex);
}

// TODO: add support for big objects, until the whole file is send or received

int ssend(int sock, void * message, size_t size){
    int rc = send(sock, message, size, 0);
    if (rc < 0) {
        perror("Error calling ssend(..)");
        return (EXIT_FAILURE);
    }
    return (EXIT_SUCCESS);
}

int srecv(int sock, void * buffer, size_t size) {
    int rc = recv(sock, buffer, size, 0);
    if (rc < 0) {
        perror("Error calling srecv(..)");
        return (EXIT_FAILURE);
    }
    printf("Received %d bytes\n", rc);
    return (EXIT_SUCCESS);
}

int saccept(int sock, int * new_sock){
    *new_sock = accept(sock, NULL, NULL);
    if (*new_sock <= 0 ){
        perror("Error calling saccept(..)");
        return (EXIT_FAILURE);
    }
    return (EXIT_SUCCESS);
}

int sconnect(int sock, sockaddr_t * peer){
    int rc = connect(sock, (struct sockaddr *)peer, sizeof(*peer));
    if (rc != 0) {
        perror("Error calling connect(..)");
        return (EXIT_FAILURE);
    }
    return (EXIT_SUCCESS);
}