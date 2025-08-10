#include "kv_store_hash.h"
#include "kv_store.hpp"

#include <stdlib.h>

kvs_hash_t global_hash = {0};

static int
_hash(const char *key, int size)
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
_create_node(const char* key, const char* value)
{
    hashnode_t *node = (hashnode_t *)kvs_malloc(sizeof(hashnode_t));
    if (!node) {
        KV_LOG("_create_node failed, malloc failed\n");
        return NULL;
    }
#if ENABLE_KEY_POINTER
    char *kcopy = (char *)kvs_malloc(strlen(key) + 1);
    if (!kcopy) {
        KV_LOG("_create_node failed, malloc failed\n");
        kvs_free(node);
        return NULL;
    }
    memset(kcopy, 0, strlen(key) + 1);
    strcpy(kcopy, key);
    node->key = kcopy;
    char *vcopy = (char *)kvs_malloc(strlen(value) + 1);
    if (!vcopy) {
        KV_LOG("_create_node failed, malloc failed\n");
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

static hashnode_t*
_create_node_with_timestamp(const char* key, const char* value, uint64_t timestamp)
{
    hashnode_t *node = _create_node(key, value);
    if (!node) {
        KV_LOG("_create_node failed, malloc failed\n");
        return NULL;
    }
    node->timestamp = timestamp;
    return node;
}

int 
kvs_hash_create(kvs_hash_t *hash)
{
    if (!hash) return -1;
    hash->table = (hashnode_t **)kvs_malloc(sizeof(hashnode_t *) * MAX_TABLE_SIZE);
    if (!hash->table) {
        KV_LOG("kvs_hash_create failed, malloc failed\n");
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
kvs_hash_set(kvs_hash_t *inst, const char *key, const char *value)
{
    uint64_t timestamp = 0;
    timestamp = get_current_timestamp_ms();
    KV_LOG("kvs_array_set timestamp: %lu\n", timestamp);
    return kvs_hash_set_with_timestamp(inst, key, value, timestamp);
    #if 0
    if (!hash || !key || !value) {
        KV_LOG("kvs_hash_set failed, invalid param\n");
        return -1;
    }
    int slot = _hash(key, hash->max_slots);
    hashnode_t *node = hash->table[slot];
    while (node) {
        if (strcmp(node->key, key) == 0) {
            //KV_LOG("kvs_hash_set failed, key already exist\n");
            return 1;
        }
        node = node->next;
    }
    hashnode_t *new_node = _create_node(key, value);
    if (!new_node) {
        KV_LOG("kvs_hash_set failed, _create_node failed\n");
        return -3;
    }
    new_node->next = hash->table[slot];
    hash->table[slot] = new_node;
    hash->count++;
    return 0;
    #endif
}

int
kvs_hash_set_with_timestamp(kvs_hash_t *hash, const char *key, const char *value, uint64_t timestamp)
{
    if (!hash || !key || !value) {
        KV_LOG("kvs_hash_set failed, invalid param\n");
        return -1;
    }
    int slot = _hash(key, hash->max_slots);
    hashnode_t *node = hash->table[slot];
    while (node) {
        if (strcmp(node->key, key) == 0) {
            //KV_LOG("kvs_hash_set failed, key already exist\n");
            return 1;
        }
        node = node->next;
    }
    hashnode_t *new_node = _create_node_with_timestamp(key, value, timestamp);
    if (!new_node) {
        KV_LOG("kvs_hash_set failed, _create_node failed\n");
        return -3;
    }
    new_node->next = hash->table[slot];
    hash->table[slot] = new_node;
    hash->count++;
    return 0;
}

char* 
kvs_hash_get(kvs_hash_t *hash, const char *key)
{
    if (!hash || !key) {
        KV_LOG("kvs_hash_get failed, invalid param\n");
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
kvs_hash_modify(kvs_hash_t *inst, const char *key, const char *value)
{
    uint64_t timestamp = 0;
    timestamp = get_current_timestamp_ms();
    KV_LOG("kvs_hash_modify timestamp: %lu\n", timestamp);
    return kvs_hash_modify_with_timestamp(inst, key, value, timestamp);
    #if 0
    if (!hash || !key || !value) {
        KV_LOG("kvs_hash_mod failed, invalid param\n");
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
        //KV_LOG("kvs_hash_mod failed, key not exist\n");
        return -2;
    }
#if ENABLE_KEY_POINTER
    kvs_free(node->value);
    char *vcopy = (char *)kvs_malloc(strlen(value) + 1);
    if (!vcopy) {
        KV_LOG("kvs_hash_mod failed, malloc failed\n");
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
    #endif
}

int
kvs_hash_modify_with_timestamp(kvs_hash_t *inst, const char* key, const char* value, uint64_t timestamp)
{
    if (!inst || !key || !value) {
        KV_LOG("kvs_hash_mod failed, invalid param\n");
        return -1;
    }

    int slot = _hash(key, inst->max_slots);
    hashnode_t *node = inst->table[slot];
    while (node) {
        if (strcmp(node->key, key) == 0) {
            break;
        }
        node = node->next;
    }
    if (!node) {
        //KV_LOG("kvs_hash_mod failed, key not exist\n");
        return -2;
    }
#if ENABLE_KEY_POINTER
    kvs_free(node->value);
    char *vcopy = (char *)kvs_malloc(strlen(value) + 1);
    if (!vcopy) {
        KV_LOG("kvs_hash_mod failed, malloc failed\n");
        return -3;
    }
    memset(vcopy, 0, strlen(value) + 1);
    strcpy(vcopy, value);
    node->value = vcopy;
#else
    memset(node->value, 0, MAX_VALUE_LEN);
    strncpy(node->value, value, MAX_VALUE_LEN);
#endif
    node->timestamp = timestamp;
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
        KV_LOG("kvs_hash_delete failed, invalid param\n");
        return -1;
    }

    int slot = _hash(key, hash->max_slots);
    hashnode_t *node = hash->table[slot];
    if (node == NULL) {
        KV_LOG("kvs_hash_delete failed, key not exist\n");
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
        node = node->next;
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

int
kvs_hash_range(kvs_hash_t* inst, const char* start_key, const char* end_key,
    kvs_item_t** results, int* count)
{
    int ret = -1;
    if (inst == NULL || start_key == NULL || end_key == NULL || results == NULL
        || count == NULL) {
        KV_LOG("kvs_hash_range failed, inst or start_key or end_key or results or count is NULL\n");
        return ret;
    }

    if (strcmp(start_key, end_key) > 0) {
        KV_LOG("kvs_hash_range failed, start_key > end_key\n");
        ret = -2;
        return ret;
    }
    KV_LOG("start_key: %s, end_key: %s\n", start_key, end_key);
    int match_count = 0;
    int total_count = kvs_hash_count(inst);
    KV_LOG("current hash total count: %d\n", total_count);

    /*收集所有符合条件的指针*/
    hashnode_t** matched_nodes = (hashnode_t**)kvs_malloc(sizeof(hashnode_t*) * total_count);
    if (!matched_nodes && total_count > 0) {
        KV_LOG("kvs_hash_range failed: memory allocation failed\n");
        ret = -3;
        return ret;
    }

    hashnode_t* node = NULL;
    for (int i = 0; i < inst->max_slots; i++) {
        node = inst->table[i];
        while (node != NULL) {
            if (strcmp(node->key, start_key) >= 0 && strcmp(node->key, end_key) <= 0) {
                matched_nodes[match_count] = node;
                match_count++;
            }
            node = node->next;
        }
    }
    KV_LOG("match count: %d\n", match_count);
    kvs_item_t* result_array = NULL;
    if (match_count == 0) {
        *results = NULL;
        *count = 0;
        ret = 1;
        goto out;
    }

    result_array = kvs_malloc(sizeof(kvs_item_t) * match_count);
    if (!result_array) {
        KV_LOG("kvs_hash_range failed, malloc failed\n");
        ret = -4;
        goto out;
    }

    for (int i = 0; i < match_count; i++) {
        hashnode_t* node = matched_nodes[i];

        result_array[i].key = kvs_malloc(strlen(node->key) + 1);
        if (!result_array[i].key) {
            KV_LOG("kvs_hash_range failed, malloc key failed\n");
            ret = -5;
            goto out;
        }
        memset(result_array[i].key, 0, sizeof(result_array[i].key));
        strcpy(result_array[i].key, node->key);

        result_array[i].value = kvs_malloc(strlen(node->value) + 1);
        if (!result_array[i].value) {
            KV_LOG("kvs_hash_range failed, malloc value failed\n");
            ret = -6;
            goto out;
        }
        memset(result_array[i].value, 0, sizeof(result_array[i].value));
        strcpy(result_array[i].value, node->value);
        result_array[i].timestamp = node->timestamp;
    }

    kvs_free(matched_nodes);
    matched_nodes = NULL;

    qsort(result_array, match_count, sizeof(kvs_item_t), compare_kvs_items);

    *results = result_array;
    *count = match_count;
    return 0;

out:
    if (matched_nodes) {
        KV_LOG("free matched_nodes\n");
        kvs_free(matched_nodes);
    }
    if (result_array) {
        for (int i = 0; i < match_count; i++) {
            if (result_array[i].key != NULL) {
                KV_LOG("free result_array[%d].key\n", i);
                kvs_free(result_array[i].key);
            }
            if (result_array[i].value != NULL) {
                KV_LOG("free result_array[%d].value\n", i);
                kvs_free(result_array[i].value);
            }
        }
        KV_LOG("free result_array\n");
        kvs_free(result_array);
    }
    return ret;
}

int
kvs_hash_get_all(kvs_hash_t* inst, kvs_item_t** results, int* count)
{
    if (inst == NULL || results == NULL || count == NULL) {
        KV_LOG("kvs_hash_get_all failed, inst or results or count is NULL\n");
        return -1;
    }
    int ret = -1;
    int total_count = kvs_hash_count(inst);
    if (total_count == 0) {
        KV_LOG("kvs_hash_get_all failed, total_count is 0\n");
        *results = NULL;
        *count = 0;
        ret = 0;
        return ret;
    }
    KV_LOG("current hash total count: %d\n", total_count);

    hashnode_t* node = NULL;
    kvs_item_t* result_array = kvs_malloc(sizeof(kvs_item_t) * total_count);
    if (!result_array) {
        KV_LOG("kvs_hash_get_all failed, malloc failed\n");
        return -2;        
    }

    int index = 0;

    for (int i = 0; i < inst->max_slots; i++) {
        node = inst->table[i];
        while (node != NULL) {
            result_array[index].key = kvs_malloc(strlen(node->key) + 1);
            if (!result_array[index].key) {
                KV_LOG("kvs_hash_get_all failed, malloc key failed\n");
                ret = -3;
                goto out;
            }
            memset(result_array[index].key, 0, sizeof(result_array[i].key));
            strcpy(result_array[index].key, node->key);

            result_array[index].value = kvs_malloc(strlen(node->value) + 1);
            if (!result_array[index].value) {
                KV_LOG("kvs_hash_get_all failed, malloc value failed\n");
                ret = -4;
                goto out;        
            }
            memset(result_array[index].value, 0, sizeof(result_array[i].value));
            strcpy(result_array[index].value, node->value);
            result_array[index].timestamp = node->timestamp;
            index++;
            node = node->next;
        }
    }
    *results = result_array;
    *count = index;
    return 0;

out:
    if (result_array) {
        for (int i = 0; i < index; i++) {
            if (result_array[i].key != NULL) {
                KV_LOG("free result_array[%d].key\n", i);
                kvs_free(result_array[i].key);
                result_array[i].key = NULL;
            }
            if (result_array[i].value != NULL) {
                KV_LOG("free result_array[%d].value\n", i);
                kvs_free(result_array[i].value);
                result_array[i].value = NULL;
            }
        }
    }
    kvs_free(result_array);
    return ret;
}

uint64_t
kvs_hash_get_timestamp(kvs_hash_t* inst, const char* key)
{
    if (inst == NULL || key == NULL) {
        KV_LOG("kvs_hash_get_timestamp failed, inst or key is NULL\n");
        return 0;
    }
    hashnode_t *node = NULL;
    int slot = _hash(key, inst->max_slots);
    node = inst->table[slot];
    while (node != NULL) {
        if (strcmp(node->key, key) == 0) {
            return node->timestamp;
        }
        node = node->next;
    }
    return 0;
}