#include "kv_save.h"
#include "kv_log.h"
#include "kv_store_array.h"

#include <stdio.h>

int 
kvs_array_save(const char* filename)
{
    int ret = -1;
    KV_LOG("save kv store to file %s\n", filename);
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        KV_LOG("failed to open file %s\n", filename);
        ret = -1;
        goto out;
    }

    int total_count = kvs_array_count(&global_array);
    KV_LOG("total count %d\n", total_count);
    int res = fwrite(&total_count, sizeof(int), 1, fp);
    KV_LOG("fwrite(&total_count, sizeof(int), 1, fp); res = %d\n", res);
    if (res != 1) {
        KV_LOG("failed to write total count\n");
        ret = -2;
        goto out;
    }
    for (int i = 0; i < total_count; i++) {
        int key_len = strlen(global_array.table[i].key) + 1;
        int value_len = strlen(global_array.table[i].value) + 1;
        res = fwrite(&key_len, sizeof(int), 1, fp);
        if (res != 1) {
            KV_LOG("failed to write key len\n");
            ret = -3;
            goto out;
        }
        res = fwrite(global_array.table[i].key, key_len, 1, fp);
        if (res != 1) {
            KV_LOG("failed to write key\n");
            ret = -4;
            goto out;
        }
        res = fwrite(&value_len, sizeof(int), 1, fp);
        if (res != 1) {
            KV_LOG("failed to write value len\n");
            ret = -5;
            goto out;
        }
        res = fwrite(global_array.table[i].value, value_len, 1, fp);
        if (res != 1) {
            KV_LOG("failed to write value\n");
            ret = -6;
            goto out;
        }
    }
    
    ret = 0;
out:
    if (fp) {
        fclose(fp);
    }
    return ret;
}

int
kvs_array_load(const char* filename)
{
    int ret = -1;
    KV_LOG("load kv store from file %s\n", filename);
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        KV_LOG("failed to open file %s\n", filename);
        ret = -1;
        goto out;
    }

    int total_count = 0;
    int res = fread(&total_count, sizeof(int), 1, fp);
    if (res != 1) {
        KV_LOG("failed to read total count\n");
        ret = -2;
        goto out;
    }
    KV_LOG("total count %d\n", total_count);
    for (int i = 0; i < total_count; i++) {
        int key_len = 0;
        int value_len = 0;
        res = fread(&key_len, sizeof(int), 1, fp);
        if (res != 1) {
            KV_LOG("failed to read key len\n");
            ret = -3;
            goto out;
        }
        KV_LOG("key len %d\n", key_len);
        char *key = (char*)kvs_malloc(key_len);
        if (!key) {
            KV_LOG("failed to malloc key\n");
            ret = -4;
            goto out;
        }
        res = fread(key, key_len, 1, fp);
        if (res != 1) {
            KV_LOG("failed to read key\n");
            ret = -5;
            kvs_free(key);
            goto out;
        }
        KV_LOG("key %s\n", key);
        res = fread(&value_len, sizeof(int), 1, fp);
        if (res != 1) {
            KV_LOG("failed to read value len\n");
            ret = -6;
            kvs_free(key);
            goto out;
        }
        KV_LOG("value len %d\n", value_len);
        char *value = (char*)kvs_malloc(value_len);
        if (!value) {
            KV_LOG("failed to malloc value\n");
            ret = -7;
            kvs_free(key);
            goto out;
        }
        res = fread(value, value_len, 1, fp);
        if (res != 1) {
            KV_LOG("failed to read value\n");
            ret = -8;
            kvs_free(key);
            kvs_free(value);
            goto out;
        }
        KV_LOG("value %s\n", value);
        kvs_array_set(&global_array, key, value);
        kvs_free(key);
        kvs_free(value);
        KV_LOG("value %s\n", global_array.table[i].value);
    }

    ret = 0;
out:
    if (fp) {
        fclose(fp);
    }
    return ret;
}