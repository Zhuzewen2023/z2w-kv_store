#ifndef __KV_STORE_SKIPTABLE_H__
#define __KV_STORE_SKIPTABLE_H__

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_LEVEL   32
#define P           0.5 /*节点提升概率*/

typedef struct Node 
{
    char key[256];
    char value[256];
    struct Node** forward; /*前向指针数组（每层一个指针）*/
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
kvs_skiptable_set(kvs_skiptable_t *table, char *key, char *value);

char* 
kvs_skiptable_get(kvs_skiptable_t *table, char *key);

int 
kvs_skiptable_modify(kvs_skiptable_t *table, char *key, char *value);

int 
kvs_skiptable_count(kvs_skiptable_t *table);

int 
kvs_skiptable_delete(kvs_skiptable_t *table, char *key);

int 
kvs_skiptable_exist(kvs_skiptable_t *table, char *key);

#ifdef __cplusplus
}
#endif

#endif
