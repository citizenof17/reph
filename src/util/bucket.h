//
// Created by pavel on 04.05.2020.
//

#ifndef REPH_BUCKET_H
#define REPH_BUCKET_H

#include "src/util/hash.h"
#include "src/util/log.h"
#include "src/util/device.h"

typedef enum bucket_class_e {
    DEVICE,  // DISK
    ROW,
    CABINET,
    RACK,
    ROOM,
} bucket_class_e;

typedef enum bucket_type_e {
    BUCKET_UNIFORM,
    BUCKET_LIST,
    BUCKET_TREE
} bucket_type_e;

typedef struct bucket_t {
    bucket_type_e bucket_type;
    bucket_class_e bucket_class;
    int (*c) (int r, int x, struct bucket_t *); // function used to calculate CRUSH
                                                // from input value of x and a
                                                // replica number r
    device_t * device;
    // This probably should be changed to pointers array (defined with **)
    struct bucket_t * inner_buckets;

    void * _impl;
} bucket_t;

typedef struct uniform_bucket_t {
    int weight;
    int size;
    bucket_t * buckets;
} uniform_bucket_t;

void init_bucket(bucket_t * ,bucket_class_e, bucket_type_e, int bucket_size);
//void init_uniform_bucket(bucket_t * bucket);

#endif //REPH_BUCKET_H
