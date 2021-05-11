//
// Created by pavel on 04.05.2020.
//

#include <assert.h>
#include "src/util/cluster_map.h"

map_entry_t gen_dummy_device_map_entry(){
    map_entry_t map_entry = {
        .inner_size = 0,
        .class = DEVICE,
        .type = BUCKET_UNIFORM,
        .capacity = 100,
        .port = 4242,
        .addr = "127.0.0.1",
    };
    return map_entry;
}

map_entry_t gen_dummy_map(){
    map_entry_t map_entry;
    map_entry.class = RACK;
    map_entry.type = BUCKET_UNIFORM;

    const int INNER_SIZE = 5;
    map_entry.inner_size = INNER_SIZE;
    map_entry.inner = (map_entry_t *)malloc(sizeof(map_entry_t) * INNER_SIZE);
    for (int i = 0; i < INNER_SIZE; i++){
        map_entry.inner[i] = gen_dummy_device_map_entry();
        map_entry.inner[i].port += i;
    }

    return map_entry;
}

char * parse_json_to_string(FILE * fin){
//    LOG("NOT IMPLEMENTED");
    char * plane_map = (char *)malloc(sizeof(char) * DEFAULT_MAP_SIZE);
    return plane_map;
}

map_entry_t parse_string_to_map_entry(char *map_repr){
    LOG("NOT IMPLEMENTED");
    map_entry_t map_entry;
    map_entry.port = 0;
    return map_entry;
}

void build_map_bucket(bucket_t * bucket, map_entry_t map_entry){
    init_bucket(bucket, map_entry.class, map_entry.type, map_entry.inner_size);

    if (map_entry.class == DEVICE){
        assert(map_entry.inner_size == 0);
        bucket->device = (device_t *)malloc(sizeof(device_t));
        init_device(bucket->device, map_entry.capacity, map_entry.port, map_entry.addr);
    }
    else{
        bucket->device = NULL;
    }

    for (int i = 0; i < map_entry.inner_size; i++) {
        build_map_bucket(bucket->inner_buckets[i], map_entry.inner[i]);
    }
}

map_entry_t make_map_entry_from_string(char * map_string){
// This function should parse json string to build map_entry_t - which is a raw
// representation of the cluster
//    LOG("NOT IMPLEMENTED");
    return gen_dummy_map();
}

//char * get_string_map_representation(FILE * fin){
//    char * string_map = parse_json_to_string(fin);
//    return string_map;
//}

cluster_map_t * build_map(map_entry_t map_entry) {
    cluster_map_t * cluster_map = (cluster_map_t *)malloc(sizeof(cluster_map_t));
    cluster_map->root = (bucket_t *)malloc(sizeof(bucket_t));
    build_map_bucket(cluster_map->root, map_entry);
    return cluster_map;
}

//cluster_map_t * build_map_from_file(FILE * fin){
//    char * string_map = get_string_map_representation(fin);
//    map_entry_t map_entry = make_map_entry_from_string(string_map);
//    return build_map(map_entry);
//}
//
//cluster_map_t * build_map_from_string(char * plane_map){
//    map_entry_t map_entry = make_map_entry_from_string(plane_map);
//    cluster_map_t * cl = build_map(map_entry);
//    return cl;
//}

cluster_map_t * build_default_map(){
    map_entry_t map_entry = gen_dummy_map();
    cluster_map_t * cl = build_map(map_entry);
    cl->version = 0;
    return cl;
}


int get_map_version(int sock, int * new_map_version){
    message_type_e message = GET_MAP_VERSION;
    int rc;

    rc = ssend(sock, &message, sizeof(message));
    RETURN_ON_FAILURE(rc);
    LOG("Send request for map version");

    rc = srecv(sock, new_map_version, sizeof(*new_map_version));
    RETURN_ON_FAILURE(rc);
    LOG("Received new map version");

    return (EXIT_SUCCESS);
}

