//
// Created by pavel on 04.05.2020.
//

#include <stdlib.h>
#include <assert.h>
#include "bucket.h"

int RANDOM_PRIME = 67631;

void init_uniform_bucket(bucket_t *, int);

void init_bucket(bucket_t * bucket, bucket_class_e b_class, bucket_type_e b_type, int bucket_size){
    bucket->bucket_class = b_class;
    bucket->bucket_type = b_type;

    switch(b_type) {
        case BUCKET_UNIFORM:
            init_uniform_bucket(bucket, bucket_size);
            break;
        case BUCKET_LIST:
            LOG("BUCKET_LIST NOT_IMPLEMENTED");
            break;
        case BUCKET_TREE:
            LOG("BUCKET_TREE NOT_IMPLEMENTED");
            break;
        default:
            LOG("Bucket types other than uniform are not supported");
            init_uniform_bucket(bucket, 1);
    }

    if (b_class == DEVICE){
        bucket->device = (device_t *)malloc(sizeof(device_t));
    }
}

int uniform_bucket_crush(int r, int x, bucket_t * bucket){
    int bucket_size = ((uniform_bucket_t *)(bucket->_impl))->size;
    if (bucket_size == 0){
        // This is leaf bucket and it stores a device
        return 0;
    }
    else{
        return (hash(x) + r * RANDOM_PRIME) % bucket_size;
    }
}

void init_uniform_bucket(bucket_t * bucket, int size){
    LOG("Initialize uniform bucket");
    bucket->_impl = (uniform_bucket_t *)malloc(sizeof(uniform_bucket_t));

    uniform_bucket_t * impl = (uniform_bucket_t *)(bucket->_impl);

    impl->size = size;

    // There is a memory overhead when this is a leaf bucket with device in it.
    impl->buckets = (bucket_t *)malloc(sizeof(bucket_t) * size);
    bucket->inner_buckets = impl->buckets;

    bucket->c = &uniform_bucket_crush;
}

