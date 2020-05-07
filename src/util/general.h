//
// Created by pavel on 06.05.2020.
//

#ifndef REPH_GENERAL_H
#define REPH_GENERAL_H

#define RETURN_ON_FAILURE(i) if ((i) != (EXIT_SUCCESS)) { return (i); }
#define VOID_RETURN_ON_FAILURE(i) if ((i) != (EXIT_SUCCESS)) { return (void *)(size_t)(i); }

void gen_random_string(char * dest, int size);

#endif //REPH_GENERAL_H
