//
// Created by pavel on 05.05.2020.
//

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <pthread.h>

#include "src/util/general.h"
#include "src/util/cluster_map.h"
#include "src/util/netwrk.h"
#include "src/util/log.h"
#include "src/util/mysleep.h"

#define HEALTH_CHECK_DELAY (2 * 1000)  // ms to wait -> 1 min
#define ALIVE (1)
#define NOT_ALIVE (0)


char * plane_cluster_map;
cluster_map_t * cluster_map;

int init_config_from_input(addr_port_t * config, int argc, char ** argv) {
    int c;

    while ((c = getopt(argc, argv, "a:p:")) != -1) {
        switch (c) {
            case 'a':
                strcpy(config->addr, optarg);
                break;
            case 'p':
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

void * handle_client(void * arg){
    socket_transfer_t * _params = arg;
    int sock, rc;
    sock = _params->sock;
    pthread_mutex_unlock(&_params->mutex);

    message_type_e message_type = GET_MAP;

    while (message_type != BYE){
        message_type = BYE;
        rc = srecv(sock, &message_type, sizeof(message_type));
        VOID_RETURN_ON_FAILURE(rc);
//        printf("Received message %d \n", message_type);

        switch(message_type){
            case GET_MAP_VERSION:
                rc = ssend(sock, &cluster_map->version, sizeof(cluster_map->version));
                VOID_RETURN_ON_FAILURE(rc);
                LOG("Send map version");
                break;
            case GET_MAP:
                rc = ssend(sock, plane_cluster_map, DEFAULT_MAP_SIZE);
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
    int sock, rc;
    sock = _params->sock;
    pthread_mutex_unlock(&_params->mutex);

    printf("SOCK VALUE IS %d\n", sock);

    while (1){
        printf("Cluster map version is %d\n", cluster_map->version);

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

int start_client_handler(addr_port_t config, pthread_t * client_handler_thread) {
    int sock, rc;
    sockaddr_t local_addr = make_local_addr(config);
    rc = prepare_reusable_listening_socket(&sock, local_addr);
    RETURN_ON_FAILURE(rc);

    socket_transfer_t params;
    init_socket_transfer(&params, sock);

    rc = pthread_create(client_handler_thread, NULL, wait_for_client_connection, &params);

    if (rc != 0) {
        perror("Error creating thread");
        return (EXIT_FAILURE);
    }

    pthread_mutex_lock(&params.mutex);
    return (EXIT_SUCCESS);
}

void update_cluster_map(){
    LOG("Updating cluster map");
    printf("Updated cluster map version from %d to %d\n",
           cluster_map->version, cluster_map->version + 1);
    cluster_map->version++;
    free(plane_cluster_map);

    plane_cluster_map = cluster_map_to_string(cluster_map);

    printf("Plane cluster map: %s\n", plane_cluster_map);
    print_cluster_map(cluster_map);
}

int send_health_check(int sock){
    message_type_e message = HEALTH_CHECK;
    int is_alive = ALIVE;

    int rc;
    rc = ssend(sock, &message, sizeof(message));
    if (rc != (EXIT_SUCCESS)){
        is_alive = NOT_ALIVE;
        return is_alive;
    }

    int res;
    rc = srecv(sock, &res, sizeof(res));
    if (rc != (EXIT_SUCCESS)){
        is_alive = NOT_ALIVE;
    }

    send_bye(sock);
    return is_alive;
}

int poll_osd(device_t * device){
    state_e old_state = device->state;

    device->state = UNKNOWN;

    int sock, rc;
//    LOG("poll_osd fun");
    rc = connect_to_peer(&sock, device->location);
    if (rc != (EXIT_SUCCESS)) {
        device->state = DOWN;
        perror("Error connecting to peer");
    }
    else {
        int alive = send_health_check(sock);
        device->state = alive ? UP : DOWN;
    }
//    printf("Health check to %s %d, device state is %d\n", device->location.addr,
//           device->location.port, device->state);

    return old_state != device->state;
}

int dfs_check_devices(bucket_t * bucket){
    int state_changed = 0;
//    printf("Bucket is: %d %d\n", bucket->class, bucket->type);
    if (bucket->class == DEVICE){
        state_changed |= poll_osd(bucket->device);
    }
    else{
        for (int i = 0; i < bucket->size; i++){
            state_changed |= dfs_check_devices(bucket->inner_buckets[i]);
        }
    }
    return state_changed;
}

_Noreturn void * osd_poller(void * arg){
    while(1){
        LOG("Start device liveness check");
        int state_changed = dfs_check_devices(cluster_map->root);
        if (state_changed) {
            LOG("State changed");
            update_cluster_map();
        }
        LOG("Sleeping until next osd polling");
        mysleep(HEALTH_CHECK_DELAY);
    }
}

int start_osd_poller(pthread_t * osd_thread){
    int rc = pthread_create(osd_thread, NULL, osd_poller, NULL);
    RETURN_ON_FAILURE(rc);
    return (EXIT_SUCCESS);
}

int run(addr_port_t config){
    LOG("Running monitor");
    int rc;

    pthread_t client_handler_thread;
    rc = start_client_handler(config, &client_handler_thread);
    RETURN_ON_FAILURE(rc);

    pthread_t osd_poller;
    rc = start_osd_poller(&osd_poller);
    RETURN_ON_FAILURE(rc);

    pthread_join(client_handler_thread, NULL);
    pthread_join(osd_poller, NULL);
    return (EXIT_SUCCESS);
}

int main(int argc, char ** argv){
    addr_port_t config = make_default_config();
    init_config_from_input(&config, argc, argv);

    // TODO: Read map filepath from input
    cluster_map = cluster_map_from_file("/home/pavel/reph/sample_cluster.txt");
//    cluster_map = build_default_map();
    plane_cluster_map = cluster_map_to_string(cluster_map);
    print_cluster_map(cluster_map);

    // TODO: free all allocated memory
    return run(config);
}


