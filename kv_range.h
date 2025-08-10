#ifndef __KV_RANGE_H__
#define __KV_RANGE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <stdint.h>

typedef struct kvs_item {
	char* key;
	char* value;
    uint64_t timestamp;
} kvs_item_t;

static int compare_kvs_items(const void* a, const void* b) {
    const kvs_item_t* item_a = (const kvs_item_t*)a;
    const kvs_item_t* item_b = (const kvs_item_t*)b;
    return strcmp(item_a->key, item_b->key);
}

#ifdef __cplusplus
}
#endif

#endif

