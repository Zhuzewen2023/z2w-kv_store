#ifndef __KV_TIME_H__
#define __KV_TIME_H__

#include <sys/time.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint64_t
get_current_timestamp_ms(void);

#ifdef __cplusplus
}
#endif

#endif

