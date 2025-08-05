#ifndef __KV_STORE_ARRAY_H__
#define __KV_STORE_ARRAY_H__

#ifdef __cplusplus
extern "C" {
#endif
#include "kv_mem.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct kvs_array_item_s {
	char *key;
	char *value;
} kvs_array_item_t;

#define KVS_ARRAY_SIZE		1024 * 1024

typedef struct kvs_array_s {
	kvs_array_item_t *table;
	int idx;
	int total;
} kvs_array_t;

extern kvs_array_t global_array;

int 
kvs_array_create(kvs_array_t *inst);

int 
kvs_array_destroy(kvs_array_t *inst);

int 
kvs_array_set(kvs_array_t *inst, char *key, char *value);

char* 
kvs_array_get(kvs_array_t *inst, char *key);

int 
kvs_array_delete(kvs_array_t *inst, char *key);

int 
kvs_array_modify(kvs_array_t *inst, char *key, char *value);

int 
kvs_array_exist(kvs_array_t *inst, char *key);

int
kvs_array_count(kvs_array_t *inst);

int
kvs_array_range(kvs_array_t *inst, const char* start_key, const char* end_key, 
               kvs_array_item_t **results, int* count);

#ifdef __cplusplus
}
#endif
#endif
