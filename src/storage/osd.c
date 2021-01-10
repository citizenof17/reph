
// Created by pavel on 08.05.2020.
//

#include <pthread.h>
#include <signal.h>

#include "src/util/bucket.h"
#include "src/util/cluster_map.h"
#include "src/util/netwrk.h"
#include "src/util/general.h"
#include "src/util/object.h"
#include "src/util/mysleep.h"
#include "src/crush.h"

#define BETWEEN_PEER_PORT (10000)

int replicas_factor = 2;
pthread_mutex_t cluster_map_mutex;
cluster_map_t * cluster_map;
addr_port_t self_config;

pthread_mutex_t recovery_mutex;
int recovery_needed;

storage_t primary_storage;    // objects for which we are primary osd
storage_t secondary_storage;  // objects for which we are secondary osd


net_config_t make_default_config(){
    net_config_t config = {
            .self = {
                    .addr = DEFAULT_ADDR,
                    .port = DEFAULT_OSD_PORT,
            },
            .monitor = {
                    .addr = DEFAULT_ADDR,
                    .port = DEFAULT_MONITOR_PORT,
            }
    };
    return config;
}

typedef struct object_with_addr_t {
    object_t obj;
    addr_port_t addr_port;
} object_with_addr_t;

typedef struct config_transfer_t {
    net_config_t config;
    pthread_mutex_t mutex;
} config_transfer_t;

