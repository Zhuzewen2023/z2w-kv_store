#include "kv_store_skiptable.h"
#include "kv_store.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

kvs_skiptable_t global_skiptable;

static int
random_level()
{
    int level = 1;
    while ((rand() < RAND_MAX * P) && (level < MAX_LEVEL)) {
        level++;
    }
    return level;
}

static Node* create_node(int level, const char* key, const char* value)
{
    Node* node = (Node*)kvs_malloc(sizeof(Node));
    if (!node) {
        KV_LOG("create node failed\n");
        return NULL;
    }

    strncpy(node->key, key, sizeof(node->key) - 1);
    node->key[sizeof(node->key) - 1] = '\0';
    strncpy(node->value, value, sizeof(node->value) - 1);
    node->value[sizeof(node->value) - 1] = '\0';

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
    node->forward = (Node**)kvs_malloc(sizeof(Node*) * (level + 1));
    if (!node->forward) {
        KV_LOG("create node forward failed\n");
        return NULL;
    }

    return node;
}

#if USE_TIMESTAMP
static Node* create_node_with_timestamp(int level, const char* key, const char* value, uint64_t timestamp)
{
    Node* node = create_node(level, key, value);
    if (!node) {
        KV_LOG("create node failed\n");
        return NULL;
    }

    node->timestamp = timestamp;
    return node;
}
#endif

int 
kvs_skiptable_create(kvs_skiptable_t *table)
{
    srand(time(NULL)); //初始化随机种子

    table->level = 0;
    table->size = 0;
    table->header = create_node(MAX_LEVEL, "", ""); /*头结点，不存储实际数据*/
    if (!table->header) {
        KV_LOG("create header failed\n");
        return -1;
    }

    for (int i = 0; i <= MAX_LEVEL; i++) {
        table->header->forward[i] = NULL;
    }

    return 0;
}

void 
kvs_skiptable_destroy(kvs_skiptable_t *table)
{
    Node* current = table->header->forward[0];
    Node* next;

    kvs_free(table->header->forward);
    kvs_free(table->header);

    while(current) {
        next = current->forward[0];
        free(current->forward);
        free(current);
        current = next;
    }

    table->level = 0;
    table->size = 0;
    table->header = NULL;
}

int 
kvs_skiptable_set(kvs_skiptable_t *inst, const char *key, const char *value)
{
#if USE_TIMESTAMP
    uint64_t timestamp = 0;
    timestamp = get_current_timestamp_ms();
    KV_LOG("kvs_skiptable_set timestamp: %lu\n", timestamp);
    return kvs_skiptable_set_with_timestamp(inst, key, value, timestamp);
#else
    Node* update[MAX_LEVEL + 1] = {0};
    Node* current = inst->header;

    /*从最高层开始搜索*/
    for (int i = inst->level; i >= 0; i--) {
        while (current->forward[i] && strcmp(current->forward[i]->key, key) < 0) {
            current = current->forward[i];
        }
        update[i] = current; /*记录前驱节点*/
    }

    /*达到最底层并移动到实际位置*/
    current = current->forward[0];

    /*检查key是否已经存在*/
    if (current && strcmp(current->key, key) == 0) {
        /*key已经存在，返回大于0的错误码*/
        KV_LOG("key %s already exists\n", key);
        return 1;
    }

    /*随机生成新节点的层数，创建新节点*/
    int new_level = random_level();
    Node *new_node = create_node(new_level, key, value);
    if (!new_node) {
        KV_LOG("create new node failed\n");
        return -1;
    }

    /*如果新节点层数高于当前层数，初始化header*/
    if (new_level > inst->level) {
        for (int i = inst->level + 1; i <= new_level; i++) {
            /*此时新层的前驱节点就是头节点*/
            update[i] = inst->header;
        }
        inst->level = new_level;
    }

    /*将新节点插入到链表中*/
    for (int i = 0; i <= new_level; i++) {
        new_node->forward[i] = update[i]->forward[i];
        update[i]->forward[i] = new_node;
    }

    inst->size++;
    return 0;
#endif
}

