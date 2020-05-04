//
// Created by pavel on 04.05.2020.
//

#include <assert.h>
#include "src/util/cluster_map.h"

cluster_map_t * build_map_dummy(){
    int BUCKET_COUNT = 5;

    cluster_map_t * cluster_map = (cluster_map_t *)malloc(sizeof(cluster_map_t));
    cluster_map->root = (bucket_t *)malloc(sizeof(bucket_t) * BUCKET_COUNT);

    return cluster_map;
}

char * parse_json_to_string(FILE * fin){
    LOG("NOT IMPLEMENTED");
    return NULL;
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
        init_device(bucket->device, map_entry.capacity, map_entry.port, map_entry.addr);
    }

    for (int i = 0; i < map_entry.inner_size; i++) {
        build_map_bucket(&bucket->inner_buckets[i], map_entry.inner[i]);
    }
}

cluster_map_t * build_map(map_entry_t map_entry) {
    cluster_map_t * cluster_map = (cluster_map_t *)malloc(sizeof(cluster_map_t));
    cluster_map->root = (bucket_t *)malloc(sizeof(bucket_t));
    build_map_bucket(cluster_map->root, map_entry);
    return cluster_map;
}

cluster_map_t * build_map_from_file(FILE * fin){
    char * map_repr = parse_json_to_string(fin);
    map_entry_t map_entry = parse_string_to_map_entry(map_repr);
    return build_map(map_entry);
}
