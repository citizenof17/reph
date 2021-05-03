//
// Created by pavel on 05.05.2020.
//

#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include "src/util/general.h"
#include "src/util/cluster_map.h"
#include "src/inner_storage/object.h"
#include "src/crush.h"
#include "src/util/log.h"
#include "src/util/netwrk.h"
#include "src/util/mysleep.h"

#define POSSIBLE_OPERATIONS_COUNT (4)

cluster_map_t * cluster_map;

storage_linear_t storage;

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

int get_target_device_location(object_t * obj, addr_port_t * location){
    crush_result_t * input = init_crush_input(1, cluster_map->root);
    crush_result_t * result = crush_select(input, DEVICE, 1, obj_hash(obj));

    if (result->size > 0){
        addr_port_t found_location = result->buckets[0]->device->location;
        strcpy(location->addr, found_location.addr);
        location->port = found_location.port;
        return (EXIT_SUCCESS);
    }

    return (EXIT_FAILURE);
}


void* post_obj(void * _obj) {
    LOG("POST");
    object_t *temp_obj = _obj;
    object_t obj = *temp_obj;
    printf("++++++ Posting key and value: %s %s ++++++\n", obj.key.val, obj.value.val);

    addr_port_t target_location;
    int rc = get_target_device_location(&obj, &target_location);
    printf("Target location: %s %d, found: %d\n", target_location.addr, target_location.port, !rc);
    VOID_RETURN_ON_FAILURE(rc)

    int sock;
    rc = connect_to_peer(&sock, target_location);
    VOID_RETURN_ON_FAILURE(rc)

    LOG("Client connected to server");

    message_type_e message = POST_OBJECT;
    rc = ssend(sock, &message, sizeof(message));
    VOID_RETURN_ON_FAILURE(rc)
    LOG("Send request for POST");

    rc = ssend(sock, &obj, sizeof(obj));
    VOID_RETURN_ON_FAILURE(rc)
    LOG("Sent obj for POST");

    rc = srecv(sock, &message, sizeof(message));
    printf("Response code for POST: %d\n", message);
    VOID_RETURN_ON_FAILURE(rc)

    send_bye(sock);
    close(sock);

    return ((void *)EXIT_SUCCESS);
}

void* get_obj(void * _obj) {
    LOG("GET");
    object_t * obj = _obj;

    addr_port_t target_location;
    int rc = get_target_device_location(obj, &target_location);
    VOID_RETURN_ON_FAILURE(rc);

    int sock;
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

void* update_obj(void * _obj) {
    LOG("UPDATE");
    object_t * obj = _obj;
    printf("++++++Update key and value: %s %s ++++++\n", obj->key.val, obj->value.val);

    addr_port_t target_location;
    int rc = get_target_device_location(obj, &target_location);
    printf("Target location: %s %d, found: %d\n", target_location.addr, target_location.port, !rc);
    VOID_RETURN_ON_FAILURE(rc)

    int sock;
    rc = connect_to_peer(&sock, target_location);
    VOID_RETURN_ON_FAILURE(rc)

    LOG("Client connected to server");

    message_type_e message = UPDATE_OBJECT;
    rc = ssend(sock, &message, sizeof(message));
    VOID_RETURN_ON_FAILURE(rc)
    LOG("Send request for UPDATE");

    rc = ssend(sock, obj, sizeof(*obj));
    VOID_RETURN_ON_FAILURE(rc)
    LOG("Sent UPDATE");

    rc = srecv(sock, &message, sizeof(message));
    printf("Response code for UPDATE: %d\n", message);
    VOID_RETURN_ON_FAILURE(rc)

    send_bye(sock);
    close(sock);

    return ((void *)EXIT_SUCCESS);
}

void* del_obj(void * _obj) {
    LOG("DELETE");
    object_t *temp_obj = _obj;
    object_t obj = *temp_obj;
    printf("Delete obj with key: %s\n", obj.key.val);

    addr_port_t target_location;
    int rc = get_target_device_location(&obj, &target_location);
    printf("Target location: %s %d, found: %d\n", target_location.addr, target_location.port, !rc);
    VOID_RETURN_ON_FAILURE(rc)

    int sock;
    rc = connect_to_peer(&sock, target_location);
    VOID_RETURN_ON_FAILURE(rc)

    LOG("Client connected to server");

    message_type_e message = DELETE_OBJECT;
    rc = ssend(sock, &message, sizeof(message));
    VOID_RETURN_ON_FAILURE(rc)
    LOG("Send request for DELETE");

    rc = ssend(sock, &obj, sizeof(obj));
    VOID_RETURN_ON_FAILURE(rc)
    LOG("Sent obj for DELETE");

    rc = srecv(sock, &message, sizeof(message));
    printf("Response code for DELETE: %d\n", message);
    VOID_RETURN_ON_FAILURE(rc)

    send_bye(sock);
    close(sock);

    return ((void *)EXIT_SUCCESS);
}

int perform_post(){
    LOG("Performing POST");

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
        push_linear(&storage, &obj, -1);
    }
    return (EXIT_SUCCESS);
}

