#include <stdio.h>
#include "src/util/log.h"
#include "src/util/cluster_map.h"
#include "src/util/bucket.h"

#define INNER_SIZE (2)

int main() {

//    map_entry_t map_entry = gen_dummy_map();
//    cluster_map_t * cluster_map = build_map(map_entry);
//
//    bucket_t * bucket = cluster_map->root;
//
//    printf("%d %d %d\n", bucket->class, bucket->type,
//           bucket->c(1, 1, cluster_map->root));
//
//    for (int i = 0; i < INNER_SIZE; i++) {
//        device_t device = *bucket->inner_buckets[i]->device;
//        printf("%d %d\n", device.port, device.capacity);
//    }
//
//    int a = 1;
//    printf("%lu %lu %lu\n", sizeof(int), sizeof(void *), sizeof((void *)(a)));
    return 0;
}
