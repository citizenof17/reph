cmake_minimum_required(VERSION 3.16)
project(reph C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} -pthread)

set(THREADS_PREFER_PTHREAD_FLAG ON)

set(BASEPATH "${CMAKE_SOURCE_DIR}")
include_directories("${BASEPATH}")

set(UTIL_SOURCES
        src/util/bucket.c
        src/util/cluster_map.c
        src/util/log.c
        src/util/hash.c
        src/util/device.c
        src/util/mysleep.c
)

set(TEST_SOURCES
        test_all.c
        ${UTIL_SOURCES}
        )

set(MAIN_SOURCES
        main.c
        ${UTIL_SOURCES}
)

set(CLIENT_SOURCES
        src/client/client.c
        ${UTIL_SOURCES}
        )

set(MONITOR_SOURCES
        src/monitor/monitor.c
        ${UTIL_SOURCES}
        )


add_executable(reph ${MAIN_SOURCES})
add_executable(client ${CLIENT_SOURCES})
add_executable(monitor ${MONITOR_SOURCES})
add_executable(test ${TEST_SOURCES})

target_link_libraries(monitor pthread)