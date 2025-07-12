#ifndef __KV_LOG_H__
#define __KV_LOG_H__
      

#ifdef ENABLE_LOG
#define KV_LOG(_fmt, ...)   fprintf(stdout, "[%s: %d]: " _fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define KV_LOG(_fmt, ...)
#endif

#endif