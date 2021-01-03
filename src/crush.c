//
// Created by pavel on 07.05.2020.
//

#include "crush.h"

crush_result_t * init_crush_input(int num, ...){
    crush_result_t * input = (crush_result_t *)malloc(sizeof(crush_result_t));
    input->buckets = (bucket_t **)malloc(sizeof(bucket_t *) * num);
    input->size = num;

    va_list valist;
    va_start(valist, num);

    int i;
    for (i = 0; i < num; i++){
        input->buckets[i] = va_arg(valist, bucket_t *);
    }
    va_end(valist);

    return input;
}

char failed(bucket_t * output_bucket){
    device_t * device = output_bucket->type == DEVICE ? output_bucket->device : NULL;
    if (device != NULL){
        return device->state == UP;
    }
    return 0;
}

char overload(bucket_t * output_bucket, int x){
    return 0;
}

char in_(bucket_t * output_bucket, crush_result_t * output, int last_pos){
    int i;
    for (i = 0; i < last_pos; i++){
        if (output_bucket == output->buckets[i]){
            return 1;
        }
    }
    return 0;
}

// input - where to select new items
// t - type of elements to be selected
// n - number of elements to select
crush_result_t * crush_select(crush_result_t * input, bucket_class_e t, int n){
    crush_result_t * output = (crush_result_t *)malloc(sizeof(crush_result_t));
    output->buckets = (bucket_t **)malloc(sizeof(bucket_t *) * n);

    bucket_t * selected_buckets;

    int i;
    int last_pos = 0;
    for (i = 0; i < input->size; i++){
        int failures = 0;

        int r;
        for (r = 0; r < n; r++){
            int replica_failures = 0;

            bucket_t * output_bucket = NULL;

            char retry_descent = 0;
            do {
                bucket_t * bucket = input->buckets[i];
                char retry_bucket = 0;

                do {
                    int rd = 0;
                    if ("first n") {  // TODO: Check what is this?
                        rd = r + failures;
                    } else {
                        rd = r + replica_failures * n;
                    }

                    output_bucket = bucket->c(rd, x);

                    char in_output = in_(output_bucket, output, last_pos);
                    if (output_bucket->class != t){
                        bucket = output_bucket;
                        retry_bucket = 1;
                    }
                    else if (in_output || failed(output_bucket) || overload(output_bucket, x)){
                        replica_failures++;
                        failures++;

                        if (in_output && replica_failures < 3){
                            retry_bucket = 1;
                        }
                        else{
                            retry_descent = 1;
                        }
                    }
                } while(!retry_bucket);
            } while(!retry_descent);

            if (output_bucket != NULL){
                output->buckets[last_pos] = output_bucket;
                last_pos++;
            }
        }
    }

    return output;
}
