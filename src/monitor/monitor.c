//
// Created by pavel on 05.05.2020.
//

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <pthread.h>

#include "src/util/netwrk.h"
#include "src/util/log.h"
#include "src/util/mysleep.h"


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

// TODO: move it from here
typedef struct params_for_client_t {
    int sock;
    int cluster_map_version;
} params_for_client_t;


void * handle_client(void * arg){
    printf("hello client\n");
    LOG("IN HANDLE CLIENT");
    params_for_client_t params = *((params_for_client_t *)arg);
    int sock, rc;
    sock = params.sock;

    printf("new sock in handler %d\n", sock);

    message_type_e message_type;
    rc = recv(sock, &message_type, sizeof(message_type), 0);
    if (rc <= 0) {
        printf("Failed\n");
        return ((void *)EXIT_FAILURE);
    }

    LOG("Received some message");
    printf("Received message %d \n", message_type);

    rc = send(sock, &params.cluster_map_version, sizeof(params.cluster_map_version), 0);
    if (rc <= 0) {
        perror("Error calling send(..)");
        return ((void *)EXIT_FAILURE);
    }

    LOG("SEND VERSION");

    return ((void *) EXIT_SUCCESS);
}

int run_monitor(addr_port_t config){
    LOG("Running monitor");
    int cluster_map_version = 0;

    sockaddr_t local_addr = make_local_addr(config);
    int sock, rc;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Error calling socket(..)");
        return (EXIT_FAILURE);
    }

    rc = bind(sock, (struct sockaddr *)&local_addr, sizeof(local_addr));
    if (rc < 0) {
        perror("Error calling bind(..)");
        return (EXIT_FAILURE);
    }

    // setting maximum number of connections
    rc = listen(sock, MAX_CLIENTS);
    if (rc) {
        perror("Error calling listen(..)");
        return (EXIT_FAILURE);
    }

    while (1){
        printf("Cluster map version is %d\n", cluster_map_version);

        int new_sock = accept(sock, NULL, NULL);
        params_for_client_t params = {
                .sock = new_sock,
                .cluster_map_version = cluster_map_version,
        };
        pthread_t thread;
        printf("new socker %d\n", new_sock);
        rc = pthread_create(&thread, NULL, handle_client, &params);
        if (rc != 0){
            perror("Error creating thread");
            return (EXIT_FAILURE);
        }
        cluster_map_version++;

        pthread_join(thread, NULL);

        return (EXIT_FAILURE);
    }
}


int main(int argc, char ** argv){

    addr_port_t config = make_default_config();
    init_config_from_input(&config, argc, argv);

    return run_monitor(config);
}


