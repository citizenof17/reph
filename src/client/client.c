//
// Created by pavel on 05.05.2020.
//

#include <stdio.h>
#include <ctype.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "src/util/general.h"
#include "src/util/cluster_map.h"
#include "src/util/object.h"
#include "src/crush.h"
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

int init_config_from_input(net_config_t * config, int argc, char ** argv) {
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

net_config_t make_default_config(){
    net_config_t config = {
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

addr_port_t get_target_device_location(object_t * obj){
    crush_result_t * input = init_crush_input(1, cluster_map->root);
    crush_result_t * result = crush_select(input, DEVICE, 1, obj_hash(obj));

    return result->buckets[0]->device->location;
}


void* post_obj(void * _obj) {
    LOG("POST");
    object_t *temp_obj = _obj;
    object_t obj = *temp_obj;
    printf("Posting key and value: %s %s\n", obj.key.val, obj.value.val);

    addr_port_t target_location = get_target_device_location(&obj);

    printf("Target location: %s %d\n", target_location.addr, target_location.port);
    int sock, rc;
    rc = connect_to_peer(&sock, target_location);
    VOID_RETURN_ON_FAILURE(rc)

    LOG("Client connected to server");

    message_type_e message = POST_OBJECT;
    rc = ssend(sock, &message, sizeof(message));
    VOID_RETURN_ON_FAILURE(rc)
    LOG("Send request for POST");

    rc = ssend(sock, &obj, sizeof(obj));
    VOID_RETURN_ON_FAILURE(rc)
    LOG("Sent POST");

    rc = srecv(sock, &message, sizeof(message));
    VOID_RETURN_ON_FAILURE(rc)
    printf("Response code for POST: %d\n", message);

    send_bye(sock);
    close(sock);

    return ((void *)EXIT_SUCCESS);
}

void* get_obj(void * _obj) {
    LOG("GET");
    object_t * obj = _obj;

    addr_port_t target_location = get_target_device_location(obj);

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
    obj.version = cluster_map->version;
    obj.primary = 1;

    pthread_t thread;
    int rc;
    rc = pthread_create(&thread, NULL, post_obj, &obj);
    if (rc != 0){
        perror("Error creating thread");
        return (EXIT_FAILURE);
    }

    void * thread_output;
    pthread_join(thread, &thread_output);
    if ((int *) thread_output == (EXIT_SUCCESS)) {
        LOG("Post was successful, adding it to local storage");
        push(storage, &storage_size, &obj);
    }
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
    else {
        LOG("Key is not found");
    }

    return (EXIT_SUCCESS);
}

int perform_some_action(){
    message_type_e possible_operations[POSSIBLE_OPERATIONS_COUNT] =
            {GET_OBJECT, POST_OBJECT, UPDATE_OBJECT, DELETE_OBJECT};

    int action_number = rand() % POSSIBLE_OPERATIONS_COUNT;

    action_number = action_number % 2; // only post and get are supported atm

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

int run(net_config_t config){
    while(1){
        int rc;
        rc = update_map_if_needed(config, &cluster_map);
        RETURN_ON_FAILURE(rc);

        rc = perform_some_action();
        RETURN_ON_FAILURE(rc);

        mysleep(10000);
    }
}

int main(int argc, char ** argv){
    init_rand();

    net_config_t config = make_default_config();
    init_config_from_input(&config, argc, argv);

    plane_cluster_map = (char *)malloc(sizeof(char) * DEFAULT_MAP_SIZE);
    cluster_map = init_empty_cluster_map();

    return run(config);
}


