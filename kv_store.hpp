#ifndef __KV_STORE_H__
#define __KV_STORE_H__

#include "reactor.hpp"

#define NETWORK_EPOLL   0
#define NETWORK_NTYCO   1
#define NETWORK_IOURING 2

#define ENABLE_ARRAY_KV_ENGINE 1
#define ENABLE_RBTREE_KV_ENGINE 1
#define ENABLE_HASH_KV_ENGINE 1
#define ENABLE_SKIPTABLE_KV_ENGINE 1
#define ENABLE_NETWORK_SELECT   NETWORK_EPOLL

#if ENABLE_ARRAY_KV_ENGINE
#include "kv_store_array.h"
#endif
#if ENABLE_RBTREE_KV_ENGINE
#include "kv_store_rbtree.h"
#endif
#if ENABLE_HASH_KV_ENGINE
#include "kv_store_hash.h"
#endif
#if ENABLE_SKIPTABLE_KV_ENGINE
#include "kv_store_skiptable.h"
#endif

#if ( ENABLE_NETWORK_SELECT == NETWORK_EPOLL )
#include "kv_store_epoll.hpp"
#elif ( ENABLE_NETWORK_SELECT == NETWORK_NTYCO )
#include "kv_store_ntyco.hpp"
#elif ( ENABLE_NETWORK_SELECT == NETWORK_IOURING )
#include "kv_store_iouring.hpp"
#endif

#endif
