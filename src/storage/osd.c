
// Created by pavel on 08.05.2020.
//

#include <pthread.h>
#include <signal.h>

#include "src/util/bucket.h"
#include "src/util/cluster_map.h"
#include "src/util/netwrk.h"
#include "src/util/general.h"
#include "src/inner_storage/object.h"
#include "src/inner_storage/map_interface.h"
#include "src/inner_storage/tree_hash_map.h"
#include "src/util/mysleep.h"
#include "src/crush.h"
#include "src/shortcuts.h"

#define BETWEEN_PEER_PORT (10000)

extern int MONITOR_PORTS[];

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
    message_type_e message_type;
} object_with_addr_t;


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
//    LOG("Send health check response");
    return (EXIT_SUCCESS);
}

int find_object(storage_t * _storage, object_t * obj){
//    int i;
//    int found = False;
//    for (i = 0; i < _storage->size; i++){
//        if (strcmp(_storage->objects[i].key.val, obj->key.val) == 0){
//            found = True;
//            break;
//        }
//    }
//    if (not found){
//        i = -1;
//    }
//    return i;
    return -1;
}

int find_and_fill_object(storage_t * _storage, object_t * obj){
//    // TODO: Optimize from linear search to log(n)
//    int pos = find_object(_storage, obj);
//    if (pos != -1){
//        strcpy(obj->value.val, _storage->objects[pos].value.val);
//        obj->version = _storage->objects[pos].version;
//    }
//    return pos;
    return -1;
}

int find_and_delete_object(storage_t * _storage, object_t * obj){
//    int pos = find_object(_storage, obj);
//    if (pos != -1){
//        remove2(_storage, pos);
//    }
//    return pos;
    return -1;
}

