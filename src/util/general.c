//
// Created by pavel on 07.05.2020.
//

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include "general.h"
#include "src/inner_storage/map_interface.h"

void init_rand(){
    srand(time(NULL));
}

void gen_random_string(char * dest, int size){
    for(int i = 0; i < size; i++){
        dest[i] = (char)(rand() % 26 + 'a');
    }
    dest[size] = '\0';
}

//void print_storage(object_t * storage, int storage_size){
//    printf("------------------\n");
//    printf("Storage size: %d\n", storage_size);
//    printf("Storage entries:\n");
//    for (int i = 0; i < storage_size; i++){
//        printf("%s, %s\n", storage[i].key.val, storage[i].value.val);
//    }
//    printf("------------------\n");
//}

//void push(object_t * storage, int * storage_size, object_t * obj){
//    object_t new_obj = {
//            .key = obj->key,
//            .value = obj->value,
//    };
//    storage[*storage_size] = new_obj;
//    *storage_size = *(storage_size) + 1;
//}

void print_storage_linear(storage_linear_t * storage){
    printf("------------------\n");
    printf("Storage size: %d\n", storage->size);
    printf("Storage entries:\n");
    for (int i = 0; i < storage->size; i++){
        printf("%s, %s, version: %d, primary: %d\n",
               storage->objects[i].key.val, storage->objects[i].value.val,
               storage->objects[i].version, storage->objects[i].primary);
    }
    printf("------------------\n");
}

void push_linear(storage_linear_t * storage, object_t * obj, int pos){
    object_t new_obj = {
            .key = obj->key,
            .value = obj->value,
            .version = obj->version,
            .primary = obj->primary,
    };
    if (pos != -1){
        storage->objects[pos] = new_obj;
    }
    else{
        storage->objects[storage->size] = new_obj;
        storage->size = storage->size + 1;
    }
}

void remove_linear(storage_linear_t * storage, int pos){
    if (pos >= storage->size){
        return;
    }

    storage->size = storage->size - 1;
    int i;
    for (i = pos; i < storage->size; i++){
        storage->objects[i] = storage->objects[i + 1];
    }
}
//
//void update2(storage_t * storage, int pos, object_t * new_obj){
//    if (pos >= storage->size){
//        return;
//    }
//    storage->objects[pos] = *new_obj;
//}

void push(storage_t * storage, object_t * obj){
    storage->push(storage, obj);
}

void get(storage_t * storage, object_t * obj){
    storage->get(storage, obj);
}

void update(storage_t * storage, object_t * obj){
    storage->push(storage, obj);
}

void remove2(storage_t * storage, object_t * obj){
    storage->remove(storage, obj);
}

int size(storage_t * storage){
    return storage->size(storage);
}

void list(storage_t * storage, object_t ** objects){
    storage->list(storage, objects);
}

void print_storage(storage_t * storage){
    printf("------------------\n");
    printf("Storage size: %d\n", size(storage));
    printf("Storage entries:\n");
    storage->print(storage);
    printf("------------------\n");
}