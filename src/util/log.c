//
// Created by pavel on 04.05.2020.
//

#include "src/util/log.h"

void LOG(char* buf) {
    fprintf(stderr, "%s\n", buf);
}