int get_cluster_map(int sock, char * plane_cluster_map){
    int rc;
    message_type_e message = GET_MAP;

    rc = ssend(sock, &message, sizeof(message));
    RETURN_ON_FAILURE(rc);
    LOG("Send request for a new map version");

    // TODO: Probably you will read more than expected from that socket (because
    //  DEFAULT_MAP_SIZE is more than actual map size is), be careful here
    rc = srecv(sock, plane_cluster_map, DEFAULT_MAP_SIZE);
    RETURN_ON_FAILURE(rc);
    LOG("Received new plane cluster map representation");

    LOG("Cluster map is updated");
    printf("New cluster map %s\n", plane_cluster_map);

    return (EXIT_SUCCESS);
}

void free_bucket(bucket_t * bucket){
    if (bucket == NULL){
        return;
    }

    int i;
    for (i = 0; i < bucket->size; i++){
        free_bucket(bucket->inner_buckets[i]);
    }
    if (bucket->_impl != NULL) {
        free(bucket->inner_buckets);
        free(bucket->_impl);
    }
    if (bucket->class == DEVICE) {
        free(bucket->device);
    }
//    free(bucket);
}

void free_map(cluster_map_t ** cluster_map){
    // TODO: Fix freeing here. Something is wrong?
    free_bucket((*cluster_map)->root);
    free(*cluster_map);
    *cluster_map = NULL;
}

int update_map_if_needed(net_config_t config, cluster_map_t ** cluster_map){
    int sock, rc;
    rc = connect_to_peer(&sock, config.monitor);
    RETURN_ON_FAILURE(rc);

    LOG("Client connected to monitor");

    int new_map_version;
    rc = get_map_version(sock, &new_map_version);
    RETURN_ON_FAILURE(rc);

    if (new_map_version > (*cluster_map)->version || (*cluster_map)->version <= 0){
        char plane_cluster_map[DEFAULT_MAP_SIZE];
        printf("New cluster map version %d\n", new_map_version);
        LOG("Cluster map version is updated, need to refresh its content");
        rc = get_cluster_map(sock, plane_cluster_map);
        RETURN_ON_FAILURE(rc);

        free_map(cluster_map);
        *cluster_map = cluster_map_from_string(plane_cluster_map);
        (*cluster_map)->version = new_map_version;
        print_cluster_map(*cluster_map);
    }

    send_bye(sock);
    close(sock);

    return (EXIT_SUCCESS);
}

cluster_map_t * init_empty_cluster_map(){
    cluster_map_t * cluster_map = (cluster_map_t *)malloc(sizeof(cluster_map_t));
    cluster_map->version = -1;
    cluster_map->root = NULL;
    return cluster_map;
}


void inc(int * pos, int val){
    *pos = *pos + val;
}

void add_space(char * bucket, int * pos){
    bucket[*pos] = ' ';
    inc(pos, 1);
}

void open_bucket(char * bucket, int * pos){
    bucket[*pos] = '{';
    inc(pos, 1);
    add_space(bucket, pos);
}

void close_bucket(char * bucket, int * pos){
    bucket[*pos] = '}';
    inc(pos, 1);
    add_space(bucket, pos);
}

void add_type(char * bucket, int * pos, bucket_type_e type) {
    char * uniform = "BUCKET_UNIFORM";
//    char * list = "BUCKET_LIST";
//    char * tree = "BUCKET_TREE";

    switch (type){
        case BUCKET_UNIFORM:
            strcpy(&bucket[*pos], uniform);
            inc(pos, strlen(uniform));
            break;
        default:
            LOG("Other types not supported");
    }
    add_space(bucket, pos);
}

void add_class(char * bucket, int * pos, bucket_class_e class) {
    char * device = "DEVICE";
    char * rack = "RACK";
    // add other classes

    switch (class){
        case DEVICE:
            strcpy(&bucket[*pos], device);
            inc(pos, strlen(device));
            break;
        case RACK:
            strcpy(&bucket[*pos], rack);
            inc(pos, strlen(rack));
            break;
        default:
            LOG("Other types not supported");
    }
    add_space(bucket, pos);
}