#if USE_TIMESTAMP
int
kvs_skiptable_set_with_timestamp(kvs_skiptable_t *inst, const char* key, const char* value, uint64_t timestamp)
{
    Node* update[MAX_LEVEL + 1] = {0};
    Node* current = inst->header;

    /*从最高层开始搜索*/
    for (int i = inst->level; i >= 0; i--) {
        while (current->forward[i] && strcmp(current->forward[i]->key, key) < 0) {
            current = current->forward[i];
        }
        update[i] = current; /*记录前驱节点*/
    }

    /*达到最底层并移动到实际位置*/
    current = current->forward[0];

    /*检查key是否已经存在*/
    if (current && strcmp(current->key, key) == 0) {
        /*key已经存在，返回大于0的错误码*/
        KV_LOG("key %s already exists\n", key);
        return 1;
    }

    /*随机生成新节点的层数，创建新节点*/
    int new_level = random_level();
    Node *new_node = create_node_with_timestamp(new_level, key, value, timestamp);
    if (!new_node) {
        KV_LOG("create new node failed\n");
        return -1;
    }

    /*如果新节点层数高于当前层数，初始化header*/
    if (new_level > inst->level) {
        for (int i = inst->level + 1; i <= new_level; i++) {
            /*此时新层的前驱节点就是头节点*/
            update[i] = inst->header;
        }
        inst->level = new_level;
    }

    /*将新节点插入到链表中*/
    for (int i = 0; i <= new_level; i++) {
        new_node->forward[i] = update[i]->forward[i];
        update[i]->forward[i] = new_node;
    }

    inst->size++;
    return 0;
}
#endif

char* 
kvs_skiptable_get(kvs_skiptable_t *table, const char *key)
{
    Node *current = table->header;

    /*从最高层开始搜索*/
    for (int i = table->level; i >= 0; i--) {
        while (current->forward[i] && strcmp(current->forward[i]->key, key) < 0) {
            current = current->forward[i];
        }
    }

    current = current->forward[0];
    if (current && strcmp(current->key, key) == 0) {
        return current->value;
    }
    KV_LOG("key %s not found\n", key);
    return NULL;
}

int 
kvs_skiptable_modify(kvs_skiptable_t *inst, const char *key, const char *value)
{
#if USE_TIMESTAMP
    uint64_t timestamp = 0;
    timestamp = get_current_timestamp_ms();
    KV_LOG("kvs_skiptable_modify timestamp: %lu\n", timestamp);
    return kvs_skiptable_modify_with_timestamp(inst, key, value, timestamp);
#else
    Node *node = inst->header;
    for (int i = inst->level; i >= 0; i--) {
        while (node->forward[i] && strcmp(node->forward[i]->key, key) < 0) {
            node = node->forward[i];
        }
    }

    node = node->forward[0];
    if (node && strcmp(node->key, key) == 0) {
        strncpy(node->value, value, sizeof(node->value) - 1);
        node->value[sizeof(node->value) - 1] = '\0';
        return 0;
    }
    KV_LOG("key %s not found\n", key);
    return -1;
#endif
}

#if USE_TIMESTAMP
int
kvs_skiptable_modify_with_timestamp(kvs_skiptable_t *inst, const char* key, const char* value, uint64_t timestamp)
{
    Node *node = inst->header;
    for (int i = inst->level; i >= 0; i--) {
        while (node->forward[i] && strcmp(node->forward[i]->key, key) < 0) {
            node = node->forward[i];
        }
    }

    node = node->forward[0];
    if (node && strcmp(node->key, key) == 0) {
        strncpy(node->value, value, sizeof(node->value) - 1);
        node->value[sizeof(node->value) - 1] = '\0';
        node->timestamp = timestamp;
        return 0;
    }
    KV_LOG("key %s not found\n", key);
    return -1;
}
#endif

int 
kvs_skiptable_count(kvs_skiptable_t *table)
{
    return table->size;
}

int 
kvs_skiptable_delete(kvs_skiptable_t *table, char *key)
{
    Node* update[MAX_LEVEL + 1] = {0};
    Node* current = table->header;

    for (int i = table->level; i >= 0; i--) {
        while (current->forward[i] && strcmp(current->forward[i]->key, key) < 0) {
            current = current->forward[i];
        }
        update[i] = current; /*记录每一层的前驱节点*/
    }

    current = current->forward[0];
    if (current == NULL || strcmp(current->key, key) != 0) {
        KV_LOG("key %s not found\n", key);
        return -1;
    }

    for (int i = 0; i <= table->level; i++) {
        if (update[i]->forward[i] != current) {
            /*比删除的节点更高层的节点，不存在current或者等于NULL*/
            break;
        }
        update[i]->forward[i] = current->forward[i];
    }

    /*清理空层*/
    while (table->level > 0 && table->header->forward[table->level] == NULL) {
        table->level--;
    }

    table->size--;
    kvs_free(current->forward);
    kvs_free(current);
    return 0;
}

int 
kvs_skiptable_exist(kvs_skiptable_t *table, char *key)
{
    if (kvs_skiptable_get(table, key) == NULL) {
        return -1;
    }
    return 0;
}

