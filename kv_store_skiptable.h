#ifndef __KV_STORE_SKIPTABLE_H__
#define __KV_STORE_SKIPTABLE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "kv_range.h"

#include <stdlib.h>

#define MAX_LEVEL   32
#define P           0.5 /*节点提升概率*/

/*
节点内存布局：
+-------------------+
| key  (固定长度)    |
+-------------------+
| value(固定长度)    |
+-------------------+
| forward[0]       | --> 层0的下个节点
| forward[1]       | --> 层1的下个节点
| ...              |
| forward[level]   | --> 最高层的下个节点
+-------------------+
*/

typedef struct Node 
{
    char key[256];
    char value[256];
    uint64_t timestamp;
    struct Node** forward; 
} Node;

typedef struct SkipTable
{
    int level; /*跳表最大层数*/
    Node* header; /*头节点*/
    int size; /*节点计数*/
} SkipTable;

typedef SkipTable kvs_skiptable_t;
extern kvs_skiptable_t global_skiptable;

int 
kvs_skiptable_create(kvs_skiptable_t *table);

void 
kvs_skiptable_destroy(kvs_skiptable_t *table);

int 
kvs_skiptable_set(kvs_skiptable_t *table, const char *key, const char *value);

int
kvs_skiptable_set_with_timestamp(kvs_skiptable_t *inst, const char* key, const char* value, uint64_t timestamp);

char* 
kvs_skiptable_get(kvs_skiptable_t *table, const char *key);

int 
kvs_skiptable_modify(kvs_skiptable_t *table, const char *key, const char *value);

int
kvs_skiptable_modify_with_timestamp(kvs_skiptable_t *table, const char *key, const char* value, uint64_t timestamp);

int 
kvs_skiptable_count(kvs_skiptable_t *table);

int 
kvs_skiptable_delete(kvs_skiptable_t *table, char *key);

int 
kvs_skiptable_exist(kvs_skiptable_t *table, char *key);

int
kvs_skiptable_range(kvs_skiptable_t* inst, const char* start_key, const char* end_key,
    kvs_item_t** results, int* count);

int
kvs_skiptable_get_all(kvs_skiptable_t* inst, kvs_item_t** results, int* count);

uint64_t
kvs_skiptable_get_timestamp(kvs_skiptable_t* inst, const char* key);

#ifdef __cplusplus
}
#endif

#endif
