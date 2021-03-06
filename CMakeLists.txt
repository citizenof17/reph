cmake_minimum_required(VERSION 3.16)
project(reph C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")

set(THREADS_PREFER_PTHREAD_FLAG ON)

set(BASEPATH "${CMAKE_SOURCE_DIR}")
include_directories("${BASEPATH}")

set(UTIL_SOURCES
        src/util/bucket.c
        src/util/cluster_map.c
        src/util/device.c
        src/util/general.c
        src/util/hash.c
        src/util/log.c
        src/util/mysleep.c
        src/util/netwrk.c
        src/crush.c
        src/shortcuts.h)

set(INNER_STORAGE_SOURCES
        src/inner_storage/rb_tree.c
        src/inner_storage/tree_hash_map.c)

#set(TEST_SOURCES
#        test_all.c
#        ${UTIL_SOURCES}
#        )
#
#set(MAIN_SOURCES
#        main.c
#        ${UTIL_SOURCES}
#)

set(CLIENT_SOURCES
        src/client/client.c
        ${UTIL_SOURCES}
        src/shortcuts.h)

set(MONITOR_SOURCES
        src/monitor/monitor.c
        ${UTIL_SOURCES}
        src/shortcuts.h)

set(OSD_SOURCES
        src/storage/osd.c
        ${UTIL_SOURCES}
        ${INNER_STORAGE_SOURCES}
        src/shortcuts.h)


#add_executable(reph ${MAIN_SOURCES})
add_executable(client ${CLIENT_SOURCES})
add_executable(monitor ${MONITOR_SOURCES})
add_executable(osd ${OSD_SOURCES})
#add_executable(test ${TEST_SOURCES})

#add_executable(blah blah.c)

target_link_libraries(monitor pthread)