int
kvs_skiptable_range(kvs_skiptable_t* inst, const char* start_key, const char* end_key,
    kvs_item_t** results, int* count)
{
    int ret = -1;
    if (inst == NULL || start_key == NULL || end_key == NULL || results == NULL
        || count == NULL) {
        KV_LOG("kvs_skiptable_range failed, inst or start_key or end_key or results or count is NULL\n");
        return ret;
    }

    if (strcmp(start_key, end_key) > 0) {
        KV_LOG("kvs_skiptable_range failed, start_key > end_key\n");
        ret = -2;
        return ret;
    }
    KV_LOG("start_key: %s, end_key: %s\n", start_key, end_key);

    /*直接分配最大可能的结果数组*/
    kvs_item_t* result_array = kvs_malloc(sizeof(kvs_item_t) * inst->size);
    if (!result_array) {
        KV_LOG("Memory allocation failed\n");
        return -3;
    }

    int match_count = 0;
    Node* current = inst->header->forward[0];
    while (current != NULL) {
        int cmp_start = strcmp(current->key, start_key);
        int cmp_end = strcmp(current->key, end_key);

        if (cmp_start >= 0 && cmp_end <= 0) {
            result_array[match_count].key = kvs_malloc(strlen(current->key) + 1);
            if (!result_array[match_count].key) {
                KV_LOG("Key allocation failed\n");
                goto out;
            }

            result_array[match_count].value = kvs_malloc(strlen(current->value) + 1);
            if (!result_array[match_count].value) {
                KV_LOG("Value allocation failed\n");
                goto out;
            }

            // 复制数据
            strcpy(result_array[match_count].key, current->key);
            strcpy(result_array[match_count].value, current->value);
#if USE_TIMESTAMP
            result_array[match_count].timestamp = current->timestamp;
#endif
            match_count++;
        }

        if (cmp_end > 0) break;

        current = current->forward[0];
        
    }
    KV_LOG("match count: %d\n", match_count);

    if (match_count == 0) {
        *results = NULL;
        *count = 0;
        ret = 1;
        goto out;
    }

    *results = result_array;
    *count = match_count;
    return 0;

out:
    if (result_array) {
        for (int i = 0; i < match_count; i++) {
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

int
kvs_skiptable_get_all(kvs_skiptable_t* inst, kvs_item_t** results, int* count)
{
    if (inst == NULL || results == NULL || count == NULL) {
        KV_LOG("kvs_skiptable_get_all failed, inst or results or count is NULL\n");
        return -1;
    }
    int ret = -1;
    int total_count = kvs_skiptable_count(inst);
    if (total_count == 0) {
        KV_LOG("kvs_skiptable_get_all failed, total_count is 0\n");
        *results = NULL;
        *count = 0;
        ret = 0;
        return ret;
    }
    KV_LOG("current skiptable total count: %d\n", total_count);

    /*直接分配最大可能的结果数组*/
    kvs_item_t* result_array = kvs_malloc(sizeof(kvs_item_t) * total_count);
    if (!result_array) {
        KV_LOG("Memory allocation failed\n");
        return -2;
    }

    int match_count = 0;
    Node* current = inst->header->forward[0];
    while (current != NULL) {
        result_array[match_count].key = kvs_malloc(strlen(current->key) + 1);
        if (!result_array[match_count].key) {
            KV_LOG("Key allocation failed\n");
            ret = -3;
            goto out;
        }

        result_array[match_count].value = kvs_malloc(strlen(current->value) + 1);
        if (!result_array[match_count].value) {
            KV_LOG("Value allocation failed\n");
            ret = -4;
            goto out;
        }

        // 复制数据
        strcpy(result_array[match_count].key, current->key);
        strcpy(result_array[match_count].value, current->value);
#if USE_TIMESTAMP
        result_array[match_count].timestamp = current->timestamp;
#endif
        match_count++;

        current = current->forward[0];
    }

    *results = result_array;
    *count = match_count;
    return 0;

out:
    if (result_array) {
        for (int i = 0; i < match_count; i++) {
            if (result_array[i].key != NULL) {
                kvs_free(result_array[i].key);
                result_array[i].key = NULL;
            }
            if (result_array[i].value != NULL) {
                kvs_free(result_array[i].value);
                result_array[i].value = NULL;
            }
        }
        kvs_free(result_array);
    }
    return ret;
}

#if USE_TIMESTAMP
uint64_t
kvs_skiptable_get_timestamp(kvs_skiptable_t* inst, const char* key)
{
    if (inst == NULL || key == NULL) {
        KV_LOG("kvs_skiptable_get_timestamp failed, inst or key is NULL\n");
        return 0;
    }
    Node* current = inst->header;

    for (int i = inst->level; i >= 0; i--) {
        while (current->forward[i] && strcmp(current->forward[i]->key, key) < 0) {
            current = current->forward[i];
        }
    }

    current = current->forward[0];
    if (current == NULL || strcmp(current->key, key) != 0) {
        KV_LOG("key %s not found\n", key);
        return 0;
    }
    return current->timestamp;
}
#endif