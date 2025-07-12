#ifndef __NTYCO_SERVER_H__
#define __NTYCO_SERVER_H__

#ifdef __cplusplus
extern "C" {
#endif

#define BUFFER_LENGTH 1024

typedef int (*RCALLBACK)(int fd);


typedef struct conn_item {
    int fd;
    char rbuffer[BUFFER_LENGTH];
    int rlen;
    char wbuffer[BUFFER_LENGTH];
    int wlen;

    union {
        RCALLBACK accept_callback;
        RCALLBACK recv_callback;
    } recv_t;

    RCALLBACK send_callback;

} conn_item_t;

enum Command{
    START = 0, 
    SET = START, 
    GET, 
    DEL, 
    MOD,
    COUNT,
};



int ntyco_entry(int port);
int kvstore_request(struct conn_item *item);
int kv_store_split_token(char *msg, char** tokens);
#ifdef __cplusplus
}
#endif

#endif
