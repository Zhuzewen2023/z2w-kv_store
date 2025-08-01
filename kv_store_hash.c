#include "kv_store_hash.h"
#include "kv_store.hpp"

kvs_hash_t global_hash = {0};

static int
_hash(char *key, int size)
{
    if (!key) return -1;
    int sum = 0;
    int i = 0;
    while (key[i] != '\0') {
        sum += key[i];
        i++;
    }

    return sum % size;
}

static hashnode_t*
_create_node(char* key, char* value)
{
    hashnode_t *node = (hashnode_t *)kvs_malloc(sizeof(hashnode_t));
    if (!node) {
        printf("_create_node failed, malloc failed\n");
        return NULL;
    }
#if ENABLE_KEY_POINTER
    char *kcopy = (char *)kvs_malloc(strlen(key) + 1);
    if (!kcopy) {
        printf("_create_node failed, malloc failed\n");
        kvs_free(node);
        return NULL;
    }
    memset(kcopy, 0, strlen(key) + 1);
    strcpy(kcopy, key);
    node->key = kcopy;
    char *vcopy = (char *)kvs_malloc(strlen(value) + 1);
    if (!vcopy) {
        printf("_create_node failed, malloc failed\n");
        kvs_free(node);
        kvs_free(kcopy);
        return NULL;
    }
    memset(vcopy, 0, strlen(value) + 1);
    strcpy(vcopy, value);
    node->value = vcopy;
#else
    strncpy(node->key, key, MAX_KEY_LEN);
    strncpy(node->value, value, MAX_VALUE_LEN);
#endif
    node->next = NULL;
    return node;

}

int 
kvs_hash_create(kvs_hash_t *hash)
{
    if (!hash) return -1;
    hash->table = (hashnode_t **)kvs_malloc(sizeof(hashnode_t *) * MAX_TABLE_SIZE);
    if (!hash->table) {
        printf("kvs_hash_create failed, malloc failed\n");
        return -1;
    }
    memset(hash->table, 0, sizeof(hashnode_t *) * MAX_TABLE_SIZE);
    hash->max_slots = MAX_TABLE_SIZE;
    hash->count = 0;
    return 0;
}

void 
kvs_hash_destroy(kvs_hash_t *hash)
{
    if (!hash) return;
    int i = 0;
    for (i = 0; i < hash->max_slots; i++) {
        hashnode_t *node = hash->table[i];
        while (node) {
            hashnode_t *tmp = node;
            node = node->next;
            hash->table[i] = node;
        #if ENABLE_KEY_POINTER
            if (tmp->key) {
                kvs_free(tmp->key);
            }
            if (tmp->value) {
                kvs_free(tmp->value);
            }
        #endif
            kvs_free(tmp);
        }
    }
    kvs_free(hash->table);
}

int 
kvs_hash_set(kvs_hash_t *hash, char *key, char *value)
{
    if (!hash || !key || !value) {
        printf("kvs_hash_set failed, invalid param\n");
        return -1;
    }
    int slot = _hash(key, hash->max_slots);
    hashnode_t *node = hash->table[slot];
    while (node) {
        if (strcmp(node->key, key) == 0) {
            //printf("kvs_hash_set failed, key already exist\n");
            return 1;
        }
        node = node->next;
    }
    hashnode_t *new_node = _create_node(key, value);
    if (!new_node) {
        printf("kvs_hash_set failed, _create_node failed\n");
        return -3;
    }
    new_node->next = hash->table[slot];
    hash->table[slot] = new_node;
    hash->count++;
    return 0;
}

char* 
kvs_hash_get(kvs_hash_t *hash, char *key)
{
    if (!hash || !key) {
        printf("kvs_hash_get failed, invalid param\n");
        return NULL;
    }

    int slot = _hash(key, hash->max_slots);
    hashnode_t *node = hash->table[slot];
    while (node) {
        if (strcmp(node->key, key) == 0) {
            return node->value;
        }
        node = node->next;
    }
    return NULL;
}

int 
kvs_hash_modify(kvs_hash_t *hash, char *key, char *value)
{
    if (!hash || !key || !value) {
        printf("kvs_hash_mod failed, invalid param\n");
        return -1;
    }

    int slot = _hash(key, hash->max_slots);
    hashnode_t *node = hash->table[slot];
    while (node) {
        if (strcmp(node->key, key) == 0) {
            break;
        }
        node = node->next;
    }
    if (!node) {
        //printf("kvs_hash_mod failed, key not exist\n");
        return -2;
    }
#if ENABLE_KEY_POINTER
    kvs_free(node->value);
    char *vcopy = (char *)kvs_malloc(strlen(value) + 1);
    if (!vcopy) {
        printf("kvs_hash_mod failed, malloc failed\n");
        return -3;
    }
    memset(vcopy, 0, strlen(value) + 1);
    strcpy(vcopy, value);
    node->value = vcopy;
#else
    memset(node->value, 0, MAX_VALUE_LEN);
    strncpy(node->value, value, MAX_VALUE_LEN);
#endif
    return 0;
    
}

int 
kvs_hash_count(kvs_hash_t *hash)
{
    return hash->count;
}

int 
kvs_hash_delete(kvs_hash_t *hash, char *key)
{
    if (!hash || !key) {
        printf("kvs_hash_delete failed, invalid param\n");
        return -1;
    }

    int slot = _hash(key, hash->max_slots);
    hashnode_t *node = hash->table[slot];
    if (node == NULL) {
        printf("kvs_hash_delete failed, key not exist\n");
        return -2;
    }

    if (strcmp(node->key, key) == 0) {
        hash->table[slot] = node->next;
    #if ENABLE_KEY_POINTER
        if (node->key) {
            kvs_free(node->key);
        }
        if (node->value) {
            kvs_free(node->value);
        }
    #endif
        kvs_free(node);
        hash->count--;
        return 0;
    }
    while (node->next) {
        if (strcmp(node->next->key, key) == 0) {
            hashnode_t *tmp = node->next;
            node->next = node->next->next;
        #if ENABLE_KEY_POINTER
            if (tmp->key) {
                kvs_free(tmp->key);
            }
            if (tmp->value) {
                kvs_free(tmp->value);
            }
        #endif
            kvs_free(tmp);
            hash->count--;
            break;
        }
    }
    return 0;
}

int 
kvs_hash_exist(kvs_hash_t *hash, char *key)
{
    char *value = kvs_hash_get(hash, key);
    if (!value) {
        return -1;
    }
    return 0;
}