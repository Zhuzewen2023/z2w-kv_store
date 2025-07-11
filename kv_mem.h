#ifndef __KV_MEM_H__
#define __KV_MEM_H__

#include <stdlib.h>
#include <stddef.h>

void* kvs_malloc(size_t size);

void* kvs_calloc(size_t nmemb, size_t size);

void kvs_free(void* ptr);

void* kvs_realloc(void* ptr, size_t size);

#endif
