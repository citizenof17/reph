//
// Created by pavel on 04.05.2020.
//

#ifndef REPH_CLUSTER_MAP_H
#define REPH_CLUSTER_MAP_H

#include <stdlib.h>
#include <stdio.h>
#include "src/util/bucket.h"
#include "src/util/device.h"
#include "src/util/general.h"

#define DEFAULT_MAP_SIZE (2000)

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


map_entry_t gen_dummy_device_map_entry();
map_entry_t gen_dummy_map();

cluster_map_t * build_map(map_entry_t map_entry);
cluster_map_t * init_empty_cluster_map();
cluster_map_t * build_default_map();

int get_map_version(int sock, int * new_map_version);
int get_cluster_map(int sock, char * plane_cluster_map);
int update_map_if_needed(net_config_t config, cluster_map_t ** cluster_map);

char * bucket_to_string(bucket_t * bucket);
char * cluster_map_to_string(cluster_map_t * cluster_map);
cluster_map_t * cluster_map_from_string(char * str);
cluster_map_t * cluster_map_from_file(char * filepath);

void print_cluster_map(cluster_map_t * map);

#endif //REPH_CLUSTER_MAP_H
