//
// Created by pavel on 06.05.2020.
//

#ifndef REPH_GENERAL_H
#define REPH_GENERAL_H

#include "src/inner_storage/object.h"
#include "src/inner_storage/map_interface.h"

#define RETURN_ON_FAILURE(i) if ((i) != (EXIT_SUCCESS)) { return (i); }
#define VOID_RETURN_ON_FAILURE(i) if ((i) != (EXIT_SUCCESS)) { return (void *)(EXIT_FAILURE); }

typedef struct storage_linear_t {
    object_t objects[100];
    int size;
} storage_linear_t;


void init_rand();
void gen_random_string(char * dest, int size);
//void print_storage(object_t * storage, int storage_size);
//void push(object_t * storage, int * storage_size, object_t * obj);

void print_storage_linear(storage_linear_t * storage);
void push_linear(storage_linear_t * storage, object_t * obj, int pos);
void remove_linear(storage_linear_t * storage, int pos);
//void update_lin(storage_t * storage, int pos, object_t * new_obj);

void print_storage(storage_t * storage);
void push(storage_t * storage, object_t * obj);
void get(storage_t * storage, object_t * obj);
void list(storage_t * storage, object_t ** objects);
void update(storage_t * storage, object_t * obj);
void remove2(storage_t * storage, object_t * obj);
int size(storage_t * storage);

#endif //REPH_GENERAL_H
