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
    char filename[128];
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

#define MAX_MSG_LENGTH  4096

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
    test_kv_case(connfd, "SET Name ZZW\n", "SUCCESS\n", "SETCase");
    test_kv_case(connfd, "GET Name\n", "ZZW\n", "GETCase");
    test_kv_case(connfd, "SET Name Linus\n", "EXIST\n", "SETCase");
    test_kv_case(connfd, "MOD Name Linus\n", "SUCCESS\n", "MODCase");
    test_kv_case(connfd, "EXIST Name\n", "EXIST\n", "EXISTCase");
    test_kv_case(connfd, "GET Name\n", "Linus\n", "GETCase");
    test_kv_case(connfd, "DEL Name\n", "SUCCESS\n", "DELCase");
    test_kv_case(connfd, "GET Name\n", "NO EXIST\n", "GETCase");
    test_kv_case(connfd, "MOD Name Linus\n", "ERROR\n", "MODCase");
    test_kv_case(connfd, "EXIST Name\n", "NO EXIST\n", "EXISTCase");
}

void array_test_case_huge_keys(int connfd, int num)
{
    int i = 0;
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "SET Name_%d ZZW_%d\n", i, i);
        // printf("cmd = %s\n", cmd);
        test_kv_case(connfd, cmd, "SUCCESS\n", "SETCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "GET Name_%d\n", i);
        // printf("cmd = %s\n", cmd);
        char pattern[128] = {0};
        snprintf(pattern, sizeof(pattern), "ZZW_%d\n", i);
        test_kv_case(connfd, cmd, pattern, "GETCase");
    }
    for (i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "SET Name_%d Linus_%d\n", i, i);
        // printf("cmd = %s\n", cmd);
        test_kv_case(connfd, cmd, "EXIST\n", "SETCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "MOD Name_%d Linus_%d\n", i, i);
        // printf("cmd = %s\n", cmd);
        test_kv_case(connfd, cmd, "SUCCESS\n", "MODCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "EXIST Name_%d\n", i);
        // printf("cmd = %s\n", cmd);
        test_kv_case(connfd, cmd, "EXIST\n", "EXISTCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "GET Name_%d\n", i);
        // printf("cmd = %s\n", cmd);
        char pattern[128] = {0};
        snprintf(pattern, sizeof(pattern), "Linus_%d\n", i);
        test_kv_case(connfd, cmd, pattern, "GETCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "DEL Name_%d\n", i);
        // printf("cmd = %s\n", cmd);
        test_kv_case(connfd, cmd, "SUCCESS\n", "DELCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "GET Name_%d\n", i);
        // printf("cmd = %s\n", cmd);
        test_kv_case(connfd, cmd, "NO EXIST\n", "GETCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "MOD Name_%d Linus_%d\n", i, i);
        // printf("cmd = %s\n", cmd);
        char case_str[128] = {0};
        snprintf(case_str, sizeof(case_str), "MODCase_%d\n", i);
        test_kv_case(connfd, cmd, "ERROR\n", case_str);
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "EXIST Name_%d\n", i);
        // printf("cmd = %s\n", cmd);
        test_kv_case(connfd, cmd, "NO EXIST\n", "EXISTCase");
    }
}

void array_save_test(int connfd, int num, char* filename)
{
    int i = 0;
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "SET Name_%d ZZW_%d\n", i, i);
        // printf("cmd = %s\n", cmd);
        test_kv_case(connfd, cmd, "SUCCESS\n", "SETCase");
    }
    char cmd[128] = {0};
    snprintf(cmd, sizeof(cmd), "SAVE %s\n", filename);
    test_kv_case(connfd, cmd, "SUCCESS\n", "SAVECase");
}

void array_load_test(int connfd, int num, char* filename)
{
    char cmd[128] = {0};
    for(int i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "GET Name_%d\n", i);
        test_kv_case(connfd, cmd, "NO EXIST\n", "GETCase");
    }
    snprintf(cmd, sizeof(cmd), "LOAD %s\n", filename);
    test_kv_case(connfd, cmd, "SUCCESS\n", "LOADCase");
    for(int i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "GET Name_%d\n", i);
        char pattern[128] = {0};
        snprintf(pattern, sizeof(pattern), "ZZW_%d\n", i);
        test_kv_case(connfd, cmd, pattern, "GETCase");
    }

}

void array_range_test(int connfd, int start_range, int end_range, int num)
{
    if (start_range > end_range) {
        printf("array range test failed, start_range > end_range\n");
        return;
    }
    if (num <= 0) {
        printf("array range test failed, num <= 0\n");
        return;
    }
    int i = 0;
    char cmd[512] = {0};
    snprintf(cmd, sizeof(cmd), "RANGE Name_%09d Name_%09d\n", start_range, end_range);
    test_kv_case(connfd, cmd, "EMPTY\n", "RANGECase with no SET");
    for (i = 0; i <= num; i++) {
        memset(cmd, 0, sizeof(cmd));
        snprintf(cmd, sizeof(cmd), "SET Name_%09d ZZW_%09d\n", i, i);
        test_kv_case(connfd, cmd, "SUCCESS\n", "SETCase");
    }
    memset(cmd, 0, sizeof(cmd));
    snprintf(cmd, sizeof(cmd), "RANGE Name_%09d Name_%09d\n", start_range, end_range);
    char pattern[4096] = { 0 };
    if (end_range > num && start_range < num) {
        //printf("end range > num\n");
        int bytes_written = 0;
        for (int i = start_range; i <= num; i++) {
            char buffer[128] = { 0 };
            snprintf(buffer, sizeof(buffer),  "Name_%09d ZZW_%09d\n", i, i);
            strncpy(pattern + bytes_written, buffer, strlen(buffer));
            bytes_written += strlen(buffer);
        }
        
    } else if (start_range > num) {
        snprintf(pattern, sizeof(pattern), "EMPTY\n");
    } else {
        int bytes_written = 0;
        for (int i = start_range; i <= end_range; i++) {
            char buffer[128] = { 0 };
            snprintf(buffer, sizeof(buffer), "Name_%09d ZZW_%09d\n", i, i);
            strncpy(pattern + bytes_written, buffer, strlen(buffer));
            bytes_written += strlen(buffer);
        }
    }
    //printf("pattern : %s\n", pattern);
    test_kv_case(connfd, cmd, pattern, "RANGECase");
    
}

