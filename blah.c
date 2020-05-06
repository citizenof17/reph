//
// Created by pavel on 05.05.2020.
//

/*
 * THIS FILE IS USED FOR QUICK TESTING
 * SHOULD NOT BE PART OF THE PROJECT
 */

#include <stdio.h>
#include <stdlib.h>

typedef struct blah_t {
    int a;
    int b;
    int c;
} blah_t;

blah_t build_blah(){
    blah_t blah = {
            .a = 15,
            .b = 16,
            .c = 20
    };
    return blah;
}

int main(){

    blah_t blah = build_blah();

    printf("%d %d %d\n", blah.a, blah.b, blah.c);

    return 0;
}
