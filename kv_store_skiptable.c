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
kvs_skiptable_set(kvs_skiptable_t *table, char *key, char *value)
{
    Node* update[MAX_LEVEL + 1] = {0};
    Node* current = table->header;

    /*从最高层开始搜索*/
    for (int i = table->level; i >= 0; i--) {
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
    if (new_level > table->level) {
        for (int i = table->level + 1; i <= new_level; i++) {
            /*此时新层的前驱节点就是头节点*/
            update[i] = table->header;
        }
        table->level = new_level;
    }

    /*将新节点插入到链表中*/
    for (int i = 0; i <= new_level; i++) {
        new_node->forward[i] = update[i]->forward[i];
        update[i]->forward[i] = new_node;
    }

    table->size++;
    return 0;
}

char* 
kvs_skiptable_get(kvs_skiptable_t *table, char *key)
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
kvs_skiptable_modify(kvs_skiptable_t *table, char *key, char *value)
{
    Node *node = table->header;
    for (int i = table->level; i >= 0; i--) {
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
}

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