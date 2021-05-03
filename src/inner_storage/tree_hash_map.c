
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "tree_hash_map.h"
#include "rb_tree.h"

#define tree_t Tree
#define MIN_SIZE (5)

#define PREPARE_IMPL(map) \
    assert(map); assert(map->impl); \
    tree_hash_impl_t* impl = (tree_hash_impl_t *)map->impl;

typedef struct {
    int size;
    tree_t *tree;
    pthread_rwlock_t *rwlock;
} tree_hash_impl_t;

static void push(map_t * map, object_t * object){
    PREPARE_IMPL(map)

//    pthread_rwlock_wrlock(&impl->rwlock[index]);
    insert(impl->tree, object);
//    pthread_rwlock_unlock(&impl->rwlock[index]);
}

// get an operation and head it to an appropriate tree
static void get(map_t * map, object_t * object){
    PREPARE_IMPL(map)

//    pthread_rwlock_rdlock(&impl->rwlock[index]);
    getValue(impl->tree, object);
//    pthread_rwlock_unlock(&impl->rwlock[index]);
}

static void rem(map_t * map, object_t * object){
    PREPARE_IMPL(map)
//    pthread_rwlock_wrlock(&impl->rwlock[index]);
    erase(impl->tree, object);
//    pthread_rwlock_unlock(&impl->rwlock[index]);
}

static void list(map_t * map, object_t ** objects){
    PREPARE_IMPL(map)
    listTree(impl->tree, objects);
}

static int get_size(map_t * map){
    PREPARE_IMPL(map)

    return impl->tree->size;
}

void tree_hash_map_init(map_t * map){
    assert(map);

    map->impl = malloc(sizeof(tree_hash_impl_t));
    tree_hash_impl_t * impl = (tree_hash_impl_t *)map->impl;

    impl->tree = createTree();
    impl->rwlock = malloc(sizeof(pthread_rwlock_t));
    pthread_rwlock_init(impl->rwlock, NULL);

    map->push = &push;
    map->get = &get;
    map->remove = &rem;
    map->list = &list;
    map->print = &tree_hash_map_print;
    map->size = &get_size;
}

// freeing a tree_hash_map
void tree_hash_map_free(map_t *map){
    PREPARE_IMPL(map);

    deleteTree(impl->tree);
    free(impl->tree);
    free(impl->rwlock);
    free(map->impl);
}

// debugging printing
void tree_hash_map_print(map_t *map){
    PREPARE_IMPL(map);
//    printTree(impl->tree->root, 3);

    int sz = get_size(map);
    object_t ** objects = malloc(sizeof(object_t *) * sz);

    list(map, objects);

    int i;
    for (i = 0; i < sz; i++){
        printf("%s, %s, version: %d, primary: %d\n",
               objects[i]->key.val, objects[i]->value.val,
               objects[i]->version, objects[i]->primary);
    }
    free(objects);
}