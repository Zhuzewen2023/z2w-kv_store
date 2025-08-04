#ifndef __KV_STORE_HASH_H__
#define __KV_STORE_HASH_H__

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_KEY_LEN     128
#define MAX_VALUE_LEN   512
#define MAX_TABLE_SIZE  1024

#define ENABLE_KEY_POINTER 1

typedef struct hashnode {
#if ENABLE_KEY_POINTER
    char *key;
    char *value;
#else
    char key[MAX_KEY_LEN];
    char value[MAX_VALUE_LEN];
#endif
    struct hashnode *next;
} hashnode_t;

typedef struct hashtable {
    hashnode_t **table;
    int max_slots;
    int count;
} hashtable_t;

typedef hashtable_t kvs_hash_t;

extern kvs_hash_t global_hash;

int 
kvs_hash_create(kvs_hash_t *hash);

void 
kvs_hash_destroy(kvs_hash_t *hash);

int 
kvs_hash_set(kvs_hash_t *hash, char *key, char *value);

char* 
kvs_hash_get(kvs_hash_t *hash, char *key);

int 
kvs_hash_modify(kvs_hash_t *hash, char *key, char *value);

int 
kvs_hash_count(kvs_hash_t *hash);

int 
kvs_hash_delete(kvs_hash_t *hash, char *key);

int 
kvs_hash_exist(kvs_hash_t *hash, char *key);

#ifdef __cplusplus
}
#endif

#endif