object_key_t get_posted_key(){
    int obj_pos = rand() % storage.size;
    return storage.objects[obj_pos].key;
}

int perform_get(){
    LOG("Performing GET");
    if (storage.size == 0) {
        return (EXIT_SUCCESS);
    }

    object_key_t key = get_posted_key();

    printf("++++++ GET request for the following key: %s ++++++\n", key.val);

    pthread_t thread;
    int rc;
    object_t obj;
    memset(&obj, 0, sizeof(object_t));
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

int perform_update(){
    LOG("Performing UPDATE");
    int try_post = rand() % 10 < 2;
    if (try_post) {
        int put_res = perform_post();
        printf("Update result: %d\n", put_res);
        return put_res;
    }

    if (storage.size == 0) {
        return (EXIT_SUCCESS);
    }

    object_t obj;
    memset(&obj, 0, sizeof(object_t));

    // Instead of get_posted_key()
    int obj_pos = rand() % storage.size;
    obj.key = storage.objects[obj_pos].key;
    gen_random_string(obj.value.val, 15);
    obj.version = cluster_map->version;
    obj.primary = 1;

    printf("UPDATE request for the following key: %s; new value: %s\n", obj.key.val, obj.value.val);

    pthread_t thread;
    int rc;
    rc = pthread_create(&thread, NULL, update_obj, &obj);
    if (rc != 0){
        perror("Error creating thread");
        return (EXIT_FAILURE);
    }

    void * thread_output;
    pthread_join(thread, &thread_output);
    if ((int *) thread_output == (EXIT_SUCCESS)) {
        LOG("Update was successful");
        push_linear(&storage, &obj, obj_pos);
    }
    return (EXIT_SUCCESS);
}

int perform_delete(){
    // TODO: delete object from local storage. Leaving it as it to also test GET for a removed
    //  object
    LOG("Performing DELETE");
    object_key_t key;

    int try_delete = rand() % 10 < 2;
    if (try_delete) {
        gen_random_string(key.val, 5);
    }
    else if (storage.size > 0){
        key = get_posted_key();
    }
    else {
        return (EXIT_SUCCESS);
    }

    printf("++++++DELETE request for the following key: %s++++++\n", key.val);

    pthread_t thread;
    int rc;
    object_t obj;
    memset(&obj, 0, sizeof(object_t));
    obj.key = key;
    obj.primary = 1;

    rc = pthread_create(&thread, NULL, del_obj, &obj);
    if (rc != 0){
        perror("Error creating thread");
        return (EXIT_FAILURE);
    }

    void * thread_output;
    pthread_join(thread, &thread_output);
    if ((int *) thread_output == (EXIT_SUCCESS)) {
        LOG("Delete was successful");
    }
    else {
        printf("Something goes wrong in del_obj, result: %d\n", *((int *)thread_output));
    }

    return (EXIT_SUCCESS);
}

int perform_some_action(){
    message_type_e possible_operations[POSSIBLE_OPERATIONS_COUNT] =
            {POST_OBJECT, GET_OBJECT, UPDATE_OBJECT, DELETE_OBJECT};

//    int action_number = rand() % POSSIBLE_OPERATIONS_COUNT;
//    action_number = action_number % 2; // only post and get are supported atm

    int action_number = rand() % 10;
    printf("Action number: %d\n", action_number);

    // TODO: Change priorities
    if      (action_number < 4) { action_number = 0; }  // POST - most preferable operation
    else if (action_number < 6) { action_number = 1; }  // GET
    else if (action_number < 8) { action_number = 2; }  // UPDATE
    else if (action_number < 10){ action_number = 3; }  // DELETE

    message_type_e action = possible_operations[action_number];

    switch (action){
        case POST_OBJECT:
            perform_post();
            break;
        case GET_OBJECT:
            perform_get();
            break;
        case UPDATE_OBJECT:
            perform_update();
            break;
        case DELETE_OBJECT:
            perform_delete();
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

        mysleep(15000);
    }
}


void intHandler(int smt){
    print_storage_linear(&storage);
    printf("Error code %d\n", smt);
    exit(1);
}


int main(int argc, char ** argv){
    init_rand();
    signal(SIGINT, intHandler);
    signal(SIGTERM, intHandler);

    storage.size = 0;

    net_config_t config = make_default_config();
    init_config_from_input(&config, argc, argv);

    cluster_map = init_empty_cluster_map();

    return run(config);
}


