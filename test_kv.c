#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>

typedef struct test_context_s{
    char serverip[16];
    int port;
    int mode;

#if 1
    int failed;
#endif

} test_context_t;

#define TIME_SUB_MS(tv1, tv2) ((tv1.tv_sec - tv2.tv_sec) * 1000 + (tv1.tv_usec - tv2.tv_usec) / 1000)

#define RBUFFER_LENGTH    128

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

void test_kv_case(int connfd, char *msg, char *pattern, char *casename)
{
    if (!msg || !pattern || !casename) {
        return;
    }
    send_msg(connfd, msg, strlen(msg));
    char result[MAX_MSG_LENGTH] = {0};
    recv_msg(connfd, result, MAX_MSG_LENGTH);
    equals(pattern, result, casename);
}

void array_test_case(int connfd)
{
    test_kv_case(connfd, "SET Name ZZW", "SUCCESS", "SETCase");
    test_kv_case(connfd, "GET Name", "ZZW", "GETCase");
    test_kv_case(connfd, "MOD Name Linus", "SUCCESS", "MODCase");
    test_kv_case(connfd, "GET Name", "Linus", "GETCase");
    test_kv_case(connfd, "DEL Name", "SUCCESS", "DELCase");
    test_kv_case(connfd, "GET Name", "NO EXIST", "GETCase");
}

//array: 0x01, rbtree: 0x02, hashtable: 0x04, skiptable: 0x08
//test_qps_tcpclient -s 127.0.0.1 -p 2048 -m 
int main(int argc, char *argv[])
{
    if (argc < 7) {
        printf("Usage: %s -s <server_ip> -p <port> -m <mode>\n", argv[0]);
        printf("mode: \n");
        printf("\tarray: 0x01, rbtree: 0x02, hashtable: 0x04, skiptable: 0x08\n");
        return -1;
    }
    int ret = 0;
    int opt;
    test_context_t ctx = {0};
    while((opt = getopt(argc, argv, "s:p:m:?")) != -1){
        switch(opt){
            case 's':
                printf("-s : %s\n", optarg);
                strcpy(ctx.serverip, optarg);
                break;
            case 'p':
                printf("-p : %s\n", optarg);
                ctx.port = atoi(optarg);
                break;
            case 'm':
                printf("-m : %s\n", optarg);
                ctx.mode = atoi(optarg);
                break;
            default:
                printf("getopt error, unknown opt\n");
                exit(1);
        }
    }

    int connfd = connect_tcpserver(ctx.serverip, ctx.port);

    if (ctx.mode & 0x01) {
        array_test_case(connfd);
    }

}