//
// Created by pavel on 04.05.2020.
//

#include <stdlib.h>
#include <assert.h>
#include "bucket.h"

int RANDOM_PRIME = 67631;

bucket_t * uniform_bucket_crush(int r, int x, bucket_t * bucket){
    int bucket_size = ((uniform_bucket_t *)(bucket->_impl))->size;
    if (bucket_size == 0){
        // This is a leaf bucket and it stores a device
        return bucket;
//        return 0;
    }
    else {
        bucket_size = 2;
        int idx = (hash(x) + r * RANDOM_PRIME) % bucket_size;
        return bucket->inner_buckets[idx];
    }
}

void init_uniform_bucket(bucket_t * bucket, int size){
    LOG("Initialize uniform bucket");

    uniform_bucket_t * impl = malloc(sizeof(uniform_bucket_t));
    bucket->_impl = (void *)impl;
    impl->size = size;

    impl->buckets = malloc(sizeof(bucket_t *) * size);
    for (int i = 0; i < size; i++){
        impl->buckets[i] = malloc(sizeof(bucket_t));
    }
    bucket->inner_buckets = impl->buckets;

    bucket->c = &uniform_bucket_crush;
}

void init_bucket(bucket_t * bucket, bucket_class_e b_class, bucket_type_e b_type,
                 int b_size){
    bucket->type = b_type;
    bucket->class = b_class;
    bucket->size = b_size;

    switch(b_type) {
        case BUCKET_UNIFORM:
            init_uniform_bucket(bucket, b_size);
            break;
        case BUCKET_LIST:
            LOG("BUCKET_LIST NOT_IMPLEMENTED");
            break;
        case BUCKET_TREE:
            LOG("BUCKET_TREE NOT_IMPLEMENTED");
            break;
        default:
            LOG("Other bucket types are not supported");
            init_uniform_bucket(bucket, 1);
    }
}
