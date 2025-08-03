#include "kv_store.hpp"

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
    int repeat_num;
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
        printf("==> FAILED --> %s\n : SHOULD BE:'%s' != RESULT:'%s'\n", casename, pattern, result);
    } else {
        //printf("==> PASS --> %s\n", casename);
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

void test_rb_case(int connfd, char *msg, char *pattern, char *casename)
{
    if (!msg || !pattern || !casename) {
        return;
    }
    send_msg(connfd, msg, strlen(msg));
    char result[MAX_MSG_LENGTH] = {0};
    recv_msg(connfd, result, MAX_MSG_LENGTH);
    equals(pattern, result, casename);
}

void test_hash_case(int connfd, char *msg, char *pattern, char *casename)
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
    test_kv_case(connfd, "SET Name Linus", "EXIST", "SETCase");
    test_kv_case(connfd, "MOD Name Linus", "SUCCESS", "MODCase");
    test_kv_case(connfd, "EXIST Name", "EXIST", "EXISTCase");
    test_kv_case(connfd, "GET Name", "Linus", "GETCase");
    test_kv_case(connfd, "DEL Name", "SUCCESS", "DELCase");
    test_kv_case(connfd, "GET Name", "NO EXIST", "GETCase");
    test_kv_case(connfd, "MOD Name Linus", "ERROR", "MODCase");
    test_kv_case(connfd, "EXIST Name", "NO EXIST", "EXISTCase");
}

