#include "kv_save.h"
#include "kv_log.h"
#include "kv_store_array.h"
#include "kv_store_rbtree.h"

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

static int
kvs_rbtree_save_node_recursive(rbtree_node *node, FILE *fp)
{
    if (!node || !fp) {
        KV_LOG("invalid node or fp\n");
        return -1;
    }
        
    if (node == global_rbtree.nil) {
        KV_LOG("node is nil\n");
        return 0;
    }
    if (node != global_rbtree.nil) {
        int key_len = strlen(node->key) + 1;
        int value_len = strlen(node->value) + 1;
        int res = fwrite(&key_len, sizeof(int), 1, fp);
        if (res != 1) {
            KV_LOG("failed to write key len\n");
            return -2;
        }
        res = fwrite(node->key, key_len, 1, fp);
        if (res != 1) {
            KV_LOG("failed to write key\n");
            return -3;
        }
        res = fwrite(&value_len, sizeof(int), 1, fp);
        if (res != 1) {
            KV_LOG("failed to write value len\n");
            return -4;
        }
        res = fwrite(node->value, value_len, 1, fp);
        if (res != 1) {
            KV_LOG("failed to write value\n");
            return -5;
        }
    }
    int ret = 0;
    if (node->left != global_rbtree.nil) {
        ret = kvs_rbtree_save_node_recursive(node->left, fp);
    }
    if (node->right != global_rbtree.nil) {
        ret = kvs_rbtree_save_node_recursive(node->right, fp);
    }
    return ret;
}

int 
kvs_rbtree_save(const char* filename)
{
    int ret = -1;
    KV_LOG("save kvs rbtree store to file %s\n", filename);
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        KV_LOG("failed to open file %s\n", filename);
        ret = -1;
        goto out;
    }

    int total_count = kvs_rbtree_count(&global_rbtree);
    KV_LOG("total count %d\n", total_count);
    int res = fwrite(&total_count, sizeof(int), 1, fp);
    KV_LOG("fwrite(&total_count, sizeof(int), 1, fp); res = %d\n", res);
    if (res != 1) {
        KV_LOG("failed to write total count\n");
        ret = -2;
        goto out;
    }

    ret = kvs_rbtree_save_node_recursive(global_rbtree.root, fp);
    KV_LOG("kvs_rbtree_save_node_recursive(global_rbtree.root, fp); res = %d\n", ret);

out:
    if (fp) {
        fclose(fp);
    }
    return ret;
}

int
kvs_rbtree_load(const char* filename)
{
    int ret = -1;
    KV_LOG("load kv rbtree store from file %s\n", filename);
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
        ret = kvs_rbtree_set(&global_rbtree, key, value);
        KV_LOG("setted value %s\n", kvs_rbtree_get(&global_rbtree, key));
        kvs_free(key);
        kvs_free(value);
        
    }

     ret = 0;
out:
    if (fp) {
        fclose(fp);
    }
    return ret;
}

