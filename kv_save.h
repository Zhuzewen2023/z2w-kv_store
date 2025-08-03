#ifndef __KV_SAVE_H__
#define __KV_SAVE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "kv_store.hpp"
#include "kv_store_array.h"

#if ENABLE_ARRAY_KV_ENGINE
int 
kvs_array_save(const char* filename);

int
kvs_array_load(const char* filename);
#endif
#if ENABLE_RBTREE_KV_ENGINE
int
kvs_rbtree_save(const char* filename);

int
kvs_rbtree_load(const char* filename);
#endif
#if ENABLE_HASH_KV_ENGINE
int
kvs_hash_save(const char* filename);

int
kvs_hash_load(const char* filename);
#endif
#if ENABLE_SKIPTABLE_KV_ENGINE
int
kvs_skiptable_save(const char* filename);

int
kvs_skiptable_load(const char* filename);
#endif

#ifdef __cplusplus
}
#endif

#endif
