#include <stdio.h>
#include "src/util/log.h"
#include "src/util/cluster_map.h"
#include "src/util/bucket.h"

#define INNER_SIZE (2)


map_entry_t gen_dummy_device_map_entry(){
    map_entry_t map_entry = {
        .inner_size = 0,
        .class = DEVICE,
        .type = BUCKET_UNIFORM,
        .capacity = 100,
        .port = 141,
        .addr = "127.0.0.1",
    };
    return map_entry;
}

map_entry_t gen_dummy_map(){
    map_entry_t map_entry;
    map_entry.class = RACK;
    map_entry.type = BUCKET_UNIFORM;

    map_entry.inner_size = INNER_SIZE;
    map_entry.inner = (map_entry_t *)malloc(sizeof(map_entry_t) * INNER_SIZE);
    for (int i = 0; i < INNER_SIZE; i++){
        map_entry.inner[i] = gen_dummy_device_map_entry();
    }

    return map_entry;
}

int main() {

    map_entry_t map_entry = gen_dummy_map();
    cluster_map_t * cluster_map = build_map(map_entry);

    bucket_t * bucket = cluster_map->root;

    printf("%d %d %d\n", bucket->bucket_class, bucket->bucket_type,
           bucket->c(1, 1, cluster_map->root));

    for (int i = 0; i < INNER_SIZE; i++) {
        device_t device = *bucket->inner_buckets[i].device;
        printf("%d %d\n", device.port, device.capacity);
    }

    LOG("Hello world\n");

    return 0;
}
