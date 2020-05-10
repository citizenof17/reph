//
// Created by pavel on 04.05.2020.
//

#include "src/util/device.h"

void init_device(device_t * device, int capacity, int port, char *addr){
    device->capacity = capacity;
    device->location.port = port;
    device->state = UNKNOWN;
    strcpy(device->location.addr, addr);
}

device_t * make_device(int capacity, int port, char *addr){
    device_t * device = (device_t *)malloc(sizeof(device_t));
    init_device(device, capacity, port, addr);
    return device;
}