void array_test_case_huge_keys(int connfd, int num)
{
    int i = 0;
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "SET Name_%d ZZW_%d", i, i);
        // printf("cmd = %s\n", cmd);
        test_kv_case(connfd, cmd, "SUCCESS", "SETCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "GET Name_%d", i);
        // printf("cmd = %s\n", cmd);
        char pattern[128] = {0};
        snprintf(pattern, sizeof(pattern), "ZZW_%d", i);
        test_kv_case(connfd, cmd, pattern, "GETCase");
    }
    for (i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "SET Name_%d Linus_%d", i, i);
        // printf("cmd = %s\n", cmd);
        test_kv_case(connfd, cmd, "EXIST", "SETCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "MOD Name_%d Linus_%d", i, i);
        // printf("cmd = %s\n", cmd);
        test_kv_case(connfd, cmd, "SUCCESS", "MODCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "EXIST Name_%d", i);
        // printf("cmd = %s\n", cmd);
        test_kv_case(connfd, cmd, "EXIST", "EXISTCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "GET Name_%d", i);
        // printf("cmd = %s\n", cmd);
        char pattern[128] = {0};
        snprintf(pattern, sizeof(pattern), "Linus_%d", i);
        test_kv_case(connfd, cmd, pattern, "GETCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "DEL Name_%d", i);
        // printf("cmd = %s\n", cmd);
        test_kv_case(connfd, cmd, "SUCCESS", "DELCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "GET Name_%d", i);
        // printf("cmd = %s\n", cmd);
        test_kv_case(connfd, cmd, "NO EXIST", "GETCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "MOD Name_%d Linus_%d", i, i);
        // printf("cmd = %s\n", cmd);
        char case_str[128] = {0};
        snprintf(case_str, sizeof(case_str), "MODCase_%d", i);
        test_kv_case(connfd, cmd, "ERROR", case_str);
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "EXIST Name_%d", i);
        // printf("cmd = %s\n", cmd);
        test_kv_case(connfd, cmd, "NO EXIST", "EXISTCase");
    }
}

void rbtree_test_case(int connfd)
{
    test_rb_case(connfd, "RSET Name ZZW", "SUCCESS", "RSETCase");
    test_rb_case(connfd, "RGET Name", "ZZW", "RGETCase");
    test_rb_case(connfd, "RSET Name Linus", "EXIST", "RSETCase");
    test_rb_case(connfd, "RMOD Name Linus", "SUCCESS", "RMODCase");
    test_rb_case(connfd, "REXIST Name", "EXIST", "REXISTCase");
    test_rb_case(connfd, "RGET Name", "Linus", "RGETCase");
    test_rb_case(connfd, "RDEL Name", "SUCCESS", "RDELCase");
    test_rb_case(connfd, "RGET Name", "NO EXIST", "RGETCase");
    test_rb_case(connfd, "RMOD Name Linus", "ERROR", "RMODCase");
    test_rb_case(connfd, "REXIST Name", "NO EXIST", "REXISTCase");
}

void rbtree_test_case_huge_keys(int connfd, int num)
{
    int i = 0;
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "RSET Name_%d ZZW_%d", i, i);
        test_rb_case(connfd, cmd, "SUCCESS", "RSETCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "RGET Name_%d", i);
        char pattern[128] = {0};
        snprintf(pattern, sizeof(pattern), "ZZW_%d", i);
        test_rb_case(connfd, cmd, pattern, "RGETCase");
    }
    for (i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "RSET Name_%d Linus_%d", i, i);
        test_rb_case(connfd, cmd, "EXIST", "RSETCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "RMOD Name_%d Linus_%d", i, i);
        test_rb_case(connfd, cmd, "SUCCESS", "RMODCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "REXIST Name_%d", i);
        test_rb_case(connfd, cmd, "EXIST", "REXISTCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "RGET Name_%d", i);
        char pattern[128] = {0};
        snprintf(pattern, sizeof(pattern), "Linus_%d", i);
        test_rb_case(connfd, cmd, pattern, "RGETCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "RDEL Name_%d", i);
        test_rb_case(connfd, cmd, "SUCCESS", "RDELCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "RGET Name_%d", i);
        test_rb_case(connfd, cmd, "NO EXIST", "RGETCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "RMOD Name_%d Linus_%d", i, i);
        test_rb_case(connfd, cmd, "ERROR", "RMODCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "REXIST Name_%d", i);
        test_rb_case(connfd, cmd, "NO EXIST", "REXISTCase");
    }
}

void hash_test_case(int connfd)
{
    test_hash_case(connfd, "HSET Name ZZW", "SUCCESS", "HSETCase");
    test_hash_case(connfd, "HGET Name", "ZZW", "HGETCase");
    test_hash_case(connfd, "HSET Name Linus", "EXIST", "HSETCase");
    test_hash_case(connfd, "HMOD Name Linus", "SUCCESS", "HMODCase");
    test_hash_case(connfd, "HEXIST Name", "EXIST", "HEXISTCase");
    test_hash_case(connfd, "HGET Name", "Linus", "HGETCase");
    test_hash_case(connfd, "HDEL Name", "SUCCESS", "HDELCase");
    test_hash_case(connfd, "HGET Name", "NO EXIST", "HGETCase");
    test_hash_case(connfd, "HMOD Name Linus", "ERROR", "HMODCase");
    test_hash_case(connfd, "HEXIST Name", "NO EXIST", "HEXISTCase");
}

void hash_test_case_huge_keys(int connfd, int num)
{
    int i = 0;
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "HSET Name_%d ZZW_%d", i, i);
        test_rb_case(connfd, cmd, "SUCCESS", "HSETCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "HGET Name_%d", i);
        char pattern[128] = {0};
        snprintf(pattern, sizeof(pattern), "ZZW_%d", i);
        test_rb_case(connfd, cmd, pattern, "HGETCase");
    }
    for (i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "HSET Name_%d Linus_%d", i, i);
        test_rb_case(connfd, cmd, "EXIST", "HSETCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "HMOD Name_%d Linus_%d", i, i);
        test_rb_case(connfd, cmd, "SUCCESS", "HMODCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "HEXIST Name_%d", i);
        test_rb_case(connfd, cmd, "EXIST", "HEXISTCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "HGET Name_%d", i);
        char pattern[128] = {0};
        snprintf(pattern, sizeof(pattern), "Linus_%d", i);
        test_rb_case(connfd, cmd, pattern, "HGETCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "HDEL Name_%d", i);
        test_rb_case(connfd, cmd, "SUCCESS", "HDELCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "HGET Name_%d", i);
        test_rb_case(connfd, cmd, "NO EXIST", "HGETCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "HMOD Name_%d Linus_%d", i, i);
        test_rb_case(connfd, cmd, "ERROR", "HMODCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "HEXIST Name_%d", i);
        test_rb_case(connfd, cmd, "NO EXIST", "HEXISTCase");
    }
}

// void array_test_case_1000k(int connfd)
// {
//     for(int i = 0; i < 1000000; i++){
//         array_test_case(connfd);
//     }
// }

// 测试数组
void skiptable_test_case(int connfd)
{
    // 测试SET命令
    test_kv_case(connfd, "SSET Name ZZW", "SUCCESS", "SSETCase");
    // 测试GET命令
    test_kv_case(connfd, "SGET Name", "ZZW", "SGETCase");
    // 测试SET命令，键已存在
    test_kv_case(connfd, "SSET Name Linus", "EXIST", "SSETCase");
    // 测试MOD命令
    test_kv_case(connfd, "SMOD Name Linus", "SUCCESS", "SMODCase");
    // 测试EXIST命令
    test_kv_case(connfd, "SEXIST Name", "EXIST", "SEXISTCase");
    // 测试GET命令
    test_kv_case(connfd, "SGET Name", "Linus", "SGETCase");
    // 测试DEL命令
    test_kv_case(connfd, "SDEL Name", "SUCCESS", "SDELCase");
    // 测试GET命令，键不存在
    test_kv_case(connfd, "SGET Name", "NO EXIST", "SGETCase");
    // 测试MOD命令，键不存在
    test_kv_case(connfd, "SMOD Name Linus", "ERROR", "SMODCase");
    test_kv_case(connfd, "SEXIST Name", "NO EXIST", "SEXISTCase");
}

void skiptable_test_case_huge_keys(int connfd, int num)
{
    int i = 0;
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "SSET Name_%d ZZW_%d", i, i);
        test_kv_case(connfd, cmd, "SUCCESS", "SSETCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "SGET Name_%d", i);
        char pattern[128] = {0};
        snprintf(pattern, sizeof(pattern), "ZZW_%d", i);
        test_kv_case(connfd, cmd, pattern, "SGETCase");
    }
    for (i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "SSET Name_%d Linus_%d", i, i);
        test_kv_case(connfd, cmd, "EXIST", "SSETCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "SMOD Name_%d Linus_%d", i, i);
        test_kv_case(connfd, cmd, "SUCCESS", "SMODCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "SEXIST Name_%d", i);
        test_kv_case(connfd, cmd, "EXIST", "SEXISTCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "SGET Name_%d", i);
        char pattern[128] = {0};
        snprintf(pattern, sizeof(pattern), "Linus_%d", i);
        test_kv_case(connfd, cmd, pattern, "SGETCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "SDEL Name_%d", i);
        test_kv_case(connfd, cmd, "SUCCESS", "SDELCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "SGET Name_%d", i);
        test_kv_case(connfd, cmd, "NO EXIST", "SGETCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "SMOD Name_%d Linus_%d", i, i);
        test_kv_case(connfd, cmd, "ERROR", "SMODCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "SEXIST Name_%d", i);
        test_kv_case(connfd, cmd, "NO EXIST", "SEXISTCase");
    }
}

//array: 1, rbtree: 2, hashtable: 3, skiptable: 4
//array_huge_keys: 5, rbtree_huge_keys: 6, hashtable_huge_keys: 7, skiptable_huge_keys: 8
//test_qps_tcpclient -s 127.0.0.1 -p 2048 -m 
int main(int argc, char *argv[])
{
    if (argc < 8) {
        printf("Usage: %s -s <server_ip> -p <port> -m <mode> -n <repeat_num>\n", argv[0]);
        printf("mode: \n");
        printf("\tarray: 1, rbtree: 2, hashtable: 3, skiptable: 4, array_huge_keys: 5, rbtree_huge_keys: 6, hashtable_huge_keys: 7, skiptable_huge_keys: 8\n");
        return -1;
    }
    int ret = 0;
    int opt;
    test_context_t ctx = {0};
    while((opt = getopt(argc, argv, "s:p:m:n:?")) != -1){
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
            case 'n':
                printf("-n : %s\n", optarg);
                ctx.repeat_num = atoi(optarg);
                break;
            default:
                printf("getopt error, unknown opt\n");
                exit(1);
        }
    }

    int connfd = connect_tcpserver(ctx.serverip, ctx.port);

    struct timeval start, end;
    gettimeofday(&start, NULL);
    if (1 == ctx.mode) {
        printf("array test case\n");
        for(int i = 0; i < ctx.repeat_num; i++){
        #if ENABLE_ARRAY_KV_ENGINE
            array_test_case(connfd);
        #endif
        }
    }
    if (2 == ctx.mode) {
        printf("rbtree test case\n");
        for(int i = 0; i < ctx.repeat_num; i++){
        #if ENABLE_RBTREE_KV_ENGINE
            rbtree_test_case(connfd);
        #endif
        }
    }
    if (3 == ctx.mode) {
        printf("hashtable test case\n");
        for(int i = 0; i < ctx.repeat_num; i++){
        #if ENABLE_HASH_KV_ENGINE
            hash_test_case(connfd);
        #endif
        }
    }
    if (4 == ctx.mode) {
        printf("skiptable test case\n");
        for(int i = 0; i < ctx.repeat_num; i++){
        #if ENABLE_SKIPTABLE_KV_ENGINE
            skiptable_test_case(connfd);
        #endif
        }
    }
    if (5 == ctx.mode) {
        printf("array huge keys test case\n");
    #if ENABLE_ARRAY_KV_ENGINE
        array_test_case_huge_keys(connfd, ctx.repeat_num);
    #endif
    }
    if (6 == ctx.mode) {
        printf("rbtree huge keys test case\n");
    #if ENABLE_RBTREE_KV_ENGINE
        rbtree_test_case_huge_keys(connfd, ctx.repeat_num);
    #endif
    }
    if (7 == ctx.mode) {
        printf("hashtable huge keys test case\n");
    #if ENABLE_HASH_KV_ENGINE
        hash_test_case_huge_keys(connfd, ctx.repeat_num);
    #endif
    }
    if (8 == ctx.mode) {
        printf("skiptable huge keys test case\n");
    #if ENABLE_SKIPTABLE_KV_ENGINE
        skiptable_test_case_huge_keys(connfd, ctx.repeat_num);
    #endif
    }
    gettimeofday(&end, NULL);

    int time_used = TIME_SUB_MS(end, start);
    printf("time: %dms, qps: %d\n", time_used, ctx.repeat_num * 1000 / time_used);

}