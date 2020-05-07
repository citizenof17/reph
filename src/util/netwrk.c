//
// Created by pavel on 07.05.2020.
//



#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "netwrk.h"

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

int sconnect(int sock, struct sockaddr_in * peer){
    int rc = connect(sock, (struct sockaddr *)peer, sizeof(*peer));
    if (rc != 0) {
        perror("Error calling connect(..)");
        return (EXIT_FAILURE);
    }
    return (EXIT_SUCCESS);
}