int
kvs_hash_save(const char* filename)
{
    int ret = -1;
    KV_LOG("save kv hash store to file %s\n", filename);
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        KV_LOG("failed to open file %s\n", filename);
        ret = -1;
        goto out;
    }

    int total_count = kvs_hash_count(&global_hash);
    KV_LOG("total count %d\n", total_count);
    int res = fwrite(&total_count, sizeof(int), 1, fp);
    KV_LOG("fwrite(&total_count, sizeof(int), 1, fp); res = %d\n", res);
    if (res != 1) {
        KV_LOG("failed to write total count\n");
        ret = -2;
        goto out;
    }

    int i = 0;
    while(1) {
        if (i >= MAX_TABLE_SIZE) {
            break;
        }
        if (global_hash.table[i] == NULL) {
            i++;
            continue;
        }
        hashnode_t *current = global_hash.table[i];
        while (current != NULL) {
            int key_len = strlen(current->key) + 1;
            int value_len = strlen(current->value) + 1;
            res = fwrite(&key_len, sizeof(int), 1, fp);
            if (res != 1) {
                KV_LOG("failed to write key len\n");
                ret = -3;
                goto out;
            }
            KV_LOG("write key len %d\n", key_len);
            res = fwrite(current->key, key_len, 1, fp);
            if (res != 1) {
                KV_LOG("failed to write key\n");
                ret = -4;
                goto out;
            }
            KV_LOG("write key %s\n", current->key);
            res = fwrite(&value_len, sizeof(int), 1, fp);
            if (res != 1) {
                KV_LOG("failed to write value len\n");
                ret = -5;
                goto out;
            }
            KV_LOG("write value len %d\n", value_len);
            res = fwrite(current->value, value_len, 1, fp);
            if (res != 1) {
                KV_LOG("failed to write value\n");
                ret = -6;
                goto out;
            }
            KV_LOG("write value %s\n", current->value);
            current = current->next;
        }
        i++;
    }
    
    ret = 0;
out:
    if (fp) {
        fclose(fp);
    }
    return ret;
}

int
kvs_hash_load(const char* filename)
{
    int ret = -1;
    KV_LOG("load kv hash store from file %s\n", filename);
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
        ret = kvs_hash_set(&global_hash, key, value);
        KV_LOG("value %s\n", kvs_hash_get(&global_hash, key));
        kvs_free(key);
        kvs_free(value);
        
    }

     ret = 0;
out:
    if (fp) {
        fclose(fp);
    }
    return ret;
}

int
kvs_skiptable_save(const char* filename)
{
    int ret = -1;
    KV_LOG("save kvs skiptable store to file %s\n", filename);
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        KV_LOG("failed to open file %s\n", filename);
        ret = -1;
        goto out;
    }

    int total_count = kvs_skiptable_count(&global_skiptable);
    KV_LOG("total count %d\n", total_count);
    int res = fwrite(&total_count, sizeof(int), 1, fp);
    KV_LOG("fwrite(&total_count, sizeof(int), 1, fp); res = %d\n", res);
    if (res != 1) {
        KV_LOG("failed to write total count\n");
        ret = -2;
        goto out;
    }

    /*traversal*/
    Node* current = global_skiptable.header->forward[0];
    while (current) {
        int key_len = strlen(current->key);
        int value_len = strlen(current->value);
        res = fwrite(&key_len, sizeof(int), 1, fp);
        if (res != 1) {
            KV_LOG("failed to write key len\n");
            ret = -3;
            goto out;
        }
        KV_LOG("write key len %d\n", key_len);
        res = fwrite(current->key, key_len, 1, fp);
        if (res != 1) {
            KV_LOG("failed to write key\n");
            ret = -4;
            goto out;
        }
        KV_LOG("write key %s\n", current->key);
        res = fwrite(&value_len, sizeof(int), 1, fp);
        if (res != 1) {
            KV_LOG("failed to write value len\n");
            ret = -5;
            goto out;
        }
        KV_LOG("write value len %d\n", value_len);
        res = fwrite(current->value, value_len, 1, fp);
        if (res != 1) {
            KV_LOG("failed to write value\n");
            ret = -6;
            goto out;
        }
        KV_LOG("write value %s\n", current->value);
        current = current->forward[0];
    }

    ret = 0;

out:
    if (fp) {
        fclose(fp);
    }
    return ret;
}

int
kvs_skiptable_load(const char* filename)
{
    int ret = -1;
    KV_LOG("load kv skiptable store from file %s\n", filename);
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
        ret = kvs_skiptable_set(&global_skiptable, key, value);
        KV_LOG("setted value %s\n", kvs_skiptable_get(&global_skiptable, key));
        kvs_free(key);
        kvs_free(value);
        
    }

     ret = 0;
out:
    if (fp) {
        fclose(fp);
    }
    return ret;
}