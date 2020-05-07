//
// Created by pavel on 05.05.2020.
//

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <pthread.h>
#include <src/util/general.h>
#include <src/util/cluster_map.h>

#include "src/util/netwrk.h"
#include "src/util/log.h"
#include "src/util/mysleep.h"

int cluster_map_version = 0;
char * plane_cluster_map;
cluster_map_t * cluster_map;

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

    message_type_e message_type = GET_MAP;

    while (message_type != BYE){
        rc = srecv(sock, &message_type, sizeof(message_type));
        VOID_RETURN_ON_FAILURE(rc);
        printf("Received message %d \n", message_type);

        switch(message_type){
            case GET_MAP_VERSION:
                rc = ssend(sock, &cluster_map_version, sizeof(cluster_map_version));
                VOID_RETURN_ON_FAILURE(rc);
                LOG("Send map version");
                break;
            case GET_MAP:
                rc = ssend(sock, plane_cluster_map, strlen(plane_cluster_map));
                VOID_RETURN_ON_FAILURE(rc);
                LOG("Send map");
                break;
            case BYE:
                LOG("End of chat, bye");
            default:
                break;
        }
    }

    return ((void *) EXIT_SUCCESS);
}

void * wait_for_client_connection(void * arg){
    socket_transfer_t *_params = arg;
    pthread_mutex_unlock(&_params->mutex);
    int sock = _params->sock;
    int rc;

    printf("SOCK VALUE IS %d\n", sock);

    while (1){
        printf("Cluster map version is %d\n", cluster_map_version);

        int new_sock;
        rc = saccept(sock, &new_sock);
        VOID_RETURN_ON_FAILURE(rc);

        socket_transfer_t inner_params;
        init_socket_transfer(&inner_params, new_sock);

        pthread_t thread;
        rc = pthread_create(&thread, NULL, handle_client, &inner_params);
        if (rc != 0){
            perror("Error creating thread");
            return ((void *)EXIT_FAILURE);
        }

        pthread_mutex_lock(&inner_params.mutex);

//        cluster_map_version++;
        pthread_join(thread, NULL);
        close(new_sock);
    }
}

int prepare_socket(int * rsock, sockaddr_t local_addr){
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

int start_client_handler(addr_port_t config, pthread_t * client_handler_thread) {
    int sock, rc;
    sockaddr_t local_addr = make_local_addr(config);
    rc = prepare_socket(&sock, local_addr);
    RETURN_ON_FAILURE(rc);

    socket_transfer_t params;
    init_socket_transfer(&params, sock);

    rc = pthread_create(client_handler_thread, NULL, wait_for_client_connection, &params);

    if (rc != 0) {
        perror("Error creating thread");
        close(sock);
        return (EXIT_FAILURE);
    }

    pthread_mutex_lock(&params.mutex);

    return (EXIT_SUCCESS);
}

void update_cluster_map(){
// TODO: This is mocked for now. This function should
    printf("Updated cluster map version from %d to %d\n",
           cluster_map_version, cluster_map_version + 1);
    gen_random_string(plane_cluster_map, SHORT_MAP_SIZE);
    printf("New cluster map: %s\n", plane_cluster_map);
    cluster_map_version++;
}

int run_monitor(addr_port_t config){
    LOG("Running monitor");
    int rc;

    pthread_t client_handler_thread;
    rc = start_client_handler(config, &client_handler_thread);
    RETURN_ON_FAILURE(rc);

    // Mock map updates by updating it every ..X secs
    for (int i = 0; i < 100; i++){
        mysleep(30000);
        update_cluster_map();
    }

    pthread_join(client_handler_thread, NULL);
    return (EXIT_SUCCESS);
}


int main(int argc, char ** argv){

    addr_port_t config = make_default_config();
    init_config_from_input(&config, argc, argv);

    plane_cluster_map = get_string_map_representation(NULL);
    // TODO: Remove this and read real config from file
    gen_random_string(plane_cluster_map, SHORT_MAP_SIZE);

    cluster_map = build_map_from_string(plane_cluster_map);

    // TODO: free all allocated memory
    return run_monitor(config);
}