void init_config_transfer(config_transfer_t * transfer, net_config_t config){
    transfer->config = config;
    pthread_mutex_init(&transfer->mutex, NULL);
    pthread_mutex_lock(&transfer->mutex);
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

int find_and_fill_object(storage_t * _storage, object_t * obj){
    int i;
    for (i = 0; i < _storage->size; i++){
        if (strcmp(_storage->objects[i].key.val, obj->key.val) == 0){
            strcpy(obj->value.val, _storage->objects[i].value.val);
            obj->version = _storage->objects[i].version;
            break;
        }
    }
    return i;
}

void * post_obj(void * _obj_with_addr) {
    LOG("POST");
    object_with_addr_t * temp = _obj_with_addr;
    object_t obj = temp->obj;
    addr_port_t target_location = temp->addr_port;

    printf("Posting key and value: %s %s to %s %d, version, primary: %d %d\n",
           obj.key.val, obj.value.val, target_location.addr, target_location.port,
           obj.version, obj.primary);

    int sock, rc;
    rc = connect_to_peer(&sock, target_location);
    VOID_RETURN_ON_FAILURE(rc)

    LOG("OSD connected to another OSD");

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

int replicate_post(object_t * obj, addr_port_t target_location){
    LOG("REPLICATION");

    object_with_addr_t params = {
            .obj = *obj,
            .addr_port = target_location,
    };
    pthread_t thread;
    int rc;
    rc = pthread_create(&thread, NULL, post_obj, &params);
    if (rc != 0){
        perror("Error creating thread");
        return (EXIT_FAILURE);
    }

    // get rid of it?
    pthread_join(thread, NULL);
    return (EXIT_SUCCESS);
}

int replicate_delete(object_t * obj){
    LOG("NOT IMPLEMENTED replicate_delete");
    return (EXIT_SUCCESS);
}

void replicate_for_multiple(crush_result_t * crush_res, object_t * obj,
                            message_type_e operation){
    int i;
    for (i = 0; i < crush_res->size; i++){
        addr_port_t target_addr = crush_res->buckets[i]->device->location;
        if (addr_cmp(&target_addr, &self_config) == 0){
            continue;
        }

        switch (operation) {
            case POST_OBJECT:
                replicate_post(obj, target_addr);
                break;
            case DELETE_OBJECT:
                replicate_delete(obj);
                break;
            default:
                break;
        };
    }
}

void replicate(object_t * obj, message_type_e operation){
    crush_result_t * select_from = init_crush_input(1, cluster_map->root);
    crush_result_t * res = crush_select(select_from, DEVICE, replicas_factor,
                                        obj_hash(obj));

    // actually found (active/alive) osd count might be less that expected replication
    // factor
    replicate_for_multiple(res, obj, operation);
    clear_crush_result(&select_from);
    clear_crush_result(&res);
}

storage_t * get_storage(int primary){
    return primary ? &primary_storage : &secondary_storage;
}

int save_object(object_t * obj){
    // very costly lookup, can be optimised with hash table?
    object_t old_obj;
    memset(&old_obj, 0, sizeof(object_t));
    strcpy(old_obj.key.val, obj->key.val);

    storage_t * _storage = get_storage(obj->primary);

    int old_pos = find_and_fill_object(_storage, &old_obj);

    printf("Found old obj (pos, version, primary, key, val): %d %d %d %s %s\n",
           old_pos, old_obj.version, old_obj.primary, old_obj.key.val, old_obj.value.val);

    if (old_obj.version >= obj->version){
        return NEWER_VERSION_STORED;
    }
//    else if (old_obj.version != 0){
//        if (strcmp(old_obj.value.val, obj->value.val) == 0){
//            // It's already store and replication is not needed
//            return OP_SUCCESS;
//        }
//        update2(_storage, old_pos, obj);
//        return OP_SUCCESS;
//    }
    else {
        push2(_storage, obj, -1);

        if (obj->primary){
            obj->primary = 0;
            replicate(obj, POST_OBJECT);
        }
    }

    return OP_SUCCESS;
}

int handle_get_object(int sock){
    LOG("Handling GET");
    int rc;
    object_t obj;
    memset(&obj, 0, sizeof(obj));

    rc = srecv(sock, &obj, sizeof(obj));
    RETURN_ON_FAILURE(rc);
    printf("Received key for GET object: %s\n", obj.key.val);

    // Look in primary_storage first
    obj.primary = 1;
    find_and_fill_object(get_storage(obj.primary), &obj);
    if (obj.version == 0){
        // Object was not found in the expected storage, try to find it in another one
        // This might happen when current OSD is secondary
        find_and_fill_object(get_storage(!obj.primary), &obj);
    }

    printf("Found object for GET: %s %s\n", obj.key.val, obj.value.val);

    rc = ssend(sock, &obj, sizeof(obj));
    RETURN_ON_FAILURE(rc);

    return (EXIT_SUCCESS);
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
    print_storage2(&primary_storage);
    print_storage2(&secondary_storage);

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

    message_type_e message_type = HEALTH_CHECK;

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

void * poll_cluster_map(void * arg){
    config_transfer_t * _params = arg;
    net_config_t config = _params->config;
    pthread_mutex_unlock(&_params->mutex);

    while (1) {
        // TODO: Need to update_map.. more frequently than recovery
        int rc;
        int old_version = cluster_map->version;
        pthread_mutex_lock(&cluster_map_mutex);
        rc = update_map_if_needed(config, &cluster_map);
        pthread_mutex_unlock(&cluster_map_mutex);
        VOID_RETURN_ON_FAILURE(rc);

        if (cluster_map->version > old_version){
            pthread_mutex_lock(&recovery_mutex);
            recovery_needed = 1;
            pthread_mutex_unlock(&recovery_mutex);
        }
        mysleep(10000);
    }
}

int start_monitor_poller(net_config_t config, pthread_t * thread) {
    config_transfer_t params;
    init_config_transfer(&params, config);

    int rc;
    rc = pthread_create(thread, NULL, poll_cluster_map, &params);
    if (rc != 0) {
        perror("Error creating thread");
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

int in_crush(addr_port_t * addr_to_check, crush_result_t * crush_res){
    int i;
    for (i = 0; i < crush_res->size; i++){
        if (addr_cmp(&crush_res->buckets[i]->device->location, addr_to_check) == 0){
            return 1;
        }
    }
    return 0;
}

void process_secondaries(){
    LOG("Processing secondaries");
    int i;
    crush_result_t * select_from = init_crush_input(1, cluster_map->root);
    for (i = 0; i < secondary_storage.size;){
        object_t obj = secondary_storage.objects[i];

        crush_result_t * res = crush_select(select_from, DEVICE, replicas_factor,
                                            obj_hash(&obj));

        if (res->size > 0){
            obj.primary = 1;
            // Check if we are new primary
            addr_port_t new_primary_location = res->buckets[0]->device->location;
            if (addr_cmp(&new_primary_location, &self_config) == 0){
                save_object(&obj);
                remove2(&secondary_storage, i);
                continue;
            }

            // Check if primary of the group is changed and replicate data for it
            // TODO: (actually check for primary change, as now we just unconditionally
            //  sending data)
            replicate_post(&obj, new_primary_location);
        }
        // Check if we are still in the list of secondaries
        if (!in_crush(&self_config, res)){
            remove2(&secondary_storage, i);
            continue;
        }
        clear_crush_result(&res);
        i++;
    }
    clear_crush_result(&select_from);
}

void process_primaries(){
    LOG("Processing primaries");

    // find new primary/secondaries for the object. If we are not primary anymore,
    // send object to a new primary.
    int i;
    crush_result_t * select_from = init_crush_input(1, cluster_map->root);
    for (i = 0; i < primary_storage.size;){
        object_t obj = primary_storage.objects[i];

        crush_result_t * res = crush_select(select_from, DEVICE, replicas_factor,
                                            obj_hash(&obj));

        if (res->size > 0){
            // Check if we are not a primary anymore and send object to a new primary
            addr_port_t new_primary_location = res->buckets[0]->device->location;
            if (addr_cmp(&new_primary_location, &self_config) != 0){
                replicate_post(&obj, new_primary_location);
                remove2(&primary_storage, i);
                continue;
            }
        }

        if (res->size == 0){
            remove2(&primary_storage, i);
            continue;
        }

        // We are good. Still a primary, let's replicate to secondaries as they might've
        // changed
        obj.primary = 0;
        replicate_for_multiple(res, &obj, POST_OBJECT);
        clear_crush_result(&res);
        i++;
    }
    clear_crush_result(&select_from);
}

_Noreturn void * perform_recovery(void * arg){
    LOG("Started recovery subprocess");
    // objects from secondary_storage:
    // move from secondary_storage to primary_storage if we are now primary on an object
    // (and also replicate it);
    // send object from secondary_storage to primary if it's (osd) state is changed;
    // remove object from secondary_storage if we are not primary/secondary OSD anymore;

    // objects from primary_storage:
    // find new primary/secondaries for the object. If we are not primary anymore,
    // send object to a new primary. If we are not in the list of secondaries remove it
    // from our storage or move to secondary_storage.

    config_transfer_t * _params = arg;
    net_config_t config = _params->config;
    pthread_mutex_unlock(&_params->mutex);

    while (1) {
        pthread_mutex_lock(&recovery_mutex);
        int _recovery_needed = recovery_needed;
        pthread_mutex_unlock(&recovery_mutex);

        if (_recovery_needed){
            LOG("Recovery is needed");

            process_secondaries();
            process_primaries();

            pthread_mutex_lock(&recovery_mutex);
            recovery_needed = 0;
            pthread_mutex_unlock(&recovery_mutex);
            print_storage2(&primary_storage);
            print_storage2(&secondary_storage);
        }

        mysleep(5000);
    }
}

int start_recovery_process(net_config_t config, pthread_t * thread){
    config_transfer_t params;
    init_config_transfer(&params, config);

    int rc;
    rc = pthread_create(thread, NULL, perform_recovery, &params);
    if (rc != 0) {
        perror("Error creating thread");
        return (EXIT_FAILURE);
    }
    pthread_mutex_lock(&params.mutex);

    return (EXIT_SUCCESS);
}

int wait_for_all(pthread_t * threads, int count){
    for (int i = 0; i < count; i++){
        pthread_join(threads[i], NULL);
    }
    return (EXIT_SUCCESS);
}

int inc_(int * counter){
    int val = *counter;
    ++(*counter);
    return val;
}

int run(net_config_t config){
    LOG("RUNNING");
    const int total_number_of_threads = 5;
    pthread_t threads[total_number_of_threads];
    int rc;
    int thread_num = 0;

    rc = start_monitor_poller(config, &threads[inc_(&thread_num)]);
    RETURN_ON_FAILURE(rc);

    while(cluster_map->root == NULL){
        mysleep(100);
    }

    LOG("Starting client and monitor handler");
    rc = start_client_and_monitor_handler(config.self, &threads[inc_(&thread_num)]);
    RETURN_ON_FAILURE(rc);
//    rc = start_peer_poller(config.self, &threads[inc_(&thread_num)]);
//    RETURN_ON_FAILURE(rc);
//    rc = start_peer_handler(config.self, &threads[inc_(&thread_num)]);
//    RETURN_ON_FAILURE(rc);
    LOG("Starting recovery");
    rc = start_recovery_process(config, &threads[inc_(&thread_num)]);
    RETURN_ON_FAILURE(rc);

    return wait_for_all(threads, total_number_of_threads);
}

void intHandler(int smt){
    printf("Primary storage:\n");
    print_storage2(&primary_storage);
    printf("Secondary storage:\n");
    print_storage2(&secondary_storage);
    printf("Error code %d\n", smt);
    fflush(NULL);
    exit(1);
}


int main(int argc, char ** argv){
    signal(SIGINT, intHandler);
    signal(SIGTERM, intHandler);

    primary_storage.size = 0;
    secondary_storage.size = 0;

    recovery_needed = 0;
    pthread_mutex_init(&recovery_mutex, NULL);

    net_config_t config = make_default_config();
    init_config_from_input(&(config.self), argc, argv);
    self_config = config.self;

    cluster_map = init_empty_cluster_map();
    pthread_mutex_init(&cluster_map_mutex, NULL);

    fflush(NULL);
    return run(config);
}