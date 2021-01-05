//
// Created by pavel on 07.06.2020.
//

#ifndef REPH_OBJECT_H
#define REPH_OBJECT_H

#define KEY_SIZE (15)
#define VALUE_SIZE (100)


typedef struct object_key_t {
    char val[KEY_SIZE];
} object_key_t;

typedef struct object_value_t {
    char val[VALUE_SIZE];
} object_value_t;

typedef struct object_t {
    object_key_t key;
    object_value_t value;
    int version;
    char primary;
} object_t;

typedef struct storage_t {
    object_t objects[100];
    int size;
} storage_t;


#endif //REPH_OBJECT_H
