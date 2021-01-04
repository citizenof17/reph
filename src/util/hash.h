//
// Created by pavel on 04.05.2020.
//

#ifndef REPH_HASH_H
#define REPH_HASH_H

#include <string.h>
#include "src/util/object.h"

int hash(int x);
int obj_hash(object_t * object);

#endif //REPH_HASH_H