void * put_obj(void * _obj_with_addr) {
    LOG("POST");
    object_with_addr_t * temp = _obj_with_addr;
    object_t obj = temp->obj;
    addr_port_t target_location = temp->addr_port;
    message_type_e message = temp->message_type;

    printf("Post/put %d key and value: %s %s to %s %d, version, primary: %d %d\n",
           message, obj.key.val, obj.value.val, target_location.addr, target_location.port,
           obj.version, obj.primary);

    int sock, rc;
    rc = connect_to_peer(&sock, target_location);
    VOID_RETURN_ON_FAILURE(rc)

    LOG("OSD connected to another OSD");

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

void * del_obj(void * _obj_with_addr){
    LOG("DELETE");
    object_with_addr_t * temp = _obj_with_addr;
    object_t obj = temp->obj;
    addr_port_t target_location = temp->addr_port;
    message_type_e message = temp->message_type;

    printf("Delete key and value: %s %s from %s %d, version, primary: %d %d\n",
           obj.key.val, obj.value.val, target_location.addr, target_location.port,
           obj.version, obj.primary);

    int sock, rc;
    rc = connect_to_peer(&sock, target_location);
    VOID_RETURN_ON_FAILURE(rc)
    LOG("OSD connected to another OSD");

    rc = ssend(sock, &message, sizeof(message));
    VOID_RETURN_ON_FAILURE(rc)
    LOG("Send request for DELETE");

    rc = ssend(sock, &obj, sizeof(obj));
    VOID_RETURN_ON_FAILURE(rc)
    LOG("Sent object for DELETE");

    rc = srecv(sock, &message, sizeof(message));
    VOID_RETURN_ON_FAILURE(rc)
    printf("Response code for DELETE: %d\n", message);

    send_bye(sock);
    close(sock);

    return ((void *)EXIT_SUCCESS);
}

int replicate_put(object_t * obj, addr_port_t target_location, message_type_e message_type){
    object_with_addr_t params = {
            .obj = *obj,
            .addr_port = target_location,
            .message_type = message_type,
    };
    pthread_t thread;
    int rc;
    rc = pthread_create(&thread, NULL, put_obj, &params);
    if (rc != 0){
        perror("Error creating thread");
        return (EXIT_FAILURE);
    }

    // get rid of it?
    pthread_join(thread, NULL);
    return (EXIT_SUCCESS);
}

int replicate_post(object_t * obj, addr_port_t target_location){
    LOG("REPLICATE POST");
    return replicate_put(obj, target_location, POST_OBJECT);
}

int replicate_update(object_t * obj, addr_port_t target_location){
    LOG("REPLICATE UPDATE");
    return replicate_put(obj, target_location, UPDATE_OBJECT);
}

int replicate_delete(object_t * obj, addr_port_t target_location){
    LOG("REPLICATE DELETE");
    object_with_addr_t params = {
            .obj = *obj,
            .addr_port = target_location,
            .message_type = DELETE_OBJECT,
    };
    pthread_t thread;
    int rc;
    rc = pthread_create(&thread, NULL, del_obj, &params);
    if (rc != 0){
        perror("Error creating thread");
        return (EXIT_FAILURE);
    }

    // get rid of it?
    pthread_join(thread, NULL);
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
            case UPDATE_OBJECT:
                replicate_update(obj, target_addr);
                break;
            case DELETE_OBJECT:
                replicate_delete(obj, target_addr);
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

    // actually found (active/alive) osd count might be less that expected replication factor
    replicate_for_multiple(res, obj, operation);
    clear_crush_result(&select_from);
    clear_crush_result(&res);
}

storage_t * get_storage(int primary){
    return primary ? &primary_storage : &secondary_storage;
}

int save_object(object_t * obj, int update_needed){
    object_t old_obj;
    memset(&old_obj, 0, sizeof(object_t));
    strcpy(old_obj.key.val, obj->key.val);

    storage_t * _storage = get_storage(obj->primary);

    get(_storage, &old_obj);

    printf("Found old obj (version, primary, key, val): %d %d %s %s\n",
           old_obj.version, old_obj.primary, old_obj.key.val, old_obj.value.val);

    if (old_obj.version >= obj->version and !update_needed){  // TODO: shall I care about update here?
        LOG("Newer version stored");
        return NEWER_VERSION_STORED;
    }
    else if (old_obj.version != 0){
        if (strcmp(old_obj.value.val, obj->value.val) == 0){
            // It's already store and replication is not needed
            return OP_SUCCESS;
        }
        update(_storage, obj);
        return OP_SUCCESS;
    }
    else {
//        if (update and old_pos != -1){
            push(_storage, obj);
//        }
//        else{
//            push(_storage, obj);
//        }

        if (obj->primary){
            obj->primary = 0;
            replicate(obj, update_needed ? UPDATE_OBJECT : POST_OBJECT);
        }
    }

    return OP_SUCCESS;
}

int remove_object(object_t * obj){
    storage_t * _storage = get_storage(obj->primary);

    remove2(_storage, obj);

    if (obj->version == 0){
        // Try to find object in alternative storage
        _storage = get_storage(!obj->primary);
        remove2(_storage, obj);
        if (obj->version == 0) {
            printf("Object for delete NOT found, key: %s\n", obj->key.val);
            return NOT_FOUND;
        }
        printf("Object for delete is found, key: %s\n", obj->key.val);
    }

    if (obj->primary){
        obj->primary = 0;
        replicate(obj, DELETE_OBJECT);
    }
    else {
        LOG("Storage is not primary, delete is not replicated");
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
    get(get_storage(obj.primary), &obj);
    if (obj.version == 0){
        // Object was not found in the expected storage, try to find it in another one
        // This might happen when current OSD is secondary
        get(get_storage(!obj.primary), &obj);
    }

    printf("Found object for GET: %s %s\n", obj.key.val, obj.value.val);

    rc = ssend(sock, &obj, sizeof(obj));
    RETURN_ON_FAILURE(rc);

    return (EXIT_SUCCESS);
}

int handle_put_object(int sock, int update){
    LOG("Handling POST/PUT");
    printf("Operation %s\n", update ? "UPDATE" : "POST");

    int rc;
    object_t obj;
    memset(&obj, 0, sizeof(obj));

    rc = srecv(sock, &obj, sizeof(obj));
    RETURN_ON_FAILURE(rc);
    printf("Received object: %s %s %d %d\n", obj.key.val, obj.value.val, obj.version,
           obj.primary);

    message_type_e response = save_object(&obj, update);
    rc = ssend(sock, &response, sizeof(response));
    RETURN_ON_FAILURE(rc);
    print_storage(&primary_storage);
    print_storage(&secondary_storage);

    return (EXIT_SUCCESS);
}

int handle_post_object(int sock){
    return handle_put_object(sock, 0);
}

int handle_update_object(int sock){
/* This will work as PUT - create new object or update the old one */
    return handle_put_object(sock, 1);
}

int handle_delete_object(int sock){
    LOG("Handling DELETE");
    int rc;
    object_t obj;
    memset(&obj, 0, sizeof(obj));

    rc = srecv(sock, &obj, sizeof(obj));
    RETURN_ON_FAILURE(rc);
    printf("Received key for DELETE object: %s\n", obj.key.val);

    message_type_e response = remove_object(&obj);
    rc = ssend(sock, &response, sizeof(response));
    RETURN_ON_FAILURE(rc);
    print_storage(&primary_storage);
    print_storage(&secondary_storage);

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

//        fprintf(stdout, "Received message %d \n", message_type);
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
//                LOG("End of chat, bye");
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

int is_primary(crush_result_t * crush_res, addr_port_t * location){
    if (crush_res->size > 0){
        addr_port_t new_primary_location = crush_res->buckets[0]->device->location;
        return addr_cmp(&new_primary_location, location) == 0;
    }
    return True;
}

int am_i_primary(crush_result_t * crush_result){
    return is_primary(crush_result, &self_config);
}

void process_secondaries(){
    LOG("Processing secondaries");
    crush_result_t * select_from = init_crush_input(1, cluster_map->root);

    int sz = size(&secondary_storage);
    object_t ** objects = malloc(sizeof(object_t *) * sz);

    list(&secondary_storage, objects);

    int i;
    for (i = 0; i < sz; i++){
        object_t obj = *objects[i];

        crush_result_t * res = crush_select(select_from, DEVICE, replicas_factor,
                                            obj_hash(&obj));

        if (res->size > 0){
            obj.primary = 1;
            if (am_i_primary(res)){
                save_object(&obj, 0);
                remove2(&secondary_storage, &obj);
                continue;
            }

            // Check if primary of the group is changed and replicate data for it
            // TODO: actually check for primary change, as now we just unconditionally
            //  sending data
            addr_port_t new_primary_location = res->buckets[0]->device->location;
            replicate_post(&obj, new_primary_location);
        }
        // Check if we are still in the list of secondaries
        if (!in_crush(&self_config, res)){
            remove2(&secondary_storage, &obj);
            continue;
        }
        clear_crush_result(&res);
    }
    free(objects);
    clear_crush_result(&select_from);
}

void process_primaries(){
    LOG("Processing primaries");

    // find new primary/secondaries for the object. If we are not primary anymore,
    // send object to a new primary.
    crush_result_t * select_from = init_crush_input(1, cluster_map->root);

    int sz = size(&primary_storage);
    object_t ** objects = malloc(sizeof(object_t *) * sz);

    list(&primary_storage, objects);

    int i;
    for (i = 0; i < sz; i++){
        object_t obj = *objects[i];

        crush_result_t * res = crush_select(select_from, DEVICE, replicas_factor,
                                            obj_hash(&obj));

        if (res->size > 0){
            // Check if we are not a primary anymore and send object to a new primary
            if (!am_i_primary(res)){
                addr_port_t new_primary_location = res->buckets[0]->device->location;
                replicate_post(&obj, new_primary_location);
                remove2(&primary_storage, &obj);
                continue;
            }
        }

        if (res->size == 0){
            remove2(&primary_storage, &obj);
            continue;
        }

        // We are good. Still a primary, let's replicate to secondaries as they might've
        // changed
        obj.primary = 0;
        replicate_for_multiple(res, &obj, POST_OBJECT);
        clear_crush_result(&res);
    }
    clear_crush_result(&select_from);
    free(objects);
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
            print_storage(&primary_storage);
            print_storage(&secondary_storage);
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
    const int total_number_of_threads = 3;
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
    print_storage(&primary_storage);
    printf("Secondary storage:\n");
    print_storage(&secondary_storage);
    printf("Error code %d\n", smt);
    fflush(NULL);
    exit(1);
}


int main(int argc, char ** argv){
    signal(SIGINT, intHandler);
    signal(SIGTERM, intHandler);

//    primary_storage.size = 0;
//    secondary_storage.size = 0;

    tree_hash_map_init(&primary_storage);
    tree_hash_map_init(&secondary_storage);

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