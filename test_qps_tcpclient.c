#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include "test_qps_tcpclient.h"



static void *test_qps_entry(void *arg)
{
    test_context_t *pctx = (test_context_t *)arg;

    int connect_num = pctx->connection / pctx->threadnum;
    int conn_fds[connect_num]; /*存储连接的文件描述符*/
    int valid_conn_count = 0;
    
    for (int i = 0; i < connect_num; i++) {
        int connfd = connect_tcpserver(pctx->serverip, pctx->port);
        if(connfd < 0){
            printf("connect_tcpserver failed\n");
            continue;
        }
        conn_fds[valid_conn_count++] = connfd;
    }

    if (valid_conn_count == 0) {
        printf("No valid connection established\n");
        return NULL;
    }

    /*计算每个连接处理的请求数*/
    int requests_per_conn = pctx->requestion / (pctx->threadnum * valid_conn_count);
    if (requests_per_conn == 0) {
        requests_per_conn = 1;
    }
    //int count = pctx->requestion / connect_num / pctx->threadnum;
    for (int i = 0; i < valid_conn_count; i++) {
        int connfd = conn_fds[i];
        int i = 0;
        int res;
        
        while (i++ < requests_per_conn){
            res = send_recv_tcppkt(connfd);
            if(res != 0){
                printf("send_recv_tcppkt failed\n");
                pctx->failed ++;
                continue;
            }

        }
        close(connfd);
    }
    
    
    return NULL;

}

int connect_tcpserver(const char *ip, unsigned short port)
{
    int connfd = -1;
    connfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in tcpserver_addr;
    memset(&tcpserver_addr, 0, sizeof(tcpserver_addr));
    tcpserver_addr.sin_family = AF_INET;
    tcpserver_addr.sin_addr.s_addr = inet_addr(ip); /*将点分十进制IPV4字符串转换成32位大端网络字节序*/
    tcpserver_addr.sin_port = htons(port);

    int ret = connect(connfd, (struct sockaddr *)&tcpserver_addr, sizeof(struct sockaddr_in));
    if (ret < 0){
        printf("connection failed\n");
        close(connfd);
        return -1;
    }

    return connfd;
}

int send_msg(int connfd, char *msg, int length)
{
    int res = send(connfd, msg, length, 0);
    if(res < 0){
        perror("send failed");
        exit(1);
    }
    return res;
}

int recv_msg(int connfd, char *msg, int length)
{
    int res = recv(connfd, msg, length, 0);
    if(res <= 0){
        perror("recv failed");
        exit(1);
    }
    return res;
}

#define MAX_MSG_LENGTH  512

void equals(char *pattern, char *result, char *casename)
{
    if(strcmp(pattern, result) != 0){
        printf("==> FAILED --> %s\n : '%s' != '%s'\n", casename, pattern, result);
    } else {
        printf("==> PASS --> %s\n", casename);
    }
}

int test_kv_case(int connfd, char *msg, char *pattern, char *casename)
{
    if (!msg || !pattern || !casename) {
        return -1;
    }
    send_msg(connfd, msg, strlen(msg));
    char result[MAX_MSG_LENGTH] = {0};
    recv_msg(connfd, result, MAX_MSG_LENGTH);
    equals(pattern, result, casename);
}

int send_recv_tcppkt(int fd)
{
    int res = send(fd, TEST_MESSAGE, strlen(TEST_MESSAGE), 0);
    if(res < 0){
        exit(1);
    }
    char rbuffer[RBUFFER_LENGTH] = {0};
    res = recv(fd, rbuffer, RBUFFER_LENGTH, 0);
    if(res <= 0){
        exit(1);
    } else {
        //printf("recv : %s\n", rbuffer);
    }
    if(strcmp(rbuffer, TEST_MESSAGE) != 0){
        printf("failed: '%s' != '%s'\n", rbuffer, TEST_MESSAGE);
        return -1;
    }
    return 0;
}

//array: 0x01, rbtree: 0x02, hashtable: 0x04, skiptable: 0x08
//test_qps_tcpclient -s 127.0.0.1 -p 2048 -t 50 -c 100 -n 10000 -m 
int main(int argc, char *argv[])
{
    if (argc < 13) {
        printf("Usage: %s -s <server_ip> -p <port> -t <thread_num> -c <connection_num> -n <request_num> -m mode\n", argv[0]);
        printf("mode: \n");
        printf("\tarray: 0x01, rbtree: 0x02, hashtable: 0x04, skiptable: 0x08\n");
        return -1;
    }
    int ret = 0;
    int opt;
    test_context_t ctx = {0};
    while((opt = getopt(argc, argv, "s:p:t:c:n:m:?")) != -1){
        switch(opt){
            case 's':
                printf("-s : %s\n", optarg);
                strcpy(ctx.serverip, optarg);
                break;
            case 'p':
                printf("-p : %s\n", optarg);
                ctx.port = atoi(optarg);
                break;
            case 't':
                printf("-t : %s\n", optarg);
                ctx.threadnum = atoi(optarg);
                break;
            case 'c':
                printf("-c : %s\n", optarg);
                ctx.connection = atoi(optarg);
                break;
            case 'n':
                printf("-n : %s\n", optarg);
                ctx.requestion = atoi(optarg);
                break;
            case 'm':
                printf("-m : %s\n", optarg);
                ctx.mode = atoi(optarg);
            default:
                printf("getopt error, unknown opt\n");
                exit(1);
        }
    }


    pthread_t *ptid = (pthread_t *)malloc(ctx.threadnum * sizeof(pthread_t));
    int i = 0;

    struct timeval tv_begin;
    gettimeofday(&tv_begin, NULL);
    for (i = 0; i < ctx.threadnum; i++){
        if (pthread_create(&ptid[i], NULL, test_qps_entry, &ctx)) {
            printf("create thread %d failed\n", i);
        } 
    }

    for (i = 0; i < ctx.threadnum; i++){
        pthread_join(ptid[i], NULL);
    }
    
    struct timeval tv_end;
    gettimeofday(&tv_end, NULL);

    int time_used = TIME_SUB_MS(tv_end, tv_begin);
    printf("success : %d, failed : %d, time_used : %d, qps : %d\n", ctx.requestion - ctx.failed, ctx.failed, time_used, ctx.requestion * 1000 / time_used);

clean:
    free(ptid);
    return ret;
}