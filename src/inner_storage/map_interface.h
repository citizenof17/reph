#ifndef REPH_MAP_INTERFACE_H
#define REPH_MAP_INTERFACE_H

#include "object.h"

// map_t is a class that provides 3 operations
// but implementations are hidden in impl
typedef struct map_t {
    void (*push) (struct map_t *, object_t *);
    void (*get) (struct map_t *, object_t *);
    void (*remove) (struct map_t *, object_t *);
    void (*print) (struct map_t *);
    int (*size) (struct map_t *);
    void *impl;
} map_t;

typedef map_t storage_t;

#endif //REPH_MAP_INTERFACE_H