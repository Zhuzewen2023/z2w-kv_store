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
        if (strcmp(inst->table[i].key, key) == 0) {
            kvs_free(inst->table[i].key);
            kvs_free(inst->table[i].value);
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
        //KV_LOG("kvs_array_modify failed, Table is empty\n");
        return -2;
    }

    int i = 0;
    for (i = 0; i < inst->total; i++) {
        if (inst->table[i].key == NULL) {
            continue;
        }

        if (strcmp(inst->table[i].key, key) == 0) {
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