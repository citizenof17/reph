//
// Created by pavel on 07.05.2020.
//

#ifndef REPH_CRUSH_H
#define REPH_CRUSH_H

#include "src/util/bucket.h"
#include <stdarg.h>

typedef struct crush_result_t {
    bucket_t ** buckets;
    int size;
} crush_result_t;

crush_result_t * crush_select(crush_result_t * input, bucket_class_e t, int n);
crush_result_t * init_crush_input(int num, ...);

#endif //REPH_CRUSH_H
