//
// Created by pavel on 07.05.2020.
//

#include <time.h>
#include <stdlib.h>
#include "general.h"

void gen_random_string(char * dest, int size){
    srand(time(NULL));

    for(int i = 0; i < size; i++){
        dest[i] = (char)(rand() % 26 + 'a');
    }
    dest[size] = '\0';
}
