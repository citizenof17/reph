//
// Created by pavel on 05.05.2020.
//

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <pthread.h>
#include <src/util/general.h>

#include "src/util/netwrk.h"
#include "src/util/log.h"
#include "src/util/mysleep.h"

int cluster_map_version = 0;


int init_config_from_input(addr_port_t * config, int argc, char ** argv) {
    int c;

    while ((c = getopt(argc, argv, "s:t:")) != -1) {
        switch (c) {
            case 's':
                strcpy(config->addr, optarg);
                break;
            case 't':
                config->port = atoi(optarg);
                break;
            case '?':
                LOG("Error occurred while processing CLI args");
                return (EXIT_FAILURE);
            default:
                return (EXIT_FAILURE);
        }
    }
    return (EXIT_SUCCESS);
}

addr_port_t make_default_config() {
    addr_port_t config = {
            .addr = DEFAULT_ADDR,
            .port = DEFAULT_MONITOR_PORT,
    };
    return config;
}

// TODO: ensure there is not error in memory cleaning and returning local variable
sockaddr_t make_local_addr(addr_port_t config){
    sockaddr_t local_addr;
    memset(&local_addr, 0, sizeof(sockaddr_t));
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(config.port);
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    return local_addr;
}

void * handle_client(void * arg){
    LOG("IN HANDLE CLIENT");
    socket_transfer_t * _params = arg;
    int sock, rc;
    sock = _params->sock;
    pthread_mutex_unlock(&_params->mutex);

    printf("new sock in handler %d\n", sock);

    message_type_e message_type;
    rc = recv(sock, &message_type, sizeof(message_type), 0);
    if (rc <= 0) {
        printf("Failed\n");
        return ((void *)EXIT_FAILURE);
    }

    LOG("Received some message");
    printf("Received message %d \n", message_type);

    rc = send(sock, &cluster_map_version, sizeof(cluster_map_version), 0);
    if (rc <= 0) {
        perror("Error calling send(..)");
        return ((void *)EXIT_FAILURE);
    }

    LOG("SEND VERSION");

    return ((void *) EXIT_SUCCESS);
}

typedef struct client_handler_params_t {
    int sock;
    pthread_mutex_t mutex;
    int cluster_map_version;
}client_handler_params_t;

void * wait_for_client_connection(void * arg){
    client_handler_params_t *_params = arg;
    pthread_mutex_unlock(&_params->mutex);
    int sock = _params->sock;
    int rc;

    printf("SOCK VALUE IS %d\n", sock);

    while (1){
        printf("Cluster map version is %d\n", cluster_map_version);

        int new_sock = accept(sock, NULL, NULL);
        if (new_sock <= 0){
            perror("Couldn't create new_sock");
            return ((void *) EXIT_FAILURE);
        }
        socket_transfer_t inner_params;
        inner_params.sock = new_sock;

        pthread_mutex_init(&inner_params.mutex, NULL);
        pthread_mutex_lock(&inner_params.mutex);

        pthread_t thread;
        printf("new socket %d\n", new_sock);
        rc = pthread_create(&thread, NULL, handle_client, &inner_params);
        if (rc != 0){
            perror("Error creating thread");
            return ((void *)EXIT_FAILURE);
        }

        pthread_mutex_lock(&inner_params.mutex);

        cluster_map_version++;

        pthread_join(thread, NULL);
        close(new_sock);
    }
}

int prepare_socket(int * rsock, sockaddr_t local_addr){
    int sock, rc;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Error calling socket(..)");
        return (EXIT_FAILURE);
    }

    int enable = 1;
    rc = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
    if (rc != 0){
        perror("setsockopt(SO_REUSEADDR) failed");
        return (EXIT_FAILURE);
    }

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
    printf("RSOCK VALUES %d\n", *rsock);

    return (EXIT_SUCCESS);
}

int start_client_handler(addr_port_t config, pthread_t * client_handler_thread) {
    int sock, rc;
    sockaddr_t local_addr = make_local_addr(config);
    rc = prepare_socket(&sock, local_addr);
    RETURN_ON_FAILURE(rc);


    client_handler_params_t params;
    params.sock = sock;
    pthread_mutex_init(&params.mutex, NULL);
    pthread_mutex_lock(&params.mutex);

    rc = pthread_create(client_handler_thread, NULL, wait_for_client_connection, &params);

    if (rc != 0) {
        perror("Error creating thread");
        close(sock);
        return (EXIT_FAILURE);
    }

    pthread_mutex_lock(&params.mutex);

    return (EXIT_SUCCESS);
}

int run_monitor(addr_port_t config){
    LOG("Running monitor");
    int rc;

    pthread_t client_handler_thread;
    rc = start_client_handler(config, &client_handler_thread);
    RETURN_ON_FAILURE(rc);

    pthread_join(client_handler_thread, NULL);
    return (EXIT_SUCCESS);
}


int main(int argc, char ** argv){

    addr_port_t config = make_default_config();
    init_config_from_input(&config, argc, argv);

    return run_monitor(config);
}


