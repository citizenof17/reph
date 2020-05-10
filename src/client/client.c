//
// Created by pavel on 05.05.2020.
//

#include <stdio.h>
#include <ctype.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <src/util/general.h>
#include <src/util/cluster_map.h>

#include "src/util/log.h"
#include "src/util/netwrk.h"
#include "src/util/mysleep.h"

// Version starts from 0, thus -1 is always outdated and will cause local map update
int cluster_map_version = -1;
char * plane_cluster_map;
cluster_map_t * cluster_map;

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

int get_map_version(int sock, int * new_map_version){
    message_type_e message = GET_MAP_VERSION;
    int rc;

    rc = ssend(sock, &message, sizeof(message));
    RETURN_ON_FAILURE(rc);
    LOG("Send request for map version");

    rc = srecv(sock, new_map_version, sizeof(*new_map_version));
    RETURN_ON_FAILURE(rc);
    LOG("Received new map version");

    return (EXIT_SUCCESS);
}

int get_cluster_map(int sock){
    int rc;
    message_type_e message = GET_MAP;

    rc = ssend(sock, &message, sizeof(message));
    RETURN_ON_FAILURE(rc);
    LOG("Send request for a new map version");

    // TODO: Probably you will read more than expected from that socket (because
    //  DEFAULT_MAP_SIZE is more than actual map size is), be careful here
    char buf[DEFAULT_MAP_SIZE];
    rc = srecv(sock, buf, DEFAULT_MAP_SIZE);
    RETURN_ON_FAILURE(rc);
    LOG("Received new plane cluster map representation");

    printf("Old cluster map %s\n", plane_cluster_map);
    strcpy(plane_cluster_map, buf);
    printf("New cluster map %s\n", plane_cluster_map);

    return (EXIT_SUCCESS);
}

int update_map_if_needed(client_config_t config){
    int sock, rc;
    rc = connect_to_peer(&sock, config.monitor);
    RETURN_ON_FAILURE(rc);

    LOG("Client connected to monitor");

    int new_map_version;
    rc = get_map_version(sock, &new_map_version);
    RETURN_ON_FAILURE(rc);

    printf("New cluster map version %d\n", new_map_version);

    if (new_map_version > cluster_map_version){
        LOG("Cluster map version is updated, need to refresh its content");
        cluster_map_version = new_map_version;
        rc = get_cluster_map(sock);
        RETURN_ON_FAILURE(rc);

        cluster_map = build_map_from_string(plane_cluster_map);
    }

    send_bye(sock);
    close(sock);

    return (EXIT_SUCCESS);
}

int run_client(client_config_t config){
    while(1){
        int rc;
        rc = update_map_if_needed(config);
        RETURN_ON_FAILURE(rc);

        mysleep(10000);
    }
}

int main(int argc, char ** argv){

    client_config_t config = make_default_config();
    init_config_from_input(&config, argc, argv);

    plane_cluster_map = (char *)malloc(sizeof(char) * DEFAULT_MAP_SIZE);
    cluster_map = (cluster_map_t *)malloc(sizeof(cluster_map_t));

    return run_client(config);
}


