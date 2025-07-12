#ifndef __TEST_QPS_TCPCLIENT_H__
#define __TEST_QPS_TCPCLIENT_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct test_context_s{
    char serverip[16];
    int port;
    int threadnum;
    int connection;
    int requestion;
    int mode;

#if 1
    int failed;
#endif

} test_context_t;

#define TIME_SUB_MS(tv1, tv2) ((tv1.tv_sec - tv2.tv_sec) * 1000 + (tv1.tv_usec - tv2.tv_usec) / 1000)
#define TEST_MESSAGE "ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890abcdefghijklmnopqrstuvwxyz\r\n"
#define RBUFFER_LENGTH    128

int connect_tcpserver(const char *ip, unsigned short port);
int send_recv_tcppkt(int fd);

#ifdef __cplusplus
}
#endif

#endif