void add_int(char * bucket, int * pos, int val) {
    char buf[7];
    sprintf(buf, "%d", val);

    strcpy(&bucket[*pos], buf);
    inc(pos, strlen(buf));
    add_space(bucket, pos);
}

void open_device(char * bucket, int * pos){
    bucket[*pos] = '[';
    inc(pos, 1);
    add_space(bucket, pos);
}

void close_device(char * bucket, int * pos){
    bucket[*pos] = ']';
    inc(pos, 1);
    add_space(bucket, pos);
}

void add_device_state(char * bucket, int * pos, state_e state) {
    char * up = "UP";
    char * down = "DOWN";
    char * unknown = "UNKNOWN";

    switch (state){
        case UP:
            strcpy(&bucket[*pos], up);
            inc(pos, strlen(up));
            break;
        case DOWN:
            strcpy(&bucket[*pos], down);
            inc(pos, strlen(down));
            break;
        case UNKNOWN:
            strcpy(&bucket[*pos], unknown);
            inc(pos, strlen(unknown));
            break;
        default:
            LOG("Other states not supported");
    }
    add_space(bucket, pos);
}

void add_device_addr(char * bucket, int * pos, char * addr) {
    strcpy(&bucket[*pos], addr);
    inc(pos, strlen(addr));
    add_space(bucket, pos);
}

void add_device(char * bucket, int * pos, device_t * device) {
    if (device == NULL){
        return;
    }

    open_device(bucket, pos);

    add_device_state(bucket, pos, device->state);
    add_int(bucket, pos, device->capacity);
    add_device_addr(bucket, pos, device->location.addr);
    add_int(bucket, pos, device->location.port);

    close_device(bucket, pos);
}

void add_bucket(char * bucket, int * pos, bucket_t * inner_bucket){
    char * inner_bucket_str = bucket_to_string(inner_bucket);
    strcpy(&bucket[*pos], inner_bucket_str);
    inc(pos, strlen(inner_bucket_str));
    free(inner_bucket_str);
}

char * bucket_to_string(bucket_t * bucket){
    char * bucket_str = malloc(sizeof(char) * DEFAULT_MAP_SIZE);
    memset(bucket_str, 0, DEFAULT_MAP_SIZE);

    int pos = 0;
    open_bucket(bucket_str, &pos);

    add_type(bucket_str, &pos, bucket->type);
    add_class(bucket_str, &pos, bucket->class);
    add_int(bucket_str, &pos, bucket->size);

    add_device(bucket_str, &pos, bucket->device);

    int i;
    for (i = 0; i < bucket->size; i++){
        add_bucket(bucket_str, &pos, bucket->inner_buckets[i]);
    }

    close_bucket(bucket_str, &pos);
    return bucket_str;
}

char * cluster_map_to_string(cluster_map_t * cluster_map){
    return bucket_to_string(cluster_map->root);
}

int find_next_space(const char * str, int pos){
    int i = pos;
    if (str[i] == ' '){
        i++;
    }

    while (str[i] != ' ' && str[i] != '\0'){
        i++;
    }
    return i;
}

void get_token(char * bucket_str, int * pos, char * buf){
    int space_pos = find_next_space(bucket_str, *pos);
    int str_len = space_pos - *pos;
    strncpy(buf, &bucket_str[*pos], str_len);
    buf[str_len] = '\0';
    inc(pos, str_len + 1);
}

bucket_type_e read_type(char * bucket_str, int * pos){
    char buf[20];
    get_token(bucket_str, pos, buf);

    if (strcmp(buf, "BUCKET_UNIFORM") == 0){
        return BUCKET_UNIFORM;
    }
    LOG("Invalid bucket type found");
    printf("Found type: %s\n", buf);
    return BUCKET_UNIFORM;
}

