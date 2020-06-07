//
// Created by pavel on 07.05.2020.
//

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include "general.h"

void init_rand(){
    srand(time(NULL));
}

void gen_random_string(char * dest, int size){
    for(int i = 0; i < size; i++){
        dest[i] = (char)(rand() % 26 + 'a');
    }
    dest[size] = '\0';
}

void print_storage(object_t * storage, int storage_size){
    printf("------------------\n");
    printf("Storage size: %d\n", storage_size);
    printf("Storage entries:\n");
    for (int i = 0; i < storage_size; i++){
        printf("%s, %s\n", storage[i].key.val, storage[i].value.val);
    }
    printf("------------------\n");
}

void push(object_t * storage, int * storage_size, object_t * obj){
    object_t new_obj = {
            .key = obj->key,
            .value = obj->value,
    };
    storage[*storage_size] = new_obj;
    *storage_size = *(storage_size) + 1;
}