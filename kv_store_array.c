#include "kv_store_array.h"
#include "kv_log.h"

kvs_array_t global_array = {0};

int 
kvs_array_create(kvs_array_t *inst)
{
    if(!inst) return -1;
    if (inst->table) {
        KV_LOG("kvs_array_create failed, Table already exists\n");
        return -1;
    }
    inst->table = kvs_malloc(KVS_ARRAY_SIZE * sizeof(kvs_array_item_t));
    if (inst->table == NULL) {
        return -1;
    }

    inst->total = 0;
    return 0;
}

int 
kvs_array_destroy(kvs_array_t *inst)
{
    if (inst == NULL) {
        return -1;
    }
    if (inst->table) {
        for (int i = 0; i < inst->total; i++) {
            if (inst->table[i].key) {
                kvs_free(inst->table[i].key);
            }
            if (inst->table[i].value) {
                kvs_free(inst->table[i].value);
            }
        }
        kvs_free(inst->table);
        inst->table = NULL;
    }
}

int 
kvs_array_set(kvs_array_t *inst, char *key, char *value) 
{
    if (inst == NULL || key == NULL || value == NULL) {
        return -1;
    }
    if (inst->total >= KVS_ARRAY_SIZE) {
        return -1;
    }

    char* str = kvs_array_get(inst, key);
    if (str != NULL) {
        //KV_LOG("set failed, Key %s already exists\n");
        return 1;
    }

    char *kcopy = (char *)kvs_malloc(strlen(key) + 1);
    if(kcopy == NULL) {
        return -2;
    }
    memset(kcopy, 0, strlen(key) + 1);
    strncpy(kcopy, key, strlen(key));

    char *vcopy = (char *)kvs_malloc(strlen(value) + 1);
    if(vcopy == NULL) {
        kvs_free(kcopy);
        return -3;
    }
    memset(vcopy, 0, strlen(value) + 1);
    strncpy(vcopy, value, strlen(value));

    int i = 0;
    for (i = 0; i < inst->total; i++) {
        if (inst->table[i].key == NULL) {
            inst->table[i].key = kcopy;
            inst->table[i].value = vcopy;
            inst->total++;
            return 0;
        }
    }

    if (i == inst->total && i < KVS_ARRAY_SIZE) {
        inst->table[i].key = kcopy;
        inst->table[i].value = vcopy;
        inst->total++;
    }

    return 0;
}

char* 
kvs_array_get(kvs_array_t *inst, char *key) 
{
    if (inst == NULL || key == NULL) {
        KV_LOG("kvs_array_get failed, inst or key is NULL\n");
        return NULL;
    }
    int i = 0;

    for (i = 0; i < inst->total; i++) {
        if (inst->table[i].key == NULL) {
            continue;
        }
        if (strcmp(inst->table[i].key, key) == 0) {
            return inst->table[i].value;
        }
    }
    return NULL;
}

int 
kvs_array_delete(kvs_array_t *inst, char *key) 
{
    if (inst == NULL || key == NULL) {
        return -1;
    }
    int i = 0;
    for (i = 0; i < inst->total; i++) {
        if (inst->table[i].key == NULL) {
            KV_LOG("table[%d].key is NULL,continue..\n", i);
            continue;
        }
        if (strcmp(inst->table[i].key, key) == 0) {
            if (inst->table[i].key) {
                kvs_free(inst->table[i].key);
            }
            if (inst->table[i].value) {
                kvs_free(inst->table[i].value);
            }
            inst->table[i].key = NULL;
            inst->table[i].value = NULL;

            if(i == inst->total - 1) {
                inst->total--;
            }
        }
        return 0;
    }

    return i; // Return index of first empty slot
}