bucket_class_e read_class(char * bucket_str, int * pos){
    char buf[20];
    get_token(bucket_str, pos, buf);

    if (strcmp(buf, "RACK") == 0){
        return RACK;
    }
    else if (strcmp(buf, "DEVICE") == 0){
        return DEVICE;
    }
    LOG("Invalid bucket class found");
    printf("Found class: %s\n", buf);
    return DEVICE;
}

int read_int(char * bucket_str, int * pos){
    char buf[20];
    memset(buf, 0, sizeof(buf));
    get_token(bucket_str, pos, buf);

    int res = atoi(buf);
    return res;
}

state_e read_device_state(char * bucket_str, int * pos){
    char buf[20];
    get_token(bucket_str, pos, buf);

    if (strcmp(buf, "UP") == 0){
        return UP;
    }
    else if (strcmp(buf, "DOWN") == 0){
        return DOWN;
    }
    else if (strcmp(buf, "UNKNOWN") == 0) {
        return UNKNOWN;
    }

    LOG("Invalid bucket class found");
    printf("Found class: %s\n", buf);
    return UNKNOWN;
}

void read_chars(char * bucket_str, int * pos, char * buf){
    get_token(bucket_str, pos, buf);
}

void read_device(char * bucket_str, int * pos, device_t * device){
    // initially pos should always be on '[' - the opening bracket
    inc(pos, 2);

    device->state = read_device_state(bucket_str, pos);
    device->capacity = read_int(bucket_str, pos);
    read_chars(bucket_str, pos, device->location.addr);
    device->location.port = read_int(bucket_str, pos);

    inc(pos, 2);
}

void string_to_bucket(char * bucket_str, int * pos, bucket_t * bucket){
    // Initially, pos should always point at '{'
    inc(pos, 2);  // bucket_str[0:2] == '{ ', skip it

    bucket_type_e type = read_type(bucket_str, pos);
    bucket_class_e class = read_class(bucket_str, pos);
    int size = read_int(bucket_str, pos);

    init_bucket(bucket, class, type, size);

    if (bucket->class == DEVICE){
        bucket->device = malloc(sizeof(device_t));
        read_device(bucket_str, pos, bucket->device);
    }
    else{
        bucket->device = NULL;
    }

    int i;
    for (i = 0; i < size; i++){
        string_to_bucket(bucket_str, pos, bucket->inner_buckets[i]);
        inc(pos, 2);
    }
}

void remove_special_chars(char * str){
    char buf[DEFAULT_MAP_SIZE];

    int pos_res = 0;
    int pos_orig = 0;

    while(str[pos_orig] != '\0'){
        if (str[pos_orig] == '\n'){
            pos_orig++;
            continue;
        }
        buf[pos_res] = str[pos_orig];
        pos_res++;
        pos_orig++;
    }

    buf[pos_res] = '\0';
    strcpy(str, buf);
}

cluster_map_t * cluster_map_from_string(char * str){
    printf("CM %s\n", str);
    remove_special_chars(str);
    printf("CM %s\n", str);

    cluster_map_t * cluster_map = malloc(sizeof(cluster_map_t));
    cluster_map->root = malloc(sizeof(bucket_t));
    int pos = 0;
    string_to_bucket(str, &pos, cluster_map->root);
    cluster_map->version = 0;

    return cluster_map;
}

cluster_map_t * cluster_map_from_file(char * filepath){
    FILE *fp;
    fp = fopen(filepath, "r");

    char buf[DEFAULT_MAP_SIZE];
    fgets(buf, DEFAULT_MAP_SIZE, fp);
    fclose(fp);
    return cluster_map_from_string(buf);
}

void print_cluster_map(cluster_map_t * cluster_map){
    char * cl_str = cluster_map_to_string(cluster_map);
    printf("CLUSTER MAP: %s\n", cl_str);
    printf("CLUSTER MAP VERSION: %d\n", cluster_map->version);
    free(cl_str);
}