#include "kv_store_array.h"
#include "kv_log.h"

kvs_array_item_t array_table[KVS_ARRAY_SIZE] = {0};
int array_idx = 0;

int kvs_array_set(char *key, char *value) {
    if (key == NULL || value == NULL || array_idx >= KVS_ARRAY_SIZE) {
        return -1;
    }

    char *kcopy = (char *)kvs_malloc(strlen(key) + 1);
    if(kcopy == NULL) {
        return -1;
    }
    memset(kcopy, 0, strlen(key) + 1);

    char *vcopy = (char *)kvs_malloc(strlen(value) + 1);
    if(vcopy == NULL) {
        kvs_free(kcopy);
        return -1;
    }
    memset(vcopy, 0, strlen(value) + 1);

    strncpy(kcopy, key, strlen(key) + 1);
    strncpy(vcopy, value, strlen(value) + 1);

    array_table[array_idx].key = kcopy;
    array_table[array_idx].value = vcopy;
    array_idx++;

    return 0;
}

char* kvs_array_get(char *key) {
    for (int i = 0; i < array_idx; i++) {
        if (strcmp(array_table[i].key, key) == 0) {
            return array_table[i].value;
        }
    }
    return NULL;
}

int kvs_array_delete(char *key) {
    if (key == NULL) {
        return -1;
    }
    int i = 0;
    for (i = 0; i < array_idx; i++) {
        if (strcmp(array_table[i].key, key) == 0) {
            kvs_free(array_table[i].key);
            kvs_free(array_table[i].value);
            array_table[i].key = NULL;
            array_table[i].value = NULL;

            // Shift all elements down
            for (int j = i; j < array_idx - 1; j++) {
                array_table[j] = array_table[j + 1];
            }
            array_idx--;
            return 0;
        }
    }
    return i; // Return index of first empty slot
}

int kvs_array_modify(char *key, char *value) {
    if (key == NULL || value == NULL || array_idx >= KVS_ARRAY_SIZE) {
        return -1;
    }
    int i = 0;
    for (i = 0; i < array_idx; i++) {
        if (strcmp(array_table[i].key, key) == 0) {
            KV_LOG("kvs_array_modify get key %s\n", array_table[i].key);
            kvs_free(array_table[i].value);
            array_table[i].value = NULL;

            char *vcopy = kvs_malloc(strlen(value) + 1);
            if (vcopy == NULL) {
                return -2;
            } else {
                
                strncpy(vcopy, value, strlen(value) + 1);
                array_table[i].value = vcopy;
                KV_LOG("kvs_array_modify set value %s\n", vcopy);
                return 0;
            }
        }
    }
    return i;
}