//
// Created by pavel on 04.05.2020.
//

#include "src/util/hash.h"

int hash(int x) {
    return x;
}

int A = 1e4 + 31;
int M = 1e9 + 7;

int obj_hash(object_t * object){
    int i;
    long long cur_hash = 0;
    for (i = 0; i < strlen(object->key.val); i++){
        char c = object->key.val[i];
        cur_hash = (cur_hash * A + c) % M;
    }
    return (int)cur_hash;
}