int 
kvs_array_modify(kvs_array_t *inst, char *key, char *value) 
{
    if (inst == NULL || key == NULL || value == NULL) {
        KV_LOG("kvs_array_modify failed, inst or key or value is NULL\n");
        return -1;
    }

    if (inst->total == 0) {
        KV_LOG("kvs_array_modify failed, Table is empty\n");
        return -2;
    }

    int i = 0;
    char all_null_flag = 1;
    for (i = 0; i < inst->total; i++) {
        if (inst->table[i].key == NULL) {
            continue;
        }

        if (strcmp(inst->table[i].key, key) == 0) {
            all_null_flag = 0;
            KV_LOG("kvs_array_modify get key %s\n", inst->table[i].key);
            kvs_free(inst->table[i].value);
            inst->table[i].value = NULL;

            char *vcopy = kvs_malloc(strlen(value) + 1);
            if (vcopy == NULL) {
                KV_LOG("kvs_array_modify failed, malloc failed\n");
                return -3;
            } else {
                memset(vcopy, 0, strlen(value) + 1);
                strncpy(vcopy, value, strlen(value) + 1);
                inst->table[i].value = vcopy;
                KV_LOG("kvs_array_modify set value %s\n", vcopy);
                return 0;
            }
        }
    }
    if (all_null_flag) {
        return -4;
    }
    return i;
}

int 
kvs_array_exist(kvs_array_t *inst, char *key) {
    if (inst == NULL || key == NULL) {
        KV_LOG("kvs_array_exist failed, inst or key is NULL\n");
        return -1;
    }

    char *str = kvs_array_get(inst, key);
    if (str == NULL) {
        return -1;
    }
    return 0;
}

int
kvs_array_count(kvs_array_t *inst) {
    if (inst == NULL) {
        return -1;
    }
    return inst->total;
}

int
kvs_array_range(kvs_array_t *inst, const char* start_key, const char* end_key, 
               kvs_array_item_t **results, int* count) 
{
    int ret = -1;
    if (inst == NULL || start_key == NULL || end_key == NULL || results == NULL 
        || count == NULL) {
        KV_LOG("kvs_array_range failed, inst or start_key or end_key or results or count is NULL\n");
        return ret;
    }

    if (strcmp(start_key, end_key) > 0) {
        KV_LOG("kvs_array_range failed, start_key > end_key\n");
        ret = -2;
        return ret;
    }
    KV_LOG("start_key: %s, end_key: %s\n", start_key, end_key);
    /*统计符合条件的项数*/
    int match_count = 0;
    for (int i = 0; i < inst->total; i++) {
        if (inst->table[i].key != NULL) {
            if (strcmp(inst->table[i].key, start_key) >= 0 
                && strcmp(inst->table[i].key, end_key) <= 0) {
                match_count++;
            }
        }
    }
    KV_LOG("match count: %d\n", match_count);

    if (match_count == 0) {
        *results = NULL;
        *count = 0;
        ret = 1;
        return ret;
    }

    kvs_array_item_t *result_array = kvs_malloc(sizeof(kvs_array_item_t) * match_count);
    if (result_array == NULL) {
        KV_LOG("kvs_array_range failed, malloc failed\n");
        ret = -3;
        return ret;
    }

    int index = 0;
    for (int i = 0; i < inst->total; i++) {
        if (inst->table[i].key != NULL) {
            if (strcmp(inst->table[i].key, start_key) >= 0 
                && strcmp(inst->table[i].key, end_key) <= 0) {
                result_array[index].key = (char *)kvs_malloc(strlen(inst->table[i].key) + 1);
                if (result_array[index].key == NULL) {
                    KV_LOG("kvs_array_range failed, malloc failed\n");
                    ret = -4;
                    goto out;
                }
                memset(result_array[index].key, 0, strlen(inst->table[i].key) + 1);
                strncpy(result_array[index].key, inst->table[i].key, strlen(inst->table[i].key) + 1);
                result_array[index].value = (char *)kvs_malloc(strlen(inst->table[i].value) + 1);
                if (result_array[index].value == NULL) {
                    KV_LOG("kvs_array_range failed, malloc failed\n");
                    ret = -5;
                    goto out;
                }
                memset(result_array[index].value, 0, strlen(inst->table[i].value) + 1);
                strncpy(result_array[index].value, inst->table[i].value, strlen(inst->table[i].value) + 1);
                index++;
            }
        }
    }
    KV_LOG("index: %d\n", index);

    *results = result_array;
    *count = index;
    ret = 0;
    return ret;

    
out:
    if (result_array) {
        for (int i = 0; i < index; i++) {
            if (result_array[i].key != NULL) {
                kvs_free(result_array[i].key);
            }
            if (result_array[i].value != NULL) {
                kvs_free(result_array[i].value);
            }
        }
        kvs_free(result_array);
    }
    return ret;    
}

