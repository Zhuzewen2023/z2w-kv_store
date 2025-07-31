#ifndef __KV_STORE_RBTREE_H__
#define __KV_STORE_RBTREE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RED     1
#define BLACK   2

#define ENABLE_KEY_CHAR 1

#if ENABLE_KEY_CHAR
typedef char* KEY_TYPE;
#else
typedef int KEY_TYPE;
#endif

typedef struct _rbtree_node
{
    unsigned char color;
    struct _rbtree_node *right;
    struct _rbtree_node *left;
    struct _rbtree_node *parent;
    KEY_TYPE key;
    void* value;
} rbtree_node;

typedef struct _rbtree
{
    rbtree_node *root;
    rbtree_node *nil;
} rbtree;

typedef struct _rbtree kvs_rbtree_t;

extern kvs_rbtree_t global_rbtree;

int 
kvs_rbtree_create(kvs_rbtree_t *inst);

void
kvs_rbtree_destroy(kvs_rbtree_t *inst);

int 
kvs_rbtree_set(kvs_rbtree_t *inst, char *key, char *value);

char*
kvs_rbtree_get(kvs_rbtree_t *inst, char *key);

int
kvs_rbtree_delete(kvs_rbtree_t *inst, char *key);

int
kvs_rbtree_modify(kvs_rbtree_t *inst, char *key, char *value);

int
kvs_rbtree_exist(kvs_rbtree_t *inst, char *key);

#ifdef __cplusplus
}
#endif

#endif
