//
// Created by pavel on 04.05.2020.
//

#ifndef REPH_CLUSTER_MAP_H
#define REPH_CLUSTER_MAP_H

#include <stdlib.h>
#include "src/util/bucket.h"
#include "src/util/device.h"
#include "src/util/general.h"

#define DEFAULT_MAP_SIZE (1024)
#define SHORT_MAP_SIZE (32)

typedef struct cluster_map_t {
    bucket_t * root;
    int version;
} cluster_map_t;

typedef struct map_entry_t {
    bucket_type_e type;
    bucket_class_e class;
    char addr[ADDR_SIZE];
    int port;
    int capacity;
    struct map_entry_t * inner;
    int inner_size;
} map_entry_t;


char * get_string_map_representation(FILE * fin);

map_entry_t gen_dummy_device_map_entry();
map_entry_t gen_dummy_map();
map_entry_t make_map_entry_from_string(char *map_string);

cluster_map_t * build_map_from_file(FILE * fin);
cluster_map_t * build_map_from_string(char * plane_map);
cluster_map_t * build_map(map_entry_t map_entry);
cluster_map_t * init_empty_cluster_map();

int get_map_version(int sock, int * new_map_version);
int get_cluster_map(int sock, char * plane_cluster_map);
int update_map_if_needed(net_config_t config, cluster_map_t ** cluster_map);
#endif //REPH_CLUSTER_MAP_H