void rbtree_test_case(int connfd)
{
    test_rb_case(connfd, "RSET Name ZZW\n", "SUCCESS\n", "RSETCase");
    test_rb_case(connfd, "RGET Name\n", "ZZW\n", "RGETCase");
    test_rb_case(connfd, "RSET Name Linus\n", "EXIST\n", "RSETCase");
    test_rb_case(connfd, "RMOD Name Linus\n", "SUCCESS\n", "RMODCase");
    test_rb_case(connfd, "REXIST Name\n", "EXIST\n", "REXISTCase");
    test_rb_case(connfd, "RGET Name\n", "Linus\n", "RGETCase");
    test_rb_case(connfd, "RDEL Name\n", "SUCCESS\n", "RDELCase");
    test_rb_case(connfd, "RGET Name\n", "NO EXIST\n", "RGETCase");
    test_rb_case(connfd, "RMOD Name Linus\n", "ERROR\n", "RMODCase");
    test_rb_case(connfd, "REXIST Name\n", "NO EXIST\n", "REXISTCase");
}

void rbtree_test_case_huge_keys(int connfd, int num)
{
    int i = 0;
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "RSET Name_%d ZZW_%d\n", i, i);
        test_rb_case(connfd, cmd, "SUCCESS\n", "RSETCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "RGET Name_%d\n", i);
        char pattern[128] = {0};
        snprintf(pattern, sizeof(pattern), "ZZW_%d\n", i);
        test_rb_case(connfd, cmd, pattern, "RGETCase");
    }
    for (i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "RSET Name_%d Linus_%d\n", i, i);
        test_rb_case(connfd, cmd, "EXIST\n", "RSETCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "RMOD Name_%d Linus_%d\n", i, i);
        test_rb_case(connfd, cmd, "SUCCESS\n", "RMODCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "REXIST Name_%d\n", i);
        test_rb_case(connfd, cmd, "EXIST\n", "REXISTCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "RGET Name_%d\n", i);
        char pattern[128] = {0};
        snprintf(pattern, sizeof(pattern), "Linus_%d\n", i);
        test_rb_case(connfd, cmd, pattern, "RGETCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "RDEL Name_%d\n", i);
        test_rb_case(connfd, cmd, "SUCCESS\n", "RDELCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "RGET Name_%d\n", i);
        test_rb_case(connfd, cmd, "NO EXIST\n", "RGETCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "RMOD Name_%d Linus_%d\n", i, i);
        test_rb_case(connfd, cmd, "ERROR\n", "RMODCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "REXIST Name_%d\n", i);
        test_rb_case(connfd, cmd, "NO EXIST\n", "REXISTCase");
    }
}

void rbtree_save_test(int connfd, int num, char* filename)
{
    int i = 0;
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "RSET Name_%d ZZW_%d\n", i, i);
        // printf("cmd = %s\n", cmd);
        test_kv_case(connfd, cmd, "SUCCESS\n", "RSETCase");
    }
    char cmd[128] = {0};
    snprintf(cmd, sizeof(cmd), "RSAVE %s\n", filename);
    test_kv_case(connfd, cmd, "SUCCESS\n", "RSAVECase");
}

void rbtree_load_test(int connfd, int num, char* filename)
{
    char cmd[128] = {0};
    for(int i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "RGET Name_%d\n", i);
        test_kv_case(connfd, cmd, "NO EXIST\n", "RGETCase");
    }
    snprintf(cmd, sizeof(cmd), "RLOAD %s\n", filename);
    test_kv_case(connfd, cmd, "SUCCESS\n", "RLOADCase");
    for(int i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "RGET Name_%d\n", i);
        char pattern[128] = {0};
        snprintf(pattern, sizeof(pattern), "ZZW_%d\n", i);
        test_kv_case(connfd, cmd, pattern, "RGETCase");
    }
}

void rbtree_range_test(int connfd, int start_range, int end_range, int num)
{
    if (start_range > end_range) {
        printf("array range test failed, start_range > end_range\n");
        return;
    }
    if (num <= 0) {
        printf("array range test failed, num <= 0\n");
        return;
    }
    int i = 0;
    char cmd[512] = { 0 };
    snprintf(cmd, sizeof(cmd), "RRANGE Name_%09d Name_%09d\n", start_range, end_range);
    test_kv_case(connfd, cmd, "EMPTY\n", "RRANGECase with no SET");
    for (i = 0; i <= num; i++) {
        memset(cmd, 0, sizeof(cmd));
        snprintf(cmd, sizeof(cmd), "RSET Name_%09d ZZW_%09d\n", i, i);
        test_kv_case(connfd, cmd, "SUCCESS\n", "RSETCase");
    }
    memset(cmd, 0, sizeof(cmd));
    snprintf(cmd, sizeof(cmd), "RRANGE Name_%09d Name_%09d\n", start_range, end_range);
    char pattern[4096] = { 0 };
    if (end_range > num && start_range < num) {
        //printf("end range > num\n");
        int bytes_written = 0;
        for (int i = start_range; i <= num; i++) {
            char buffer[128] = { 0 };
            snprintf(buffer, sizeof(buffer), "Name_%09d ZZW_%09d\n", i, i);
            strncpy(pattern + bytes_written, buffer, strlen(buffer));
            bytes_written += strlen(buffer);
        }

    }
    else if (start_range > num) {
        snprintf(pattern, sizeof(pattern), "EMPTY\n");
    }
    else {
        int bytes_written = 0;
        for (int i = start_range; i <= end_range; i++) {
            char buffer[128] = { 0 };
            snprintf(buffer, sizeof(buffer), "Name_%09d ZZW_%09d\n", i, i);
            strncpy(pattern + bytes_written, buffer, strlen(buffer));
            bytes_written += strlen(buffer);
        }
    }
    //printf("pattern : %s\n", pattern);
    test_kv_case(connfd, cmd, pattern, "RRANGECase");

}

void hash_test_case(int connfd)
{
    test_hash_case(connfd, "HSET Name ZZW\n", "SUCCESS\n", "HSETCase");
    test_hash_case(connfd, "HGET Name\n", "ZZW\n", "HGETCase");
    test_hash_case(connfd, "HSET Name Linus\n", "EXIST\n", "HSETCase");
    test_hash_case(connfd, "HMOD Name Linus\n", "SUCCESS\n", "HMODCase");
    test_hash_case(connfd, "HEXIST Name\n", "EXIST\n", "HEXISTCase");
    test_hash_case(connfd, "HGET Name\n", "Linus\n", "HGETCase");
    test_hash_case(connfd, "HDEL Name\n", "SUCCESS\n", "HDELCase");
    test_hash_case(connfd, "HGET Name\n", "NO EXIST\n", "HGETCase");
    test_hash_case(connfd, "HMOD Name Linus\n", "ERROR\n", "HMODCase");
    test_hash_case(connfd, "HEXIST Name\n", "NO EXIST\n", "HEXISTCase");
}

void hash_test_case_huge_keys(int connfd, int num)
{
    int i = 0;
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "HSET Name_%d ZZW_%d\n", i, i);
        test_rb_case(connfd, cmd, "SUCCESS\n", "HSETCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "HGET Name_%d\n", i);
        char pattern[128] = {0};
        snprintf(pattern, sizeof(pattern), "ZZW_%d\n", i);
        test_rb_case(connfd, cmd, pattern, "HGETCase");
    }
    for (i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "HSET Name_%d Linus_%d\n", i, i);
        test_rb_case(connfd, cmd, "EXIST\n", "HSETCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "HMOD Name_%d Linus_%d\n", i, i);
        test_rb_case(connfd, cmd, "SUCCESS\n", "HMODCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "HEXIST Name_%d\n", i);
        test_rb_case(connfd, cmd, "EXIST\n", "HEXISTCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "HGET Name_%d\n", i);
        char pattern[128] = {0};
        snprintf(pattern, sizeof(pattern), "Linus_%d\n", i);
        test_rb_case(connfd, cmd, pattern, "HGETCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "HDEL Name_%d\n", i);
        test_rb_case(connfd, cmd, "SUCCESS\n", "HDELCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "HGET Name_%d\n", i);
        test_rb_case(connfd, cmd, "NO EXIST\n", "HGETCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "HMOD Name_%d Linus_%d\n", i, i);
        test_rb_case(connfd, cmd, "ERROR\n", "HMODCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "HEXIST Name_%d\n", i);
        test_rb_case(connfd, cmd, "NO EXIST\n", "HEXISTCase");
    }
}

void hash_save_test(int connfd, int num, char* filename)
{
    int i = 0;
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "HSET Name_%d ZZW_%d\n", i, i);
        // printf("cmd = %s\n", cmd);
        test_kv_case(connfd, cmd, "SUCCESS\n", "HSETCase");
    }
    char cmd[128] = {0};
    snprintf(cmd, sizeof(cmd), "HSAVE %s\n", filename);
    test_kv_case(connfd, cmd, "SUCCESS\n", "HSAVECase");
}

void hash_load_test(int connfd, int num, char* filename)
{
    char cmd[128] = {0};
    for(int i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "HGET Name_%d\n", i);
        test_kv_case(connfd, cmd, "NO EXIST\n", "HGETCase");
    }
    snprintf(cmd, sizeof(cmd), "HLOAD %s\n", filename);
    test_kv_case(connfd, cmd, "SUCCESS\n", "HLOADCase");
    for(int i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "HGET Name_%d\n", i);
        char pattern[128] = {0};
        snprintf(pattern, sizeof(pattern), "ZZW_%d\n", i);
        test_kv_case(connfd, cmd, pattern, "HGETCase");
    }

}

void hash_range_test(int connfd, int start_range, int end_range, int num)
{
    if (start_range > end_range) {
        printf("hash range test failed, start_range > end_range\n");
        return;
    }
    if (num <= 0) {
        printf("hash range test failed, num <= 0\n");
        return;
    }
    int i = 0;
    char cmd[512] = { 0 };
    snprintf(cmd, sizeof(cmd), "HRANGE Name_%09d Name_%09d\n", start_range, end_range);
    test_kv_case(connfd, cmd, "EMPTY\n", "HRANGECase with no SET");
    for (i = 0; i <= num; i++) {
        memset(cmd, 0, sizeof(cmd));
        snprintf(cmd, sizeof(cmd), "HSET Name_%09d ZZW_%09d\n", i, i);
        test_kv_case(connfd, cmd, "SUCCESS\n", "HSETCase");
    }
    memset(cmd, 0, sizeof(cmd));
    snprintf(cmd, sizeof(cmd), "HRANGE Name_%09d Name_%09d\n", start_range, end_range);
    char pattern[4096] = { 0 };
    if (end_range > num && start_range < num) {
        //printf("end range > num\n");
        int bytes_written = 0;
        for (int i = start_range; i <= num; i++) {
            char buffer[128] = { 0 };
            snprintf(buffer, sizeof(buffer), "Name_%09d ZZW_%09d\n", i, i);
            strncpy(pattern + bytes_written, buffer, strlen(buffer));
            bytes_written += strlen(buffer);
        }

    }
    else if (start_range > num) {
        snprintf(pattern, sizeof(pattern), "EMPTY\n");
    }
    else {
        int bytes_written = 0;
        for (int i = start_range; i <= end_range; i++) {
            char buffer[128] = { 0 };
            snprintf(buffer, sizeof(buffer), "Name_%09d ZZW_%09d\n", i, i);
            strncpy(pattern + bytes_written, buffer, strlen(buffer));
            bytes_written += strlen(buffer);
        }
    }
    //printf("pattern : %s\n", pattern);
    test_kv_case(connfd, cmd, pattern, "HRANGECase");

}

void skiptable_test_case(int connfd)
{
    test_kv_case(connfd, "SSET Name ZZW\n", "SUCCESS\n", "SSETCase");
    test_kv_case(connfd, "SGET Name\n", "ZZW\n", "SGETCase");
    test_kv_case(connfd, "SSET Name Linus\n", "EXIST\n", "SSETCase");
    test_kv_case(connfd, "SMOD Name Linus\n", "SUCCESS\n", "SMODCase");
    test_kv_case(connfd, "SEXIST Name\n", "EXIST\n", "SEXISTCase");
    test_kv_case(connfd, "SGET Name\n", "Linus\n", "SGETCase");
    test_kv_case(connfd, "SDEL Name\n", "SUCCESS\n", "SDELCase");
    test_kv_case(connfd, "SGET Name\n", "NO EXIST\n", "SGETCase");
    test_kv_case(connfd, "SMOD Name Linus\n", "ERROR\n", "SMODCase");
    test_kv_case(connfd, "SEXIST Name\n", "NO EXIST\n", "SEXISTCase");
}

void skiptable_test_case_huge_keys(int connfd, int num)
{
    int i = 0;
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "SSET Name_%d ZZW_%d\n", i, i);
        test_kv_case(connfd, cmd, "SUCCESS\n", "SSETCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "SGET Name_%d\n", i);
        char pattern[128] = {0};
        snprintf(pattern, sizeof(pattern), "ZZW_%d\n", i);
        test_kv_case(connfd, cmd, pattern, "SGETCase");
    }
    for (i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "SSET Name_%d Linus_%d\n", i, i);
        test_kv_case(connfd, cmd, "EXIST\n", "SSETCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "SMOD Name_%d Linus_%d\n", i, i);
        test_kv_case(connfd, cmd, "SUCCESS\n", "SMODCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "SEXIST Name_%d\n", i);
        test_kv_case(connfd, cmd, "EXIST\n", "SEXISTCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "SGET Name_%d\n", i);
        char pattern[128] = {0};
        snprintf(pattern, sizeof(pattern), "Linus_%d\n", i);
        test_kv_case(connfd, cmd, pattern, "SGETCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "SDEL Name_%d\n", i);
        test_kv_case(connfd, cmd, "SUCCESS\n", "SDELCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "SGET Name_%d\n", i);
        test_kv_case(connfd, cmd, "NO EXIST\n", "SGETCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "SMOD Name_%d Linus_%d\n", i, i);
        test_kv_case(connfd, cmd, "ERROR\n", "SMODCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "SEXIST Name_%d\n", i);
        test_kv_case(connfd, cmd, "NO EXIST\n", "SEXISTCase");
    }
}

void skiptable_save_test(int connfd, int num, char* filename)
{
    int i = 0;
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "SSET Name_%d ZZW_%d\n", i, i);
        // printf("cmd = %s\n", cmd);
        test_kv_case(connfd, cmd, "SUCCESS\n", "SSETCase");
    }
    char cmd[128] = {0};
    snprintf(cmd, sizeof(cmd), "SSAVE %s\n", filename);
    test_kv_case(connfd, cmd, "SUCCESS\n", "SSAVECase");
}

void skiptable_load_test(int connfd, int num, char* filename)
{
    char cmd[128] = {0};
    for(int i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "SGET Name_%d\n", i);
        test_kv_case(connfd, cmd, "NO EXIST\n", "SGETCase");
    }
    snprintf(cmd, sizeof(cmd), "SLOAD %s\n", filename);
    test_kv_case(connfd, cmd, "SUCCESS\n", "SLOADCase");
    for(int i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "SGET Name_%d\n", i);
        char pattern[128] = {0};
        snprintf(pattern, sizeof(pattern), "ZZW_%d\n", i);
        test_kv_case(connfd, cmd, pattern, "SGETCase");
    }

}

void skiptable_range_test(int connfd, int start_range, int end_range, int num)
{
    if (start_range > end_range) {
        printf("skiptable range test failed, start_range > end_range\n");
        return;
    }
    if (num <= 0) {
        printf("skiptable range test failed, num <= 0\n");
        return;
    }
    int i = 0;
    char cmd[512] = { 0 };
    snprintf(cmd, sizeof(cmd), "SRANGE Name_%09d Name_%09d\n", start_range, end_range);
    test_kv_case(connfd, cmd, "EMPTY\n", "SRANGECase with no SET");
    for (i = 0; i <= num; i++) {
        memset(cmd, 0, sizeof(cmd));
        snprintf(cmd, sizeof(cmd), "SSET Name_%09d ZZW_%09d\n", i, i);
        test_kv_case(connfd, cmd, "SUCCESS\n", "SSETCase");
    }
    memset(cmd, 0, sizeof(cmd));
    snprintf(cmd, sizeof(cmd), "SRANGE Name_%09d Name_%09d\n", start_range, end_range);
    char pattern[4096] = { 0 };
    if (end_range > num && start_range < num) {
        //printf("end range > num\n");
        int bytes_written = 0;
        for (int i = start_range; i <= num; i++) {
            char buffer[128] = { 0 };
            snprintf(buffer, sizeof(buffer), "Name_%09d ZZW_%09d\n", i, i);
            strncpy(pattern + bytes_written, buffer, strlen(buffer));
            bytes_written += strlen(buffer);
        }

    }
    else if (start_range > num) {
        snprintf(pattern, sizeof(pattern), "EMPTY\n");
    }
    else {
        int bytes_written = 0;
        for (int i = start_range; i <= end_range; i++) {
            char buffer[128] = { 0 };
            snprintf(buffer, sizeof(buffer), "Name_%09d ZZW_%09d\n", i, i);
            strncpy(pattern + bytes_written, buffer, strlen(buffer));
            bytes_written += strlen(buffer);
        }
    }
    //printf("pattern : %s\n", pattern);
    test_kv_case(connfd, cmd, pattern, "SRANGECase");

}

void array_multiple_commands_test(int connfd, int num)
{
    if (!ENABLE_ARRAY_KV_ENGINE) {
        printf("Error: Array engine is not enabled.\n");
        return;
    }
    for(int i = 0; i < num; i++){
        char cmd[512] = {0};
        snprintf(cmd, sizeof(cmd), "SET Name_%d ZZW_%d\n\
                                    GET Name_%d\n\
                                    SET Name_%d Linus_%d\n\
                                    MOD Name_%d Linus_%d\n\
                                    EXIST Name_%d\n\
                                    GET Name_%d\n\
                                    DEL Name_%d\n\
                                    GET Name_%d\n\
                                    MOD Name_%d Linus_%d\n\
                                    EXIST Name_%d\n", 
                                    i, i, i, i, i, i, i, i, i, i, i, i, i, i);

        char pattern[512] = {0};
        snprintf(pattern, sizeof(pattern), "SUCCESS\nZZW_%d\nEXIST\nSUCCESS\n\
EXIST\nLinus_%d\nSUCCESS\nNO EXIST\nERROR\nNO EXIST\n",
                                    i, i);
        test_kv_case(connfd, cmd, pattern, "ArrayMultipleCommandsCase");

    }
}

void rbtree_multiple_commands_test(int connfd, int num)
{
    if (!ENABLE_RBTREE_KV_ENGINE) {
        printf("Error: Red-Black Tree engine is not enabled.\n");
        return;
    }
    for(int i = 0; i < num; i++){
        char cmd[512] = {0};
        snprintf(cmd, sizeof(cmd), "RSET Name_%d ZZW_%d\n\
                                    RGET Name_%d\n\
                                    RSET Name_%d Linus_%d\n\
                                    RMOD Name_%d Linus_%d\n\
                                    REXIST Name_%d\n\
                                    RGET Name_%d\n\
                                    RDEL Name_%d\n\
                                    RGET Name_%d\n\
                                    RMOD Name_%d Linus_%d\n\
                                    REXIST Name_%d\n", 
                                    i, i, i, i, i, i, i, i, i, i, i, i, i, i);

        char pattern[512] = {0};
        snprintf(pattern, sizeof(pattern), "SUCCESS\nZZW_%d\nEXIST\nSUCCESS\n\
EXIST\nLinus_%d\nSUCCESS\nNO EXIST\nERROR\nNO EXIST\n",
                                    i, i);
        test_kv_case(connfd, cmd, pattern, "RbTreeMultipleCommandsCase");

    }
}

void hash_multiple_commands_test(int connfd, int num)
{
    if (!ENABLE_HASH_KV_ENGINE) {
        printf("Error: Hash engine is not enabled.\n");
        return;
    }
    for(int i = 0; i < num; i++){
        char cmd[512] = {0};
        snprintf(cmd, sizeof(cmd), "HSET Name_%d ZZW_%d\n\
                                    HGET Name_%d\n\
                                    HSET Name_%d Linus_%d\n\
                                    HMOD Name_%d Linus_%d\n\
                                    HEXIST Name_%d\n\
                                    HGET Name_%d\n\
                                    HDEL Name_%d\n\
                                    HGET Name_%d\n\
                                    HMOD Name_%d Linus_%d\n\
                                    HEXIST Name_%d\n", 
                                    i, i, i, i, i, i, i, i, i, i, i, i, i, i);

        char pattern[512] = {0};
        snprintf(pattern, sizeof(pattern), "SUCCESS\nZZW_%d\nEXIST\nSUCCESS\n\
EXIST\nLinus_%d\nSUCCESS\nNO EXIST\nERROR\nNO EXIST\n",
                                    i, i);
        test_kv_case(connfd, cmd, pattern, "HashMultipleCommandsCase");

    }
}

void skiptable_multiple_commands_test(int connfd, int num)
{
    if (!ENABLE_SKIPTABLE_KV_ENGINE) {
        printf("Error: Skiptable engine is not enabled.\n");
        return;
    }
    for(int i = 0; i < num; i++){
        char cmd[512] = {0};
        snprintf(cmd, sizeof(cmd), "SSET Name_%d ZZW_%d\n\
                                    SGET Name_%d\n\
                                    SSET Name_%d Linus_%d\n\
                                    SMOD Name_%d Linus_%d\n\
                                    SEXIST Name_%d\n\
                                    SGET Name_%d\n\
                                    SDEL Name_%d\n\
                                    SGET Name_%d\n\
                                    SMOD Name_%d Linus_%d\n\
                                    SEXIST Name_%d\n", 
                                    i, i, i, i, i, i, i, i, i, i, i, i, i, i);

        char pattern[512] = {0};
        snprintf(pattern, sizeof(pattern), "SUCCESS\nZZW_%d\nEXIST\nSUCCESS\nEXIST\nLinus_%d\nSUCCESS\nNO EXIST\nERROR\nNO EXIST\n",
                                    i, i);
        test_kv_case(connfd, cmd, pattern, "SkipTableMultipleCommandsCase");

    }
}

void all_engine_multiple_commands_test(int connfd, int num)
{
    if (!ENABLE_ARRAY_KV_ENGINE || !ENABLE_RBTREE_KV_ENGINE 
        || !ENABLE_HASH_KV_ENGINE || !ENABLE_SKIPTABLE_KV_ENGINE) {
        printf("Error: Not all engines are enabled.\n");
        return;
    }
    for(int i = 0; i < num; i++){
        char cmd[512] = {0};
        snprintf(cmd, sizeof(cmd), "SET Name_%d ZZW_%d\n\
                                    RSET Name_%d ZZW_%d\n\
                                    HSET Name_%d ZZW_%d\n\
                                    SSET Name_%d ZZW_%d\n", 
                                    i, i, i, i, i, i, i, i);

        test_kv_case(connfd, cmd, "SUCCESS\nSUCCESS\nSUCCESS\nSUCCESS\n", 
                                    "AllEngineSETCase1");

        memset(cmd, 0, sizeof(cmd));
        snprintf(cmd, sizeof(cmd), "GET Name_%d\n\
                                    RGET Name_%d\n\
                                    HGET Name_%d\n\
                                    SGET Name_%d\n", i, i, i, i);

        char pattern[512] = {0};
        snprintf(pattern, sizeof(pattern), "ZZW_%d\nZZW_%d\nZZW_%d\nZZW_%d\n", 
                                    i, i, i, i);
        test_kv_case(connfd, cmd, pattern, "AllEngineGETCase");

        memset(cmd, 0, sizeof(cmd));
        snprintf(cmd, sizeof(cmd), "SET Name_%d Linus_%d\n\
                                    RSET Name_%d Linus_%d\n\
                                    HSET Name_%d Linus_%d\n\
                                    SSET Name_%d Linus_%d\n", 
                                    i, i, i, i, i, i, i, i);

        test_kv_case(connfd, cmd, "EXIST\nEXIST\nEXIST\nEXIST\n", 
                                    "AllEngineSETCase2"
                                    );

        memset(cmd, 0, sizeof(cmd));
        snprintf(cmd, sizeof(cmd), "MOD Name_%d Linus_%d\n\
                                    RMOD Name_%d Linus_%d\n\
                                    HMOD Name_%d Linus_%d\n\
                                    SMOD Name_%d Linus_%d\n", 
                                    i, i, i, i, i, i, i, i);

        test_kv_case(connfd, cmd, "SUCCESS\nSUCCESS\nSUCCESS\nSUCCESS\n", 
                                    "AllEngineMODCase"
                                    );

        memset(cmd, 0, sizeof(cmd));
        snprintf(cmd, sizeof(cmd), "EXIST Name_%d\n\
                                    REXIST Name_%d\n\
                                    HEXIST Name_%d\n\
                                    SEXIST Name_%d\n", 
                                    i, i, i, i);

        test_kv_case(connfd, cmd, "EXIST\nEXIST\nEXIST\nEXIST\n", 
                                    "AllEngineEXISTCase"
                                    );

        memset(cmd, 0, sizeof(cmd));
        snprintf(cmd, sizeof(cmd), "GET Name_%d\n\
                                    RGET Name_%d\n\
                                    HGET Name_%d\n\
                                    SGET Name_%d\n", 
                                    i, i, i, i);

        memset(pattern, 0, sizeof(pattern));
        snprintf(pattern, sizeof(pattern), "Linus_%d\nLinus_%d\nLinus_%d\nLinus_%d\n",  
                                    i, i, i, i);

        test_kv_case(connfd, cmd, pattern, "AllEngineGETCase");

        memset(cmd, 0, sizeof(cmd));
        snprintf(cmd, sizeof(cmd), "DEL Name_%d\n\
                                    RDEL Name_%d\n\
                                    HDEL Name_%d\n\
                                    SDEL Name_%d\n", 
                                    i, i, i, i);

        test_kv_case(connfd, cmd, "SUCCESS\nSUCCESS\nSUCCESS\nSUCCESS\n", 
                                    "AllEngineDELCase");

        memset(cmd, 0, sizeof(cmd));
        snprintf(cmd, sizeof(cmd), "GET Name_%d\n\
                                    RGET Name_%d\n\
                                    HGET Name_%d\n\
                                    SGET Name_%d\n", 
                                    i, i, i, i);

        test_kv_case(connfd, cmd, "NO EXIST\nNO EXIST\nNO EXIST\nNO EXIST\n", 
                                    "AllEngineDELCase"
                                    );

        memset(cmd, 0, sizeof(cmd));
        snprintf(cmd, sizeof(cmd), "MOD Name_%d Linus_%d\n\
                                    RMOD Name_%d Linus_%d\n\
                                    HMOD Name_%d Linus_%d\n\
                                    SMOD Name_%d Linus_%d\n", 
                                    i, i, i, i, i, i, i, i);
        test_kv_case(connfd, cmd, "ERROR\nERROR\nERROR\nERROR\n", 
                                    "AllEngineDELCase");

        memset(cmd, 0, sizeof(cmd));
        snprintf(cmd, sizeof(cmd), "EXIST Name_%d\n\
                                    REXIST Name_%d\n\
                                    HEXIST Name_%d\n\
                                    SEXIST Name_%d\n", 
                                    i, i, i, i);

        test_kv_case(connfd, cmd, "NO EXIST\nNO EXIST\nNO EXIST\nNO EXIST\n", 
                                    "AllEngineDELCase"
                                    );
    }
    
    
}

static void print_banner(void)
{
    printf("\n");
    printf("███████╗██████╗ ██╗    ██╗    ██╗  ██╗██╗   ██╗    ███████╗████████╗ ██████╗ ██████╗ ███████╗\n");
    printf("╚══███╔╝╚════██╗██║    ██║    ██║ ██╔╝██║   ██║    ██╔════╝╚══██╔══╝██╔═══██╗██╔══██╗██╔════╝\n");
    printf("  ███╔╝  █████╔╝██║ █╗ ██║    █████╔╝ ██║   ██║    ███████╗   ██║   ██║   ██║██████╔╝█████╗  \n");
    printf(" ███╔╝  ██╔═══╝ ██║███╗██║    ██╔═██╗ ╚██╗ ██╔╝    ╚════██║   ██║   ██║   ██║██╔══██╗██╔══╝  \n");
    printf("███████╗███████╗╚███╔███╔╝    ██║  ██╗ ╚████╔╝     ███████║   ██║   ╚██████╔╝██║  ██║███████╗\n");
    printf("╚══════╝╚══════╝ ╚══╝╚══╝     ╚═╝  ╚═╝  ╚═══╝      ╚══════╝   ╚═╝    ╚═════╝ ╚═╝  ╚═╝╚══════╝\n");
    printf("\n");
}

// 打印使用说明
static void print_usage(const char* prog_name) {
    printf("Usage: %s -s <server_ip> -p <port> -m <mode> -n <repeat_num>\n\n", prog_name);
    
    printf("Options:\n");
    printf("  -s <server_ip>    Server IP address\n");
    printf("  -p <port>         Server port number\n");
    printf("  -m <mode>         Test mode (see below)\n");
    printf("  -n <repeat_num>   Number of test repetitions\n\n");
    
    printf("Available Test Modes:\n");
    printf("  [1]  Test Array KV Engine\n");
    printf("  [2]  Test Red-Black Tree KV Engine\n");
    printf("  [3]  Test Hash Table KV Engine\n");
    printf("  [4]  Test SkipTable KV Engine\n");
    printf("  [5]  Test Array with Huge Keys\n");
    printf("  [6]  Test Red-Black Tree with Huge Keys\n");
    printf("  [7]  Test Hash Table with Huge Keys\n");
    printf("  [8]  Test SkipTable with Huge Keys\n");
    printf("  [9]  Test Save Array to Disk\n");
    printf("  [10] Test Save Red-Black Tree to Disk\n");
    printf("  [11] Test Save Hash Table to Disk\n");
    printf("  [12] Test Save SkipTable to Disk\n");
    printf("  [13] Test Load Array from Disk\n");
    printf("  [14] Test Load Red-Black Tree from Disk\n");
    printf("  [15] Test Load Hash Table from Disk\n");
    printf("  [16] Test Load SkipTable from Disk\n");
    printf("  [17] Test Array KV Engine Multiple Commands Test\n");
    printf("  [18] Test Red-Black Tree KV Engine Multiple Commands Test\n");
    printf("  [19] Test Hash Table KV Engine Multiple Commands Test\n");
    printf("  [20] Test SkipTable KV Engine Multiple Commands Test\n");
    printf("  [21] Test All Engine Multiple Commands Test\n");
    printf("  [22] Test Array Range Command Test\n");
    printf("  [23] Test Red-Black Tree Range Command Test\n");
    printf("  [24] Test Hash Range Command Test\n");
    printf("  [25] Test Skiptable Range Command Test\n");
    
    printf("Examples:\n");
    printf("  %s -s 127.0.0.1 -p 8080 -m 1 -n 100\n", prog_name);
    printf("  %s -s localhost -p 8888 -m 10 -n 1\n\n", prog_name);
}

//array: 1, rbtree: 2, hashtable: 3, skiptable: 4
//array_huge_keys: 5, rbtree_huge_keys: 6, hashtable_huge_keys: 7, skiptable_huge_keys: 8
//test_qps_tcpclient -s 127.0.0.1 -p 2048 -m 
int main(int argc, char *argv[])
{
    print_banner();
    print_usage(argv[0]);
    if (argc < 8) {
        //print_usage(argv[0]);
        printf("Invalid parameters, please check Usage!\n");
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
    if (9 == ctx.mode) {
        printf("save array to harddisk test case\n");
        printf("Please Enter filename: ");
        scanf("%s", ctx.filename);
        array_save_test(connfd, ctx.repeat_num, ctx.filename);
        //kvs_array_save(ctx.filename);
    }
    if (10 == ctx.mode) {
        printf("save rbtree to harddisk test case\n");
        printf("Please Enter filename: ");
        scanf("%s", ctx.filename);
        rbtree_save_test(connfd, ctx.repeat_num, ctx.filename);
        //kvs_rbtree_save(ctx.filename);
    }
    if (11 == ctx.mode) {
        printf("save hashtable to harddisk test case\n");
        printf("Please Enter filename: ");
        scanf("%s", ctx.filename);
        hash_save_test(connfd, ctx.repeat_num, ctx.filename);
        //kvs_hashtable_save(ctx.filename);
    }
    if (12 == ctx.mode) {
        printf("save skiptable to harddisk test case\n");
        printf("Please Enter filename: ");
        scanf("%s", ctx.filename);
        skiptable_save_test(connfd, ctx.repeat_num, ctx.filename);
        //kvs_skiptable_save(ctx.filename);
    }
    if (13 == ctx.mode) {
        printf("load array from harddisk test case\n");
        printf("Please Enter filename: ");
        scanf("%s", ctx.filename);
        //kvs_array_load(ctx.filename);
        array_load_test(connfd, ctx.repeat_num, ctx.filename);
    }
    if (14 == ctx.mode) {
        printf("load rbtree from harddisk test case\n");
        printf("Please Enter filename: ");
        scanf("%s", ctx.filename);
        //kvs_rbtree_load(ctx.filename);
        rbtree_load_test(connfd, ctx.repeat_num, ctx.filename);
    }
    if (15 == ctx.mode) {
        printf("load hashtable from harddisk test case\n");
        printf("Please Enter filename: ");
        scanf("%s", ctx.filename);
        hash_load_test(connfd, ctx.repeat_num, ctx.filename);
    }
    if (16 == ctx.mode) {
        printf("load skiptable from harddisk test case\n");
        printf("Please Enter filename: ");
        scanf("%s", ctx.filename);
        skiptable_load_test(connfd, ctx.repeat_num, ctx.filename);
    }
    if (17 == ctx.mode) {
        printf("Test Array KV Engine Multiple Commands\n");
        array_multiple_commands_test(connfd, ctx.repeat_num);
    }
    if (18 == ctx.mode) {
        printf("Test Rbtree KV Engine Multiple Commands\n");
        rbtree_multiple_commands_test(connfd, ctx.repeat_num);
    }
    if (19 == ctx.mode) {
        printf("Test Hash KV Engine Multiple Commands\n");
        hash_multiple_commands_test(connfd, ctx.repeat_num);
    }
    if (20 == ctx.mode) {
        printf("Test Skiptable KV Engine Multiple Commands\n");
        skiptable_multiple_commands_test(connfd, ctx.repeat_num);
    }
    if (21 == ctx.mode) {
        printf("Test All Engine Multiple Commands Test\n");
        all_engine_multiple_commands_test(connfd, ctx.repeat_num);
    }
    if (22 == ctx.mode) {
        printf("Test Array Range Command Test\n");
        printf("Please Enter start range: ");
        int start_range = 0;
        scanf("%d", &start_range);
        printf("Please Enter end range: ");
        int end_range = 0;
        scanf("%d", &end_range);
        printf("SET Range[0, %d]\n", ctx.repeat_num);
        printf("GET Range[%d, %d]\n", start_range, end_range);
        array_range_test(connfd, start_range, end_range, ctx.repeat_num);
    }
    if (23 == ctx.mode) {
        printf("Test Red-Black Tree Range Command Test\n");
        printf("Please Enter start range: ");
        int start_range = 0;
        scanf("%d", &start_range);
        printf("Please Enter end range: ");
        int end_range = 0;
        scanf("%d", &end_range);
        printf("RSET Range[0, %d]\n", ctx.repeat_num);
        printf("RGET Range[%d, %d]\n", start_range, end_range);
        rbtree_range_test(connfd, start_range, end_range, ctx.repeat_num);
    }
    if (24 == ctx.mode) {
        printf("Test Hash Range Command Test\n");
        printf("Please Enter start range: ");
        int start_range = 0;
        scanf("%d", &start_range);
        printf("Please Enter end range: ");
        int end_range = 0;
        scanf("%d", &end_range);
        printf("HSET Range[0, %d]\n", ctx.repeat_num);
        printf("HGET Range[%d, %d]\n", start_range, end_range);
        hash_range_test(connfd, start_range, end_range, ctx.repeat_num);
    }
    if (25 == ctx.mode) {
        printf("Test Skiptable Range Command Test\n");
        printf("Please Enter start range: ");
        int start_range = 0;
        scanf("%d", &start_range);
        printf("Please Enter end range: ");
        int end_range = 0;
        scanf("%d", &end_range);
        printf("SSET Range[0, %d]\n", ctx.repeat_num);
        printf("SGET Range[%d, %d]\n", start_range, end_range);
        skiptable_range_test(connfd, start_range, end_range, ctx.repeat_num);
    }

    gettimeofday(&end, NULL);

    int time_used = TIME_SUB_MS(end, start);
    printf("time: %dms, qps: %d\n", time_used, ctx.repeat_num * 1000 / time_used);

    close(connfd);
    connfd = -1;
    return 0;

}