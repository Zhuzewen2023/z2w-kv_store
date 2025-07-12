#ifndef __KV_STORE_ARRAY_H__
#define __KV_STORE_ARRAY_H__

#ifdef __cplusplus
extern "C" {
#endif
#include "kv_mem.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define KVS_ARRAY_SIZE   1024

typedef struct kvs_array_item_s {
    char *key;
    char *value;
} kvs_array_item_t;



int kvs_array_set(char *key, char *value);
char* kvs_array_get(char *key);
int kvs_array_delete(char *key);
int kvs_array_modify(char *key, char *value);

#ifdef __cplusplus
}
#endif
#endif
