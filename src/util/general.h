//
// Created by pavel on 06.05.2020.
//

#ifndef REPH_GENERAL_H
#define REPH_GENERAL_H

#include "src/util/object.h"

#define RETURN_ON_FAILURE(i) if ((i) != (EXIT_SUCCESS)) { return (i); }
#define VOID_RETURN_ON_FAILURE(i) if ((i) != (EXIT_SUCCESS)) { return (void *)(size_t)(i); }

void init_rand();
void gen_random_string(char * dest, int size);
void print_storage(object_t * storage, int storage_size);
void push(object_t * storage, int * storage_size, object_t * obj);

#endif //REPH_GENERAL_H
