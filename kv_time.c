#include "kv_time.h"
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

uint64_t
get_current_timestamp_ms()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (uint64_t)(tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

#ifdef __cplusplus
}
#endif