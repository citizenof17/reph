//
// Created by pavel on 05.05.2020.
//

#include <stdio.h>
#include <ctype.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>

#include "src/util/log.h"
#include "src/util/netwrk.h"
#include "src/util/mysleep.h"

typedef struct client_config_t {
    addr_port_t self;
    addr_port_t monitor;
} client_config_t;

int init_config_from_input(client_config_t * config, int argc, char ** argv) {
    int c;

    while ((c = getopt(argc, argv, "a:p:s:t:")) != -1) {
        switch (c) {
            case 'a':
                strcpy(config->self.addr, optarg);
                break;
            case 'p':
                config->self.port = atoi(optarg);
                break;
            case 's':
                strcpy(config->monitor.addr, optarg);
                break;
            case 't':
                config->monitor.port = atoi(optarg);
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

client_config_t make_default_config(){
    client_config_t config = {
            .self = {
                    .addr = DEFAULT_ADDR,
                    .port = DEFAULT_CLIENT_PORT,
            },
            .monitor = {
                    .addr = DEFAULT_ADDR,
                    .port = DEFAULT_MONITOR_PORT,
            }
    };
    return config;
}

// TODO: ensure there is not error in memory cleaning and returning local variable
sockaddr_t make_peer(addr_port_t peer_config){
    sockaddr_t peer;
    peer.sin_family = AF_INET;
    peer.sin_port = htons(peer_config.port);
    peer.sin_addr.s_addr = inet_addr(peer_config.addr);

    return peer;
}

int get_map_version(int sock, int * new_map_version){
    int rc;
    message_type_e message = GET_MAP_VERSION;

    rc = send(sock, &message, sizeof(message), 0);
    if (rc <= 0){
        perror("Error sending get_map_version");
        return (EXIT_FAILURE);
    }

    LOG("SEND REQUEST");

    rc = recv(sock, new_map_version, sizeof(*new_map_version), 0);
    if (rc <= 0){
        perror("Error receiving get_map_version");
        return (EXIT_FAILURE);
    }

    LOG("RECEIVED CLUSTER VERSION");

    return (EXIT_SUCCESS);
}

int get_cluster_map(int sock){
    return (EXIT_SUCCESS);
}

int run_client(client_config_t config){

    int cluster_map_version = 0;

    while(1){
        struct sockaddr_in peer = make_peer(config.monitor);
        int sock, rc;

        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            perror("Error calling socket(..)");
            return (EXIT_FAILURE);
        }

        rc = connect(sock, (struct sockaddr *)&peer, sizeof(peer));
        if (rc != 0) {
            perror("Error calling connect(..)");
            return (EXIT_FAILURE);
        }

        LOG("Client connected to monitor");

        int new_map_version;
        rc = get_map_version(sock, &new_map_version);
        if (rc == EXIT_FAILURE){
            return (EXIT_FAILURE);
        }
        printf("New cluster map version %d\n", new_map_version);

        if (new_map_version > cluster_map_version){
            LOG("Cluster map version is updated, need to refresh its content");
            cluster_map_version = new_map_version;
            rc = get_cluster_map(sock);
            if (rc == EXIT_FAILURE){
                return (EXIT_FAILURE);
            }
        }
    }
}

int main(int argc, char ** argv){

    client_config_t config = make_default_config();
    init_config_from_input(&config, argc, argv);

    return run_client(config);;
}


