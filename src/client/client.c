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
#include <pthread.h>
#include <src/util/object.h>

#include "src/util/log.h"
#include "src/util/netwrk.h"
#include "src/util/mysleep.h"

#define POSSIBLE_OPERATIONS_COUNT (4)

// Version starts from 0, thus -1 is always outdated and will cause local map update
int cluster_map_version = -1;
char * plane_cluster_map;
cluster_map_t * cluster_map;

int storage_size = 0;
object_t storage[100];

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

    LOG("Cluster map is updated");
//    printf("Old cluster map %s\n", plane_cluster_map);
    strcpy(plane_cluster_map, buf);
//    printf("New cluster map %s\n", plane_cluster_map);

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

device_t mocked_crush(){
    return *cluster_map->root->inner_buckets[0]->device;
}

device_t crush(object_key_t obj_key){
    return mocked_crush();
}

addr_port_t get_target_device_location(object_key_t obj_key){
    // TODO: Use CRUSH to calculate real device location.
    device_t target_device = crush(obj_key);
    return target_device.location;
}


void* post_obj(void* _obj) {
    LOG("POST");
    object_t *temp_obj = _obj;
    object_t obj = *temp_obj;
    printf("Posting key and value: %s %s\n", obj.key.val, obj.value.val);

    addr_port_t target_location = get_target_device_location(obj.key);

    int sock, rc;
    rc = connect_to_peer(&sock, target_location);
    VOID_RETURN_ON_FAILURE(rc);

    LOG("Client connected to server");

    message_type_e message = POST_OBJECT;
    rc = ssend(sock, &message, sizeof(message));
    VOID_RETURN_ON_FAILURE(rc);
    LOG("Send request for POST");

    rc = ssend(sock, &obj, sizeof(obj));
    VOID_RETURN_ON_FAILURE(rc);
    LOG("Sent POST");

    rc = srecv(sock, &message, sizeof(message));
    VOID_RETURN_ON_FAILURE(rc);
    printf("Response code for POST: %d\n", message);

    send_bye(sock);
    close(sock);

    return ((void *)EXIT_SUCCESS);
}

void* get_obj(void * _obj) {
    LOG("GET");
    object_t * obj = _obj;

    addr_port_t target_location = get_target_device_location(obj->key);

    int sock, rc;
    rc = connect_to_peer(&sock, target_location);
    VOID_RETURN_ON_FAILURE(rc);

    LOG("Client connected to server");

    message_type_e message = GET_OBJECT;
    rc = ssend(sock, &message, sizeof(message));
    VOID_RETURN_ON_FAILURE(rc);
    LOG("Send request for GET");

    rc = ssend(sock, obj, sizeof(*obj));
    VOID_RETURN_ON_FAILURE(rc);
    LOG("Send obj for GET");

    rc = srecv(sock, obj, sizeof(*obj));
    VOID_RETURN_ON_FAILURE(rc);

    send_bye(sock);
    close(sock);

    return ((void *)EXIT_SUCCESS);
}

void delete_obj() {}
void update_obj() {}

int perform_post(){
    object_t obj;
    object_key_t key;
    object_value_t value;

    gen_random_string(key.val, 5);
    gen_random_string(value.val, 15);


    printf("Generated key and value: %s %s\n", key.val, value.val);

    obj.key = key;
    obj.value = value;

    pthread_t thread;
    int rc;
    rc = pthread_create(&thread, NULL, post_obj, &obj);
    if (rc != 0){
        perror("Error creating thread");
        return (EXIT_FAILURE);
    }

    pthread_join(thread, NULL);
    push(storage, &storage_size, &obj);
    return (EXIT_SUCCESS);
}

object_key_t get_posted_key(){
    int obj_pos = rand() % storage_size;
    return storage[obj_pos].key;
}

int perform_get(){
    if (storage_size == 0) {
        return (EXIT_SUCCESS);
    }

    object_key_t key = get_posted_key();

    printf("GET request for the following key: %s\n", key.val);

    pthread_t thread;
    int rc;
    object_t obj;
    obj.key = key;
    rc = pthread_create(&thread, NULL, get_obj, &obj);
    if (rc != 0){
        perror("Error creating thread");
        return (EXIT_FAILURE);
    }

    pthread_join(thread, NULL);
    if (obj.value.val[0] != '\0') {
        printf("Received obj for GET: %s %s\n", obj.key.val, obj.value.val);
    }
    else{
        LOG("Key is not found");
    }

    return (EXIT_SUCCESS);
}

int perform_some_action(){
    message_type_e possible_operations[POSSIBLE_OPERATIONS_COUNT] =
            {GET_OBJECT, POST_OBJECT, UPDATE_OBJECT, DELETE_OBJECT};

    int action_number = rand() % POSSIBLE_OPERATIONS_COUNT;

    action_number = action_number % 2; // only post and get are supported

    message_type_e action = possible_operations[action_number];

    switch (action){
        case POST_OBJECT:
            perform_post();
            break;
        case GET_OBJECT:
            perform_get();
            break;
        default:
            LOG("Unsupported operation");
    }
    return (EXIT_SUCCESS);
}

int run_client(client_config_t config){
    while(1){
        int rc;
        rc = update_map_if_needed(config);
        RETURN_ON_FAILURE(rc);

        rc = perform_some_action();
        RETURN_ON_FAILURE(rc);

        mysleep(10000);
    }
}

int main(int argc, char ** argv){
    init_rand();

    client_config_t config = make_default_config();
    init_config_from_input(&config, argc, argv);

    plane_cluster_map = (char *)malloc(sizeof(char) * DEFAULT_MAP_SIZE);
    cluster_map = (cluster_map_t *)malloc(sizeof(cluster_map_t));

    return run_client(config);
}


