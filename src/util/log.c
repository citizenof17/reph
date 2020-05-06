//
// Created by pavel on 04.05.2020.
//

#include "src/util/log.h"

void LOG(char* buf) {
    char timestamp[100];
    time_t t = time(NULL);
    struct tm * p = localtime(&t);

    strftime(timestamp, 100, "%c", p);
    fprintf(stderr, "%s: %s\n", timestamp, buf);
}