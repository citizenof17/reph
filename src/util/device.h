//
// Created by pavel on 04.05.2020.
//

#ifndef REPH_DEVICE_H
#define REPH_DEVICE_H

#include <stdlib.h>
#include <string.h>
#include "netwrk.h"

#define ADDR_SIZE (16)

typedef enum state_e {
    UNKNOWN  = 0,
    UP       = 1,
    DOWN     = 2
} state_e;


// This should be a separate server that stores data (in some simple fashion, e.g. hash map)
typedef struct device_t {
    state_e state;
    int capacity;
    addr_port_t location;
} device_t;

void init_device(device_t * device, int capacity, int port, char *addr);
device_t * make_device(int capacity, int port, char *addr);


#endif //REPH_DEVICE_H
