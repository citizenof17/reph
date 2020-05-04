cmake_minimum_required(VERSION 3.16)
project(reph C)

set(CMAKE_C_STANDARD 11)

set(BASEPATH "${CMAKE_SOURCE_DIR}")
include_directories("${BASEPATH}")

set(SOURCE_FILES
        main.c
        src/util/bucket.c
        src/util/cluster_map.c
        src/util/log.c
        src/util/hash.c
        src/util/device.h src/util/device.c)

add_executable(reph ${SOURCE_FILES})