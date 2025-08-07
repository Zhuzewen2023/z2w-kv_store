#ifndef __KV_RANGE_H__
#define __KV_RANGE_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct kvs_item {
	char* key;
	char* value;
} kvs_item_t;

#ifdef __cplusplus
}
#endif

#endif

