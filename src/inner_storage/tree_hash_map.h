#ifndef REPH_TREE_HASH_MAP_H
#define REPH_TREE_HASH_MAP_H

#include "map_interface.h"

// operations for using a tree_hash_map
void tree_hash_map_init(map_t *map);
void tree_hash_map_free(map_t *map);
void tree_hash_map_print(map_t *map); //auxiliary function

#endif //REPH_TREE_HASH_MAP_H