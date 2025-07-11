#include "kv_mem.h"
#include <stdlib.h>

void* kvs_malloc(size_t size) {
    return malloc(size);
}

void* kvs_calloc(size_t nmemb, size_t size) {
    return calloc(nmemb, size);
}

void kvs_free(void* ptr) {
    free(ptr);
}

void* kvs_realloc(void* ptr, size_t size) {
    return realloc(ptr, size);
}

