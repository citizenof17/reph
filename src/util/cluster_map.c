//
// Created by pavel on 04.05.2020.
//

#include <assert.h>
#include "src/util/cluster_map.h"


map_entry_t gen_dummy_device_map_entry(){
    map_entry_t map_entry = {
        .inner_size = 0,
        .class = DEVICE,
        .type = BUCKET_UNIFORM,
        .capacity = 100,
        .port = 4242,
        .addr = "127.0.0.1",
    };
    return map_entry;
}

map_entry_t gen_dummy_map(){
    map_entry_t map_entry;
    map_entry.class = RACK;
    map_entry.type = BUCKET_UNIFORM;

    const int INNER_SIZE = 5;
    map_entry.inner_size = INNER_SIZE;
    map_entry.inner = (map_entry_t *)malloc(sizeof(map_entry_t) * INNER_SIZE);
    for (int i = 0; i < INNER_SIZE; i++){
        map_entry.inner[i] = gen_dummy_device_map_entry();
        map_entry.inner[i].port += i;
    }

    return map_entry;
}

char * parse_json_to_string(FILE * fin){
//    LOG("NOT IMPLEMENTED");
    char * plane_map = (char *)malloc(sizeof(char) * DEFAULT_MAP_SIZE);
    return plane_map;
}

map_entry_t parse_string_to_map_entry(char *map_repr){
    LOG("NOT IMPLEMENTED");
    map_entry_t map_entry;
    map_entry.port = 0;
    return map_entry;
}

void build_map_bucket(bucket_t * bucket, map_entry_t map_entry){
    init_bucket(bucket, map_entry.class, map_entry.type, map_entry.inner_size);

    if (map_entry.class == DEVICE){
        assert(map_entry.inner_size == 0);
        bucket->device = (device_t *)malloc(sizeof(device_t));
        init_device(bucket->device, map_entry.capacity, map_entry.port, map_entry.addr);
    }

    for (int i = 0; i < map_entry.inner_size; i++) {
        build_map_bucket(bucket->inner_buckets[i], map_entry.inner[i]);
    }
}

map_entry_t make_map_entry_from_string(char *map_string){
// This function should parse json string to build map_entry_t - which is raw
// representation of the cluster
//    LOG("NOT IMPLEMENTED");
    return gen_dummy_map();
}

char * get_string_map_representation(FILE * fin){
    char * string_map = parse_json_to_string(fin);
    return string_map;
}

cluster_map_t * build_map(map_entry_t map_entry) {
    cluster_map_t * cluster_map = (cluster_map_t *)malloc(sizeof(cluster_map_t));
    cluster_map->root = (bucket_t *)malloc(sizeof(bucket_t));
    build_map_bucket(cluster_map->root, map_entry);
    return cluster_map;
}

cluster_map_t * build_map_from_file(FILE * fin){
    char * string_map = get_string_map_representation(fin);
    map_entry_t map_entry = make_map_entry_from_string(string_map);
    return build_map(map_entry);
}

cluster_map_t * build_map_from_string(char * plane_map){
    map_entry_t map_entry = make_map_entry_from_string(plane_map);
    cluster_map_t * cl = build_map(map_entry);
    return cl;
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

int get_cluster_map(int sock, char * plane_cluster_map){
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


void free_bucket(bucket_t * bucket){
    if (bucket == NULL){
        return;
    }

    int i;
    for (i = 0; i < bucket->size; i++){
        free_bucket(bucket->inner_buckets[i]);
    }
    if (bucket->_impl != NULL) {
        free(bucket->inner_buckets);
        free(bucket->_impl);
    }
    if (bucket->class == DEVICE) {
        free(bucket->device);
    }
//    free(bucket);
}

void free_map(cluster_map_t * cluster_map){
    free_bucket(cluster_map->root);
//    free(cluster_map);
}

int update_map_if_needed(net_config_t config, cluster_map_t ** cluster_map){
    int sock, rc;
    rc = connect_to_peer(&sock, config.monitor);
    RETURN_ON_FAILURE(rc);

    LOG("Client connected to monitor");

    int new_map_version;
    rc = get_map_version(sock, &new_map_version);
    RETURN_ON_FAILURE(rc);

    printf("New cluster map version %d\n", new_map_version);

    char plane_cluster_map[DEFAULT_MAP_SIZE];
    if (new_map_version > (*cluster_map)->version || (*cluster_map)->version <= 0){
        LOG("Cluster map version is updated, need to refresh its content");
        rc = get_cluster_map(sock, plane_cluster_map);
        RETURN_ON_FAILURE(rc);

        free_map(*cluster_map);
        free(*cluster_map);
        *cluster_map = build_map_from_string(plane_cluster_map);
        (*cluster_map)->version = new_map_version;
    }

    send_bye(sock);
    close(sock);

    return (EXIT_SUCCESS);
}

cluster_map_t * init_empty_cluster_map(){
    cluster_map_t * cluster_map = (cluster_map_t *)malloc(sizeof(cluster_map_t));
    cluster_map->version = -1;
    cluster_map->root = NULL;
    return cluster_map;
}