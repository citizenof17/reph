//
// Created by pavel on 08.05.2020.
//

#include <pthread.h>
#include "src/util/bucket.h"
#include "src/util/cluster_map.h"
#include "src/util/netwrk.h"
#include "src/util/general.h"
#include "src/util/object.h"
#include <signal.h>

#define BETWEEN_PEER_PORT (10000)

int cluster_map_version = -1;

storage_t storage;

addr_port_t make_default_config(){
    addr_port_t config = {
            .addr = DEFAULT_ADDR,
            .port = DEFAULT_OSD_PORT
    };

    return config;
}

int init_config_from_input(addr_port_t *config, int argc, char ** argv){
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

int handle_health_check(int sock){
    int rc = 1;
    rc = ssend(sock, &rc, sizeof(rc));  // just send some random stuff
    RETURN_ON_FAILURE(rc);
    LOG("Send health check response");
    return (EXIT_SUCCESS);
}

int find_and_fill_object(object_t * obj){
    int i;
    for (i = 0; i < storage.size; i++){
        if (strcmp(storage.objects[i].key.val, obj->key.val) == 0){
            strcpy(obj->value.val, storage.objects[i].value.val);
            obj->version = storage.objects[i].version;
            break;
        }
    }
    return i;
}

int handle_get_object(int sock){
    LOG("Handling GET");
    int rc;
    object_t obj;

    rc = srecv(sock, &obj, sizeof(obj));
    RETURN_ON_FAILURE(rc);
    printf("Received object: %s %s \n", obj.key.val, obj.value.val);

    find_and_fill_object(&obj);

    rc = ssend(sock, &obj, sizeof(obj));
    RETURN_ON_FAILURE(rc);

    return (EXIT_SUCCESS);
}

int save_object(object_t * obj){
    // very costly lookup
    object_t old_obj = empty_object;
    int old_pos = find_and_fill_object(&old_obj);

    if (old_obj.version >= obj->version){
        return NEWER_VERSION_STORED;
    }
    else if (old_obj.version != -1){  // It's already in a storage but with an old value
        push2(&storage, obj, old_pos);
    }
    else{
        push2(&storage, obj, -1);
    }

    return OP_SUCCESS;
}

int handle_post_object(int sock){
    LOG("Handling POST");
    int rc;
    object_t obj;

    rc = srecv(sock, &obj, sizeof(obj));
    RETURN_ON_FAILURE(rc);
    printf("Received object: %s %s %d %d\n", obj.key.val, obj.value.val, obj.version,
           obj.primary);

    message_type_e response = save_object(&obj);
    rc = ssend(sock, &response, sizeof(response));
    RETURN_ON_FAILURE(rc);

    return (EXIT_SUCCESS);
}

int handle_update_object(int sock){
    LOG("handle_update_object NOT IMPLEMENTED");
    return (EXIT_SUCCESS);
}

int handle_delete_object(int sock){
    LOG("handle_delete_object NOT IMPLEMENTED");
    return (EXIT_SUCCESS);
}

void * handle_client_or_monitor(void * arg){
    socket_transfer_t * _params = arg;
    int sock, rc;
    sock = _params->sock;
    pthread_mutex_unlock(&_params->mutex);

    message_type_e message_type = GET_MAP;

    while (message_type != BYE){
        rc = srecv(sock, &message_type, sizeof(message_type));
        VOID_RETURN_ON_FAILURE(rc);

        fprintf(stdout, "Received message %d \n", message_type);
        fflush(NULL);

        switch(message_type){
            case HEALTH_CHECK:
                rc = handle_health_check(sock);
                VOID_RETURN_ON_FAILURE(rc);
                break;
            case POST_OBJECT:
                rc = handle_post_object(sock);
                VOID_RETURN_ON_FAILURE(rc);
                break;
            case GET_OBJECT:
                rc = handle_get_object(sock);
                VOID_RETURN_ON_FAILURE(rc);
                break;
            case UPDATE_OBJECT:
                rc = handle_update_object(sock);
                VOID_RETURN_ON_FAILURE(rc);
                break;
            case DELETE_OBJECT:
                rc = handle_delete_object(sock);
                VOID_RETURN_ON_FAILURE(rc);
                break;
            case BYE:
                LOG("End of chat, bye");
                break;
            default:
                LOG("Unprocessable message type");
                break;
        }
    }

    return ((void *)EXIT_SUCCESS);
}

void * wait_for_client_and_monitor_connection(void * arg){
    socket_transfer_t *_params = arg;
    int sock, rc;
    sock = _params->sock;
    pthread_mutex_unlock(&_params->mutex);

    // if stdout is redirected to a file, you need to fflush() it
    fprintf(stdout, "SOCK VALUE IS %d\n", sock);
    fflush(NULL);

    while (1){
        int new_sock;
        rc = saccept(sock, &new_sock);
        VOID_RETURN_ON_FAILURE(rc);

        socket_transfer_t inner_params;
        init_socket_transfer(&inner_params, new_sock);

        pthread_t thread;
        rc = pthread_create(&thread, NULL, handle_client_or_monitor, &inner_params);
        VOID_RETURN_ON_FAILURE(rc);

        pthread_mutex_lock(&inner_params.mutex);

        pthread_join(thread, NULL);
        close(new_sock);
    }
}

int start_client_and_monitor_handler(addr_port_t config, pthread_t * thread){
    int sock, rc;
    sockaddr_t local_addr = make_local_addr(config);
    rc = prepare_reusable_listening_socket(&sock, local_addr);
    RETURN_ON_FAILURE(rc);

    socket_transfer_t params;
    init_socket_transfer(&params, sock);

    rc = pthread_create(thread, NULL, wait_for_client_and_monitor_connection, &params);

    if (rc != 0) {
        perror("Error creating thread");
        close(sock);
        return (EXIT_FAILURE);
    }

    pthread_mutex_lock(&params.mutex);

    return (EXIT_SUCCESS);
}

int start_peer_poller(addr_port_t config, pthread_t * thread){
    LOG("start_peer_poller NOT IMPLEMENTED");
    return (EXIT_SUCCESS);
}

int start_peer_handler(addr_port_t config, pthread_t * thread){
    LOG("start_peer_handler NOT IMPLEMENTED");
    return (EXIT_SUCCESS);
}

int wait_for_all(pthread_t * threads, int count){
    for (int i = 0; i < count; i++){
        pthread_join(threads[i], NULL);
    }
    return (EXIT_SUCCESS);
}

int run_osd(addr_port_t config){

    const int total_number_of_threads = 3;
    pthread_t threads[total_number_of_threads];
    int rc;
    rc = start_client_and_monitor_handler(config, &threads[0]);
    RETURN_ON_FAILURE(rc);
    rc = start_peer_poller(config, &threads[1]);
    RETURN_ON_FAILURE(rc);
    rc = start_peer_handler(config, &threads[2]);
    RETURN_ON_FAILURE(rc);

    return wait_for_all(threads, total_number_of_threads);
}

void intHandler(int smt){
    print_storage2(&storage);
    printf("Error code %d\n", smt);
    exit(1);
}


int main(int argc, char ** argv){
    signal(SIGINT | SIGTERM, intHandler);

    storage.size = 0;

    addr_port_t config = make_default_config();
    init_config_from_input(&config, argc, argv);

    return run_osd(config);
}