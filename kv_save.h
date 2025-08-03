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

#ifdef __cplusplus
}
#endif

#endif
