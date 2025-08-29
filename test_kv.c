#include "kv_store.hpp"

#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <ctype.h>
#include <semaphore.h>

typedef struct test_status_s
{
    int total_cases; //总测试用例数
    int passed_cases; //通过的用例数
    int failed_cases; //失败的用例数
    int total_operations; //总操作数（用于QPS计算)
    pthread_mutex_t status_mutex; //统计信息互斥锁
} test_status_t;

test_status_t g_status = {0, 0, 0, 0, PTHREAD_MUTEX_INITIALIZER};

typedef struct test_context_s{
    char serverip[16];
    char syncip[16];
    int port;
    int syncport;
    int mode;
    int repeat_num;
    char filename[128];
    int failed;
    int thread_id; //线程ID，用于多线程测试
    sem_t* start_sem; //用于同步线程开始
    int engine_type;
    int operations_per_thread;
    int start_range;
    int end_range;
    int start_num;
    int end_num;
} test_context_t;

typedef struct thread_safety_test_s
{
    int num_threads;
	int operations_per_thread;
	char serverip[16];
    int port;
	int engine_type; // 0 for array, 1 for rbtree, 2 for hash, 3 for skiptable
}thread_safety_test_t;

#define TIME_SUB_MS(tv1, tv2) ((tv1.tv_sec - tv2.tv_sec) * 1000.0 + (tv1.tv_usec - tv2.tv_usec) / 1000.0)

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

#define MAX_MSG_LENGTH  512000

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

void array_test_case_multithread(int connfd, int key_id)
{
    char cmd[1024] = { 0 };
    char pattern[1024] = { 0 };
    //printf("key_id:%d\n", key_id);
	snprintf(cmd, sizeof(cmd), "SET Name_%d ZZW_%d\n", key_id, key_id);
	test_kv_case(connfd, cmd, "SUCCESS\n", "SETCase");
    memset(cmd, 0, sizeof(cmd));
	snprintf(cmd, sizeof(cmd), "GET Name_%d\n", key_id);
	snprintf(pattern, sizeof(pattern), "ZZW_%d\n", key_id);
	test_kv_case(connfd, cmd, pattern, "GETCase");
	memset(cmd, 0, sizeof(cmd));
    memset(pattern, 0, sizeof(pattern));
	snprintf(cmd, sizeof(cmd), "SET Name_%d Linus_%d\n", key_id, key_id);
	test_kv_case(connfd, cmd, "EXIST\n", "SETCase");
	memset(cmd, 0, sizeof(cmd));
	snprintf(cmd, sizeof(cmd), "MOD Name_%d Linus_%d\n", key_id, key_id);
	test_kv_case(connfd, cmd, "SUCCESS\n", "MODCase");
	memset(cmd, 0, sizeof(cmd));
	snprintf(cmd, sizeof(cmd), "EXIST Name_%d\n", key_id);
	test_kv_case(connfd, cmd, "EXIST\n", "EXISTCase");
	memset(cmd, 0, sizeof(cmd));
	snprintf(cmd, sizeof(cmd), "GET Name_%d\n", key_id);
	snprintf(pattern, sizeof(pattern), "Linus_%d\n", key_id);
	test_kv_case(connfd, cmd, pattern, "GETCase");
	memset(cmd, 0, sizeof(cmd));
	snprintf(cmd, sizeof(cmd), "DEL Name_%d\n", key_id);
	test_kv_case(connfd, cmd, "SUCCESS\n", "DELCase");
	memset(cmd, 0, sizeof(cmd));
	snprintf(cmd, sizeof(cmd), "GET Name_%d\n", key_id);
	test_kv_case(connfd, cmd, "NO EXIST\n", "GETCase");
	memset(cmd, 0, sizeof(cmd));
	snprintf(cmd, sizeof(cmd), "MOD Name_%d Linus_%d\n", key_id, key_id);
	test_kv_case(connfd, cmd, "ERROR\n", "MODCase");
	memset(cmd, 0, sizeof(cmd));
	snprintf(cmd, sizeof(cmd), "EXIST Name_%d\n", key_id);
	test_kv_case(connfd, cmd, "NO EXIST\n", "EXISTCase");
}

void array_test_case_huge_keys(int connfd, int num)
{
    int i = 0;
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "SET Name_%d ZZW_%d\n", i, i);
        test_kv_case(connfd, cmd, "SUCCESS\n", "SETCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "GET Name_%d\n", i);
        char pattern[128] = {0};
        snprintf(pattern, sizeof(pattern), "ZZW_%d\n", i);
        test_kv_case(connfd, cmd, pattern, "GETCase");
    }
    for (i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "SET Name_%d Linus_%d\n", i, i);
        test_kv_case(connfd, cmd, "EXIST\n", "SETCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "MOD Name_%d Linus_%d\n", i, i);
        test_kv_case(connfd, cmd, "SUCCESS\n", "MODCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "EXIST Name_%d\n", i);
        test_kv_case(connfd, cmd, "EXIST\n", "EXISTCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "GET Name_%d\n", i);
        char pattern[128] = {0};
        snprintf(pattern, sizeof(pattern), "Linus_%d\n", i);
        test_kv_case(connfd, cmd, pattern, "GETCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "DEL Name_%d\n", i);
        test_kv_case(connfd, cmd, "SUCCESS\n", "DELCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "GET Name_%d\n", i);
        test_kv_case(connfd, cmd, "NO EXIST\n", "GETCase");
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "MOD Name_%d Linus_%d\n", i, i);
        char case_str[128] = {0};
        snprintf(case_str, sizeof(case_str), "MODCase_%d\n", i);
        test_kv_case(connfd, cmd, "ERROR\n", case_str);
    }
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "EXIST Name_%d\n", i);
        test_kv_case(connfd, cmd, "NO EXIST\n", "EXISTCase");
    }
}

void array_save_test(int connfd, int num, char* filename)
{
    int i = 0;
    for(i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "SET Name_%d ZZW_%d\n", i, i);
        test_kv_case(connfd, cmd, "SUCCESS\n", "SETCase");
    }
    char cmd[128] = {0};
    snprintf(cmd, sizeof(cmd), "SAVE %s\n", filename);
    test_kv_case(connfd, cmd, "SUCCESS\n", "SAVECase");
}

void array_save_test_multithread(int connfd, test_context_t* ctx)
{
    struct timeval start, end;
	int i = 0;
    for (i = 0; i < ctx->operations_per_thread; i++) {
        int key_id = i + (ctx->thread_id * ctx->operations_per_thread);
        char cmd[512] = { 0 };
        snprintf(cmd, sizeof(cmd), "SET Name_%d ZZW_%d\n", key_id, key_id);
        test_kv_case(connfd, cmd, "SUCCESS\n", "SETCase");
    }
    char cmd[512] = { 0 };
    snprintf(cmd, sizeof(cmd), "SAVE %s\n", ctx->filename);
    gettimeofday(&start, NULL);
    test_kv_case(connfd, cmd, "SUCCESS\n", "SAVECase");
    gettimeofday(&end, NULL);
    double time_used = TIME_SUB_MS(end, start);
	printf("SAVE TEST : Thread %d: time: %fms, qps: %f\n", ctx->thread_id, time_used, (double)(1000.0) / time_used);
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

void array_load_test_multithread(int connfd, test_context_t* ctx)
{
	struct timeval start, end;
	int i = 0;
	//for (i = 0; i < ctx->operations_per_thread; i++) {
	//	int key_id = i + (ctx->thread_id * ctx->operations_per_thread);
	//	char cmd[512] = { 0 };
	//	snprintf(cmd, sizeof(cmd), "GET Name_%d\n", key_id);
	//	test_kv_case(connfd, cmd, "NO EXIST\n", "GETCase");
	//}
	char cmd[512] = { 0 };
	snprintf(cmd, sizeof(cmd), "LOAD %s\n", ctx->filename);
	gettimeofday(&start, NULL);
	test_kv_case(connfd, cmd, "SUCCESS\n", "LOADCase");
	gettimeofday(&end, NULL);
	double time_used = TIME_SUB_MS(end, start);
	printf("LOAD TEST : Thread %d: time: %fms, qps: %f\n", ctx->thread_id, time_used, (double)(1000.0) / time_used);
	for (i = 0; i < ctx->operations_per_thread; i++) {
		int key_id = i + (ctx->thread_id * ctx->operations_per_thread);
		char cmd[512] = { 0 };
		snprintf(cmd, sizeof(cmd), "GET Name_%d\n", key_id);
		char pattern[512] = { 0 };
		snprintf(pattern, sizeof(pattern), "ZZW_%d\n", key_id);
		test_kv_case(connfd, cmd, pattern, "GETCase");
	}
}

void array_range_test(int connfd, int start_range, int end_range, int start_num, int end_num)
{
    struct timeval start, end;
    if (start_range > end_range) {
        printf("array range test failed, start_range > end_range\n");
        return;
    }
    if (start_num <= 0 || end_num <= 0) {
        printf("array range test failed, num <= 0\n");
        return;
    }
    int i = 0;
    char cmd[512] = {0};
    snprintf(cmd, sizeof(cmd), "RANGE Name_%09d Name_%09d\n", start_num, end_num);
    test_kv_case(connfd, cmd, "EMPTY\n", "RANGECase with no SET");
    for (i = start_range; i <= end_range; i++) {
        memset(cmd, 0, sizeof(cmd));
        snprintf(cmd, sizeof(cmd), "SET Name_%09d ZZW_%09d\n", i, i);
        test_kv_case(connfd, cmd, "SUCCESS\n", "SETCase");
    }
    memset(cmd, 0, sizeof(cmd));
    //printf("SET FINISHED\n");
    snprintf(cmd, sizeof(cmd), "RANGE Name_%09d Name_%09d\n", start_num, end_num);
    char pattern[512000] = { 0 };
    if (end_range >= end_num && start_range <= start_num) {
        //printf("end range > num\n");
        int bytes_written = 0;
        for (int i = start_num; i <= end_num; i++) {
			//printf("bytes_written : %d\n", bytes_written);
            char buffer[512] = { 0 };
            snprintf(buffer, sizeof(buffer),  "Name_%09d ZZW_%09d\n", i, i);
            strncpy(pattern + bytes_written, buffer, strlen(buffer));
            bytes_written += strlen(buffer);
        }
        
    } else if (start_num < start_range) {
        snprintf(pattern, sizeof(pattern), "EMPTY\n");
    } else {
        /*end_num > end_range*/
        int bytes_written = 0;
        for (int i = start_num; i <= end_range; i++) {
            char buffer[512] = { 0 };
            snprintf(buffer, sizeof(buffer), "Name_%09d ZZW_%09d\n", i, i);
            strncpy(pattern + bytes_written, buffer, strlen(buffer));
            bytes_written += strlen(buffer);
        }
    }
    //printf("pattern : %s\n", pattern);
    gettimeofday(&start, NULL);
    
    test_kv_case(connfd, cmd, pattern, "RANGECase");

    gettimeofday(&end, NULL);
    double time_used = TIME_SUB_MS(end, start);
    printf("time: %fms, qps: %f\n", time_used, (double)(1000.0) / time_used);
    
}

void array_range_test_multithread(int connfd, test_context_t* ctx)
{
    int interval = (ctx->end_range - ctx->start_range + 1) * 10;
    int sr = (ctx->thread_id + 1) * (ctx->start_range + interval);
    int er = (ctx->thread_id + 1) * (ctx->end_range + interval);
    int sn = (ctx->thread_id + 1) * (ctx->start_num + interval);
    int en = (ctx->thread_id + 1) * (ctx->end_num + interval);
    array_range_test(connfd, sr, er, sn, en);
}

void array_sync_test_source(int connfd, int num)
{
    int i = 0;
    for (i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "SET Name_%d ZZW_%d\n", i, i);
        test_kv_case(connfd, cmd, "SUCCESS\n", "SETCase");
    }
}

void array_sync_test_source_multithread(int connfd, test_context_t* ctx)
{
    int i = 0;
    for (i = 0; i < ctx->operations_per_thread; i++) {
        int key_id = i + (ctx->thread_id + 1) * ctx->operations_per_thread;
        char cmd[512] = { 0 };
        snprintf(cmd, sizeof(cmd), "SET Name_%d ZZW_%d\n", key_id, key_id);
        test_kv_case(connfd, cmd, "SUCCESS\n", "SETCase");
    }
}

void array_sync_test_dest(int connfd, int num, char* source_ip, int source_port)
{
    if (source_ip == NULL || source_port <= 0) {
        printf("array sync test failed, source_ip or source_port is invalid\n");
        return;
    }
    printf("source_ip = %s, source_port = %d\n", source_ip, source_port);
    int i = 0;
    for (i = 0; i < num; i++) {
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "GET Name_%d\n", i);
        test_kv_case(connfd, cmd, "NO EXIST\n", "GETCase1");
    }
    struct timeval start, end;
    gettimeofday(&start, NULL);
    {
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "SYNC %s %d\n", source_ip, source_port);
        test_kv_case(connfd, cmd, "SUCCESS\n", "SYNCCase");
    }
    gettimeofday(&end, NULL);
    int time_used = TIME_SUB_MS(end, start);
    printf("time: %dms, qps: %d\n", time_used, 1000 * num / time_used);
    
    for (i = 0; i < num; i++) {
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "GET Name_%d\n", i);
        char pattern[128] = {0};
        snprintf(pattern, sizeof(pattern), "ZZW_%d\n", i);
        test_kv_case(connfd, cmd, pattern, "GETCase2");
    }
}

void array_sync_test_dest_multithread(int connfd, test_context_t* ctx)
{
    struct timeval start_time, end_time;
    printf("server ip = %s, server_port = %d\n", ctx->serverip, ctx->port);
    printf("sync ip = %s, sync port = %d\n", ctx->syncip, ctx->syncport);
    int i = 0;
    //for (i = 0; i < ctx->operations_per_thread; i++) {
    //    int key_id = i + (ctx->thread_id + 1) * ctx->operations_per_thread;
    //    char cmd[512] = { 0 };
    //    snprintf(cmd, sizeof(cmd), "GET Name_%d\n", key_id);
    //    test_kv_case(connfd, cmd, "NO EXIST\n", "GETCase1");
    //}
    gettimeofday(&start_time, NULL);
    {
        char cmd[512] = { 0 };
        snprintf(cmd, sizeof(cmd), "SYNC %s %d\n", ctx->syncip, ctx->syncport);
        test_kv_case(connfd, cmd, "SUCCESS\n", "SYNCCase");
    }
    //printf("Thread:%d, sync over\n", ctx->thread_id);
    gettimeofday(&end_time, NULL);
    double time_used = TIME_SUB_MS(end_time, start_time);
    printf("time: %fms, qps: %f\n", time_used, (double)(1000.0 * ctx->operations_per_thread) / time_used);
    for (i = 0; i < ctx->operations_per_thread; i++) {
        int key_id = i + (ctx->thread_id + 1) * ctx->operations_per_thread;
        char cmd[512] = { 0 };
        snprintf(cmd, sizeof(cmd), "GET Name_%d\n", key_id);
        char pattern[512] = { 0 };
        snprintf(pattern, sizeof(pattern), "ZZW_%d\n", key_id);
        test_kv_case(connfd, cmd, pattern, "GETCase2");
    }
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

void rbtree_test_case_multithread(int connfd, int key_id)
{
    char cmd[1024] = { 0 };
    char pattern[1024] = { 0 };
    snprintf(cmd, sizeof(cmd), "RSET Name_%d ZZW_%d\n", key_id, key_id);
    test_kv_case(connfd, cmd, "SUCCESS\n", "RSETCase");
    memset(cmd, 0, sizeof(cmd));
    snprintf(cmd, sizeof(cmd), "RGET Name_%d\n", key_id);
    snprintf(pattern, sizeof(pattern), "ZZW_%d\n", key_id);
    test_kv_case(connfd, cmd, pattern, "RGETCase");
    memset(cmd, 0, sizeof(cmd));
    memset(pattern, 0, sizeof(pattern));
    snprintf(cmd, sizeof(cmd), "RSET Name_%d Linus_%d\n", key_id, key_id);
    test_kv_case(connfd, cmd, "EXIST\n", "RSETCase");
    memset(cmd, 0, sizeof(cmd));
    snprintf(cmd, sizeof(cmd), "RMOD Name_%d Linus_%d\n", key_id, key_id);
    test_kv_case(connfd, cmd, "SUCCESS\n", "RMODCase");
    memset(cmd, 0, sizeof(cmd));
    snprintf(cmd, sizeof(cmd), "REXIST Name_%d\n", key_id);
    test_kv_case(connfd, cmd, "EXIST\n", "REXISTCase");
    memset(cmd, 0, sizeof(cmd));
    snprintf(cmd, sizeof(cmd), "RGET Name_%d\n", key_id);
    snprintf(pattern, sizeof(pattern), "Linus_%d\n", key_id);
    test_kv_case(connfd, cmd, pattern, "RGETCase");
    memset(cmd, 0, sizeof(cmd));
    snprintf(cmd, sizeof(cmd), "RDEL Name_%d\n", key_id);
    test_kv_case(connfd, cmd, "SUCCESS\n", "RDELCase");
    memset(cmd, 0, sizeof(cmd));
    snprintf(cmd, sizeof(cmd), "RGET Name_%d\n", key_id);
    test_kv_case(connfd, cmd, "NO EXIST\n", "RGETCase");
    memset(cmd, 0, sizeof(cmd));
    snprintf(cmd, sizeof(cmd), "RMOD Name_%d Linus_%d\n", key_id, key_id);
    test_kv_case(connfd, cmd, "ERROR\n", "RMODCase");
    memset(cmd, 0, sizeof(cmd));
    snprintf(cmd, sizeof(cmd), "REXIST Name_%d\n", key_id);
    test_kv_case(connfd, cmd, "NO EXIST\n", "REXISTCase");
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

void rbtree_save_test_multithread(int connfd, test_context_t* ctx)
{
    struct timeval start, end;
    int i = 0;
    for (i = 0; i < ctx->operations_per_thread; i++) {
        int key_id = i + (ctx->thread_id * ctx->operations_per_thread);
        char cmd[512] = { 0 };
        snprintf(cmd, sizeof(cmd), "RSET Name_%d ZZW_%d\n", key_id, key_id);
        test_kv_case(connfd, cmd, "SUCCESS\n", "RSETCase");
    }
    char cmd[512] = { 0 };
    snprintf(cmd, sizeof(cmd), "RSAVE %s\n", ctx->filename);
    gettimeofday(&start, NULL);
    test_kv_case(connfd, cmd, "SUCCESS\n", "RSAVECase");
    gettimeofday(&end, NULL);
    double time_used = TIME_SUB_MS(end, start);
    printf("SAVE TEST : Thread %d: time: %fms, qps: %f\n", ctx->thread_id, time_used, (double)(1000.0) / time_used);
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

void rbtree_load_test_multithread(int connfd, test_context_t* ctx)
{
    struct timeval start, end;
    int i = 0;
    char cmd[512] = { 0 };
    snprintf(cmd, sizeof(cmd), "RLOAD %s\n", ctx->filename);
    gettimeofday(&start, NULL);
    test_kv_case(connfd, cmd, "SUCCESS\n", "RLOADCase");
    gettimeofday(&end, NULL);
    double time_used = TIME_SUB_MS(end, start);
    printf("RLOAD TEST : Thread %d: time: %fms, qps: %f\n", ctx->thread_id, time_used, (double)(1000.0) / time_used);
    for (i = 0; i < ctx->operations_per_thread; i++) {
        int key_id = i + (ctx->thread_id * ctx->operations_per_thread);
        char cmd[512] = { 0 };
        snprintf(cmd, sizeof(cmd), "RGET Name_%d\n", key_id);
        char pattern[512] = { 0 };
        snprintf(pattern, sizeof(pattern), "ZZW_%d\n", key_id);
        test_kv_case(connfd, cmd, pattern, "RGETCase");
    }
}

void rbtree_range_test(int connfd, int start_range, int end_range, int start_num, int end_num)
{
    struct timeval start, end;
    if (start_range > end_range) {
        printf("rbtree range test failed, start_range > end_range\n");
        return;
    }
    if (start_num <= 0 || end_num <= 0) {
        printf("rbtree range test failed, num <= 0\n");
        return;
    }
    int i = 0;
    char cmd[512] = { 0 };
    snprintf(cmd, sizeof(cmd), "RRANGE Name_%09d Name_%09d\n", start_num, end_num);
    test_kv_case(connfd, cmd, "EMPTY\n", "RRANGECase with no SET");
    for (i = start_range; i <= end_range; i++) {
        memset(cmd, 0, sizeof(cmd));
        snprintf(cmd, sizeof(cmd), "RSET Name_%09d ZZW_%09d\n", i, i);
        test_kv_case(connfd, cmd, "SUCCESS\n", "RSETCase");
    }
    memset(cmd, 0, sizeof(cmd));
    snprintf(cmd, sizeof(cmd), "RRANGE Name_%09d Name_%09d\n", start_num, end_num);
    char pattern[512000] = { 0 };
    if (end_range >= end_num && start_range <= start_num) {
        //printf("end range > num\n");
        int bytes_written = 0;
        for (int i = start_num; i <= end_num; i++) {
            char buffer[512] = { 0 };
            snprintf(buffer, sizeof(buffer), "Name_%09d ZZW_%09d\n", i, i);
            strncpy(pattern + bytes_written, buffer, strlen(buffer));
            bytes_written += strlen(buffer);
        }

    }
    else if (start_num < start_range) {
        snprintf(pattern, sizeof(pattern), "EMPTY\n");
    }
    else {
        /*end_num > end_range*/
        int bytes_written = 0;
        for (int i = start_num; i <= end_range; i++) {
            char buffer[128] = { 0 };
            snprintf(buffer, sizeof(buffer), "Name_%09d ZZW_%09d\n", i, i);
            strncpy(pattern + bytes_written, buffer, strlen(buffer));
            bytes_written += strlen(buffer);
        }
    }
    //printf("pattern : %s\n", pattern);
    gettimeofday(&start, NULL);
    test_kv_case(connfd, cmd, pattern, "RRANGECase");
    gettimeofday(&end, NULL);
    double time_used = TIME_SUB_MS(end, start);
    printf("time: %fms, qps: %f\n", time_used, (double)(1000.0) / time_used);
}

void rbtree_range_test_multithread(int connfd, test_context_t* ctx)
{
    int interval = (ctx->end_range - ctx->start_range + 1) * 10;
    int sr = (ctx->thread_id + 1) * (ctx->start_range + interval);
    int er = (ctx->thread_id + 1) * (ctx->end_range + interval);
    int sn = (ctx->thread_id + 1) * (ctx->start_num + interval);
    int en = (ctx->thread_id + 1) * (ctx->end_num + interval);
    rbtree_range_test(connfd, sr, er, sn, en);
}

void rbtree_sync_test_source(int connfd, int num)
{
    int i = 0;
    for (i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "RSET Name_%d ZZW_%d\n", i, i);
        test_kv_case(connfd, cmd, "SUCCESS\n", "RSETCase");
    }
}

void rbtree_sync_test_source_multithread(int connfd, test_context_t* ctx)
{
    int i = 0;
    for (i = 0; i < ctx->operations_per_thread; i++) {
        int key_id = i + (ctx->thread_id + 1) * ctx->operations_per_thread;
        char cmd[512] = { 0 };
        snprintf(cmd, sizeof(cmd), "RSET Name_%d ZZW_%d\n", key_id, key_id);
        test_kv_case(connfd, cmd, "SUCCESS\n", "RSETCase");
    }
}

void rbtree_sync_test_dest(int connfd, int num, char* source_ip, int source_port)
{
    if (source_ip == NULL || source_port <= 0) {
        printf("rbtree sync test failed, source_ip or source_port is invalid\n");
        return;
    }
    printf("source_ip = %s, source_port = %d\n", source_ip, source_port);
    int i = 0;
    for (i = 0; i < num; i++) {
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "RGET Name_%d\n", i);
        test_kv_case(connfd, cmd, "NO EXIST\n", "RGETCase1");
    }
    struct timeval start, end;
    gettimeofday(&start, NULL);
    {
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "RSYNC %s %d\n", source_ip, source_port);
        test_kv_case(connfd, cmd, "SUCCESS\n", "RSYNCCase");
    }
    gettimeofday(&end, NULL);
    int time_used = TIME_SUB_MS(end, start);
    printf("time: %dms, qps: %d\n", time_used, 1000 * num / time_used);

    for (i = 0; i < num; i++) {
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "RGET Name_%d\n", i);
        char pattern[128] = {0};
        snprintf(pattern, sizeof(pattern), "ZZW_%d\n", i);
        test_kv_case(connfd, cmd, pattern, "RGETCase2");
    }
    
}

void rbtree_sync_test_dest_multithread(int connfd, test_context_t* ctx)
{
    struct timeval start_time, end_time;
    printf("server ip = %s, server_port = %d\n", ctx->serverip, ctx->port);
    printf("sync ip = %s, sync port = %d\n", ctx->syncip, ctx->syncport);
    int i = 0;
    //for (i = 0; i < ctx->operations_per_thread; i++) {
    //    int key_id = i + (ctx->thread_id + 1) * ctx->operations_per_thread;
    //    char cmd[512] = { 0 };
    //    snprintf(cmd, sizeof(cmd), "GET Name_%d\n", key_id);
    //    test_kv_case(connfd, cmd, "NO EXIST\n", "GETCase1");
    //}
    gettimeofday(&start_time, NULL);
    {
        char cmd[512] = { 0 };
        snprintf(cmd, sizeof(cmd), "RSYNC %s %d\n", ctx->syncip, ctx->syncport);
        test_kv_case(connfd, cmd, "SUCCESS\n", "RSYNCCase");
    }
    //printf("Thread:%d, sync over\n", ctx->thread_id);
    gettimeofday(&end_time, NULL);
    double time_used = TIME_SUB_MS(end_time, start_time);
    printf("time: %fms, qps: %f\n", time_used, (double)(1000.0 * ctx->operations_per_thread) / time_used);
    for (i = 0; i < ctx->operations_per_thread; i++) {
        int key_id = i + (ctx->thread_id + 1) * ctx->operations_per_thread;
        char cmd[512] = { 0 };
        snprintf(cmd, sizeof(cmd), "RGET Name_%d\n", key_id);
        char pattern[512] = { 0 };
        snprintf(pattern, sizeof(pattern), "ZZW_%d\n", key_id);
        test_kv_case(connfd, cmd, pattern, "RGETCase2");
    }
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

void hash_test_case_multithread(int connfd, int key_id)
{
    char cmd[1024] = { 0 };
    char pattern[1024] = { 0 };
    snprintf(cmd, sizeof(cmd), "HSET Name_%d ZZW_%d\n", key_id, key_id);
    test_kv_case(connfd, cmd, "SUCCESS\n", "HSETCase");
    memset(cmd, 0, sizeof(cmd));
    snprintf(cmd, sizeof(cmd), "HGET Name_%d\n", key_id);
    snprintf(pattern, sizeof(pattern), "ZZW_%d\n", key_id);
    test_kv_case(connfd, cmd, pattern, "HGETCase");
    memset(cmd, 0, sizeof(cmd));
    memset(pattern, 0, sizeof(pattern));
    snprintf(cmd, sizeof(cmd), "HSET Name_%d Linus_%d\n", key_id, key_id);
    test_kv_case(connfd, cmd, "EXIST\n", "HSETCase");
    memset(cmd, 0, sizeof(cmd));
    snprintf(cmd, sizeof(cmd), "HMOD Name_%d Linus_%d\n", key_id, key_id);
    test_kv_case(connfd, cmd, "SUCCESS\n", "HMODCase");
    memset(cmd, 0, sizeof(cmd));
    snprintf(cmd, sizeof(cmd), "HEXIST Name_%d\n", key_id);
    test_kv_case(connfd, cmd, "EXIST\n", "HEXISTCase");
    memset(cmd, 0, sizeof(cmd));
    snprintf(cmd, sizeof(cmd), "HGET Name_%d\n", key_id);
    snprintf(pattern, sizeof(pattern), "Linus_%d\n", key_id);
    test_kv_case(connfd, cmd, pattern, "HGETCase");
    memset(cmd, 0, sizeof(cmd));
    snprintf(cmd, sizeof(cmd), "HDEL Name_%d\n", key_id);
    test_kv_case(connfd, cmd, "SUCCESS\n", "HDELCase");
    memset(cmd, 0, sizeof(cmd));
    snprintf(cmd, sizeof(cmd), "HGET Name_%d\n", key_id);
    test_kv_case(connfd, cmd, "NO EXIST\n", "HGETCase");
    memset(cmd, 0, sizeof(cmd));
    snprintf(cmd, sizeof(cmd), "HMOD Name_%d Linus_%d\n", key_id, key_id);
    test_kv_case(connfd, cmd, "ERROR\n", "HMODCase");
    memset(cmd, 0, sizeof(cmd));
    snprintf(cmd, sizeof(cmd), "HEXIST Name_%d\n", key_id);
    test_kv_case(connfd, cmd, "NO EXIST\n", "HEXISTCase");
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

void hash_save_test_multithread(int connfd, test_context_t* ctx)
{
    struct timeval start, end;
    int i = 0;
    for (i = 0; i < ctx->operations_per_thread; i++) {
        int key_id = i + (ctx->thread_id * ctx->operations_per_thread);
        char cmd[512] = { 0 };
        snprintf(cmd, sizeof(cmd), "HSET Name_%d ZZW_%d\n", key_id, key_id);
        test_kv_case(connfd, cmd, "SUCCESS\n", "HSETCase");
    }
    char cmd[512] = { 0 };
    snprintf(cmd, sizeof(cmd), "HSAVE %s\n", ctx->filename);
    gettimeofday(&start, NULL);
    test_kv_case(connfd, cmd, "SUCCESS\n", "HSAVECase");
    gettimeofday(&end, NULL);
    double time_used = TIME_SUB_MS(end, start);
    printf("SAVE TEST : Thread %d: time: %fms, qps: %f\n", ctx->thread_id, time_used, (double)(1000.0) / time_used);
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

void hash_load_test_multithread(int connfd, test_context_t* ctx)
{
    struct timeval start, end;
    int i = 0;
    char cmd[512] = { 0 };
    snprintf(cmd, sizeof(cmd), "HLOAD %s\n", ctx->filename);
    gettimeofday(&start, NULL);
    test_kv_case(connfd, cmd, "SUCCESS\n", "HLOADCase");
    gettimeofday(&end, NULL);
    double time_used = TIME_SUB_MS(end, start);
    printf("HLOAD TEST : Thread %d: time: %fms, qps: %f\n", ctx->thread_id, time_used, (double)(1000.0) / time_used);
    for (i = 0; i < ctx->operations_per_thread; i++) {
        int key_id = i + (ctx->thread_id * ctx->operations_per_thread);
        char cmd[512] = { 0 };
        snprintf(cmd, sizeof(cmd), "HGET Name_%d\n", key_id);
        char pattern[512] = { 0 };
        snprintf(pattern, sizeof(pattern), "ZZW_%d\n", key_id);
        test_kv_case(connfd, cmd, pattern, "HGETCase");
    }
}

void hash_range_test(int connfd, int start_range, int end_range, int start_num, int end_num)
{
    struct timeval start, end;
    if (start_range > end_range) {
        printf("hash range test failed, start_range > end_range\n");
        return;
    }
    if (start_num <= 0 || end_num <= 0) {
        printf("hash range test failed, num <= 0\n");
        return;
    }
    int i = 0;
    char cmd[512] = { 0 };
    snprintf(cmd, sizeof(cmd), "HRANGE Name_%09d Name_%09d\n", start_num, end_num);
    test_kv_case(connfd, cmd, "EMPTY\n", "HRANGECase with no SET");
    for (i = start_range; i <= end_range; i++) {
        memset(cmd, 0, sizeof(cmd));
        snprintf(cmd, sizeof(cmd), "HSET Name_%09d ZZW_%09d\n", i, i);
        test_kv_case(connfd, cmd, "SUCCESS\n", "HSETCase");
    }
    memset(cmd, 0, sizeof(cmd));
    snprintf(cmd, sizeof(cmd), "HRANGE Name_%09d Name_%09d\n", start_num, end_num);
    char pattern[512000] = { 0 };
    if (end_range >= end_num && start_range <= start_num) {
        //printf("end range > num\n");
        int bytes_written = 0;
        for (int i = start_num; i <= end_num; i++) {
            char buffer[512] = { 0 };
            snprintf(buffer, sizeof(buffer), "Name_%09d ZZW_%09d\n", i, i);
            strncpy(pattern + bytes_written, buffer, strlen(buffer));
            bytes_written += strlen(buffer);
        }

    }
    else if (start_num < start_range) {
        snprintf(pattern, sizeof(pattern), "EMPTY\n");
    }
    else {
        /*end_num > end_range*/
        int bytes_written = 0;
        for (int i = start_num; i <= end_range; i++) {
            char buffer[128] = { 0 };
            snprintf(buffer, sizeof(buffer), "Name_%09d ZZW_%09d\n", i, i);
            strncpy(pattern + bytes_written, buffer, strlen(buffer));
            bytes_written += strlen(buffer);
        }
    }
    //printf("pattern : %s\n", pattern);
    gettimeofday(&start, NULL);
    test_kv_case(connfd, cmd, pattern, "HRANGECase");
    gettimeofday(&end, NULL);
    double time_used = TIME_SUB_MS(end, start);
    printf("time: %fms, qps: %f\n", time_used, (double)(1000.0) / time_used);
#if 0
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
#endif
}

void hash_range_test_multithread(int connfd, test_context_t* ctx)
{
    int interval = (ctx->end_range - ctx->start_range + 1) * 10;
    int sr = (ctx->thread_id + 1) * (ctx->start_range + interval);
    int er = (ctx->thread_id + 1) * (ctx->end_range + interval);
    int sn = (ctx->thread_id + 1) * (ctx->start_num + interval);
    int en = (ctx->thread_id + 1) * (ctx->end_num + interval);
    hash_range_test(connfd, sr, er, sn, en);
}

void hash_sync_test_source(int connfd, int num)
{
    int i = 0;
    for (i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "HSET Name_%d ZZW_%d\n", i, i);
        test_kv_case(connfd, cmd, "SUCCESS\n", "HSETCase");
    }
}

void hash_sync_test_source_multithread(int connfd, test_context_t* ctx)
{
    int i = 0;
    for (i = 0; i < ctx->operations_per_thread; i++) {
        int key_id = i + (ctx->thread_id + 1) * ctx->operations_per_thread;
        char cmd[512] = { 0 };
        snprintf(cmd, sizeof(cmd), "HSET Name_%d ZZW_%d\n", key_id, key_id);
        test_kv_case(connfd, cmd, "SUCCESS\n", "HSETCase");
    }
}

void hash_sync_test_dest(int connfd, int num, char* source_ip, int source_port)
{
    if (source_ip == NULL || source_port <= 0) {
        printf("hash sync test failed, source_ip or source_port is invalid\n");
        return;
    }
    printf("source_ip = %s, source_port = %d\n", source_ip, source_port);
    int i = 0;
    for (i = 0; i < num; i++) {
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "HGET Name_%d\n", i);
        test_kv_case(connfd, cmd, "NO EXIST\n", "HGETCase1");
    }
    struct timeval start, end;
    gettimeofday(&start, NULL);
    {
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "HSYNC %s %d\n", source_ip, source_port);
        test_kv_case(connfd, cmd, "SUCCESS\n", "HSYNCCase");
    }
    gettimeofday(&end, NULL);
    int time_used = TIME_SUB_MS(end, start);
    printf("time: %dms, qps: %d\n", time_used, 1000 * num / time_used);

    for (i = 0; i < num; i++) {
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "HGET Name_%d\n", i);
        char pattern[128] = {0};
        snprintf(pattern, sizeof(pattern), "ZZW_%d\n", i);
        test_kv_case(connfd, cmd, pattern, "HGETCase2");
    }
    
}

void hash_sync_test_dest_multithread(int connfd, test_context_t* ctx)
{
    struct timeval start_time, end_time;
    printf("server ip = %s, server_port = %d\n", ctx->serverip, ctx->port);
    printf("sync ip = %s, sync port = %d\n", ctx->syncip, ctx->syncport);
    int i = 0;
    //for (i = 0; i < ctx->operations_per_thread; i++) {
    //    int key_id = i + (ctx->thread_id + 1) * ctx->operations_per_thread;
    //    char cmd[512] = { 0 };
    //    snprintf(cmd, sizeof(cmd), "GET Name_%d\n", key_id);
    //    test_kv_case(connfd, cmd, "NO EXIST\n", "GETCase1");
    //}
    gettimeofday(&start_time, NULL);
    {
        char cmd[512] = { 0 };
        snprintf(cmd, sizeof(cmd), "HSYNC %s %d\n", ctx->syncip, ctx->syncport);
        test_kv_case(connfd, cmd, "SUCCESS\n", "HSYNCCase");
    }
    //printf("Thread:%d, sync over\n", ctx->thread_id);
    gettimeofday(&end_time, NULL);
    double time_used = TIME_SUB_MS(end_time, start_time);
    printf("time: %fms, qps: %f\n", time_used, (double)(1000.0 * ctx->operations_per_thread) / time_used);
    for (i = 0; i < ctx->operations_per_thread; i++) {
        int key_id = i + (ctx->thread_id + 1) * ctx->operations_per_thread;
        char cmd[512] = { 0 };
        snprintf(cmd, sizeof(cmd), "HGET Name_%d\n", key_id);
        char pattern[512] = { 0 };
        snprintf(pattern, sizeof(pattern), "ZZW_%d\n", key_id);
        test_kv_case(connfd, cmd, pattern, "HGETCase2");
    }
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

void skiptable_test_case_multithread(int connfd, int key_id)
{
    char cmd[1024] = { 0 };
    char pattern[1024] = { 0 };
    //printf("key_id: %d\n", key_id);
    snprintf(cmd, sizeof(cmd), "SSET Name_%d ZZW_%d\n", key_id, key_id);
    test_kv_case(connfd, cmd, "SUCCESS\n", "SSETCase");
    memset(cmd, 0, sizeof(cmd));
    snprintf(cmd, sizeof(cmd), "SGET Name_%d\n", key_id);
    snprintf(pattern, sizeof(pattern), "ZZW_%d\n", key_id);
    test_kv_case(connfd, cmd, pattern, "SGETCase");
    memset(cmd, 0, sizeof(cmd));
    memset(pattern, 0, sizeof(pattern));
    snprintf(cmd, sizeof(cmd), "SSET Name_%d Linus_%d\n", key_id, key_id);
    test_kv_case(connfd, cmd, "EXIST\n", "SSETCase");
    memset(cmd, 0, sizeof(cmd));
    snprintf(cmd, sizeof(cmd), "SMOD Name_%d Linus_%d\n", key_id, key_id);
    test_kv_case(connfd, cmd, "SUCCESS\n", "SMODCase");
    memset(cmd, 0, sizeof(cmd));
    snprintf(cmd, sizeof(cmd), "SEXIST Name_%d\n", key_id);
    test_kv_case(connfd, cmd, "EXIST\n", "SEXISTCase");
    memset(cmd, 0, sizeof(cmd));
    snprintf(cmd, sizeof(cmd), "SGET Name_%d\n", key_id);
    snprintf(pattern, sizeof(pattern), "Linus_%d\n", key_id);
    test_kv_case(connfd, cmd, pattern, "SGETCase");
    memset(cmd, 0, sizeof(cmd));
    snprintf(cmd, sizeof(cmd), "SDEL Name_%d\n", key_id);
    test_kv_case(connfd, cmd, "SUCCESS\n", "SDELCase");
    memset(cmd, 0, sizeof(cmd));
    snprintf(cmd, sizeof(cmd), "SGET Name_%d\n", key_id);
    test_kv_case(connfd, cmd, "NO EXIST\n", "SGETCase");
    memset(cmd, 0, sizeof(cmd));
    snprintf(cmd, sizeof(cmd), "SMOD Name_%d Linus_%d\n", key_id, key_id);
    test_kv_case(connfd, cmd, "ERROR\n", "SMODCase");
    memset(cmd, 0, sizeof(cmd));
    snprintf(cmd, sizeof(cmd), "SEXIST Name_%d\n", key_id);
    test_kv_case(connfd, cmd, "NO EXIST\n", "SEXISTCase");
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

void skiptable_save_test_multithread(int connfd, test_context_t* ctx)
{
    struct timeval start, end;
    int i = 0;
    for (i = 0; i < ctx->operations_per_thread; i++) {
        int key_id = i + (ctx->thread_id * ctx->operations_per_thread);
        char cmd[512] = { 0 };
        snprintf(cmd, sizeof(cmd), "SSET Name_%d ZZW_%d\n", key_id, key_id);
        test_kv_case(connfd, cmd, "SUCCESS\n", "SSETCase");
    }
    char cmd[512] = { 0 };
    snprintf(cmd, sizeof(cmd), "SSAVE %s\n", ctx->filename);
    gettimeofday(&start, NULL);
    test_kv_case(connfd, cmd, "SUCCESS\n", "SSAVECase");
    gettimeofday(&end, NULL);
    double time_used = TIME_SUB_MS(end, start);
    printf("SAVE TEST : Thread %d: time: %fms, qps: %f\n", ctx->thread_id, time_used, (double)(1000.0) / time_used);
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

void skiptable_load_test_multithread(int connfd, test_context_t* ctx)
{
    struct timeval start, end;
    int i = 0;
    char cmd[512] = { 0 };
    snprintf(cmd, sizeof(cmd), "SLOAD %s\n", ctx->filename);
    gettimeofday(&start, NULL);
    test_kv_case(connfd, cmd, "SUCCESS\n", "SLOADCase");
    gettimeofday(&end, NULL);
    double time_used = TIME_SUB_MS(end, start);
    printf("SLOAD TEST : Thread %d: time: %fms, qps: %f\n", ctx->thread_id, time_used, (double)(1000.0) / time_used);
    for (i = 0; i < ctx->operations_per_thread; i++) {
        int key_id = i + (ctx->thread_id * ctx->operations_per_thread);
        char cmd[512] = { 0 };
        snprintf(cmd, sizeof(cmd), "SGET Name_%d\n", key_id);
        char pattern[512] = { 0 };
        snprintf(pattern, sizeof(pattern), "ZZW_%d\n", key_id);
        test_kv_case(connfd, cmd, pattern, "SGETCase");
    }
}

void skiptable_range_test(int connfd, int start_range, int end_range, int start_num, int end_num)
{
    struct timeval start, end;
    if (start_range > end_range) {
        printf("skiptable range test failed, start_range > end_range\n");
        return;
    }
    if (start_num <= 0 || end_num <= 0) {
        printf("skiptable range test failed, num <= 0\n");
        return;
    }
    int i = 0;
    char cmd[512] = { 0 };
    snprintf(cmd, sizeof(cmd), "SRANGE Name_%09d Name_%09d\n", start_num, end_num);
    test_kv_case(connfd, cmd, "EMPTY\n", "SRANGECase with no SET");
    for (i = start_range; i <= end_range; i++) {
        memset(cmd, 0, sizeof(cmd));
        snprintf(cmd, sizeof(cmd), "SSET Name_%09d ZZW_%09d\n", i, i);
        test_kv_case(connfd, cmd, "SUCCESS\n", "SSETCase");
    }
    memset(cmd, 0, sizeof(cmd));
    snprintf(cmd, sizeof(cmd), "SRANGE Name_%09d Name_%09d\n", start_num, end_num);
    char pattern[512000] = { 0 };
    if (end_range >= end_num && start_range <= start_num) {
        //printf("end range > num\n");
        int bytes_written = 0;
        for (int i = start_num; i <= end_num; i++) {
            char buffer[512] = { 0 };
            snprintf(buffer, sizeof(buffer), "Name_%09d ZZW_%09d\n", i, i);
            strncpy(pattern + bytes_written, buffer, strlen(buffer));
            bytes_written += strlen(buffer);
        }

    }
    else if (start_num < start_range) {
        snprintf(pattern, sizeof(pattern), "EMPTY\n");
    }
    else {
        /*end_num > end_range*/
        int bytes_written = 0;
        for (int i = start_num; i <= end_range; i++) {
            char buffer[128] = { 0 };
            snprintf(buffer, sizeof(buffer), "Name_%09d ZZW_%09d\n", i, i);
            strncpy(pattern + bytes_written, buffer, strlen(buffer));
            bytes_written += strlen(buffer);
        }
    }
    //printf("pattern : %s\n", pattern);
    gettimeofday(&start, NULL);
    test_kv_case(connfd, cmd, pattern, "SRANGECase");
    gettimeofday(&end, NULL);
    double time_used = TIME_SUB_MS(end, start);
    printf("time: %fms, qps: %f\n", time_used, (double)(1000.0) / time_used);
#if 0
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
#endif
}

void skiptable_range_test_multithread(int connfd, test_context_t* ctx)
{
    int interval = (ctx->end_range - ctx->start_range + 1) * 10;
    int sr = (ctx->thread_id + 1) * (ctx->start_range + interval);
    int er = (ctx->thread_id + 1) * (ctx->end_range + interval);
    int sn = (ctx->thread_id + 1) * (ctx->start_num + interval);
    int en = (ctx->thread_id + 1) * (ctx->end_num + interval);
    skiptable_range_test(connfd, sr, er, sn, en);
}

void skiptable_sync_test_source(int connfd, int num)
{
    int i = 0;
    for (i = 0; i < num; i++){
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "SSET Name_%d ZZW_%d\n", i, i);
        test_kv_case(connfd, cmd, "SUCCESS\n", "SSETCase");
    }
}

void skiptable_sync_test_source_multithread(int connfd, test_context_t* ctx)
{
    int i = 0;
    for (i = 0; i < ctx->operations_per_thread; i++) {
        int key_id = i + (ctx->thread_id + 1) * ctx->operations_per_thread;
        char cmd[512] = { 0 };
        snprintf(cmd, sizeof(cmd), "SSET Name_%d ZZW_%d\n", key_id, key_id);
        test_kv_case(connfd, cmd, "SUCCESS\n", "SSETCase");
    }
}

void skiptable_sync_test_dest(int connfd, int num, char* source_ip, int source_port)
{
    if (source_ip == NULL || source_port <= 0) {
        printf("skiptable sync test failed, source_ip or source_port is invalid\n");
        return;
    }
    printf("source_ip = %s, source_port = %d\n", source_ip, source_port);
    int i = 0;
    for (i = 0; i < num; i++) {
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "SGET Name_%d\n", i);
        test_kv_case(connfd, cmd, "NO EXIST\n", "SGETCase1");
    }
    struct timeval start, end;
    gettimeofday(&start, NULL);
    {
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "SSYNC %s %d\n", source_ip, source_port);
        test_kv_case(connfd, cmd, "SUCCESS\n", "SSYNCCase");
    }
    gettimeofday(&end, NULL);
    int time_used = TIME_SUB_MS(end, start);
    printf("time: %dms, qps: %d\n", time_used, 1000 * num / time_used);

    for (i = 0; i < num; i++) {
        char cmd[128] = {0};
        snprintf(cmd, sizeof(cmd), "SGET Name_%d\n", i);
        char pattern[128] = {0};
        snprintf(pattern, sizeof(pattern), "ZZW_%d\n", i);
        test_kv_case(connfd, cmd, pattern, "SGETCase2");
    }
    
}

void skiptable_sync_test_dest_multithread(int connfd, test_context_t* ctx)
{
    struct timeval start_time, end_time;
    printf("server ip = %s, server_port = %d\n", ctx->serverip, ctx->port);
    printf("sync ip = %s, sync port = %d\n", ctx->syncip, ctx->syncport);
    int i = 0;
    //for (i = 0; i < ctx->operations_per_thread; i++) {
    //    int key_id = i + (ctx->thread_id + 1) * ctx->operations_per_thread;
    //    char cmd[512] = { 0 };
    //    snprintf(cmd, sizeof(cmd), "GET Name_%d\n", key_id);
    //    test_kv_case(connfd, cmd, "NO EXIST\n", "GETCase1");
    //}
    gettimeofday(&start_time, NULL);
    {
        char cmd[512] = { 0 };
        snprintf(cmd, sizeof(cmd), "SSYNC %s %d\n", ctx->syncip, ctx->syncport);
        test_kv_case(connfd, cmd, "SUCCESS\n", "SSYNCCase");
    }
    //printf("Thread:%d, sync over\n", ctx->thread_id);
    gettimeofday(&end_time, NULL);
    double time_used = TIME_SUB_MS(end_time, start_time);
    printf("time: %fms, qps: %f\n", time_used, (double)(1000.0 * ctx->operations_per_thread) / time_used);
    for (i = 0; i < ctx->operations_per_thread; i++) {
        int key_id = i + (ctx->thread_id + 1) * ctx->operations_per_thread;
        char cmd[512] = { 0 };
        snprintf(cmd, sizeof(cmd), "SGET Name_%d\n", key_id);
        char pattern[512] = { 0 };
        snprintf(pattern, sizeof(pattern), "ZZW_%d\n", key_id);
        test_kv_case(connfd, cmd, pattern, "SGETCase2");
    }
}

void array_multiple_commands_test(int connfd, int num)
{
    if (!ENABLE_ARRAY_KV_ENGINE) {
        printf("Error: Array engine is not enabled.\n");
        return;
    }
    for(int i = 0; i < num; i++){
        char cmd[1024] = {0};
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

        char pattern[1024] = {0};
        snprintf(pattern, sizeof(pattern), "SUCCESS\nZZW_%d\nEXIST\nSUCCESS\n\
EXIST\nLinus_%d\nSUCCESS\nNO EXIST\nERROR\nNO EXIST\n",
                                    i, i);
        test_kv_case(connfd, cmd, pattern, "ArrayMultipleCommandsCase");

    }
}

void array_multiple_commands_test_multithread(int connfd, test_context_t* ctx)
{
    if (!ENABLE_ARRAY_KV_ENGINE) {
        printf("Error: Array engine is not enabled.\n");
        return;
    }
    struct timeval start, end;
	int start_num = ctx->thread_id * ctx->operations_per_thread;
    int end_num = start_num + ctx->operations_per_thread;
    gettimeofday(&start, NULL);
    for (int i = start_num; i < end_num; i++) {
        char cmd[1024] = { 0 };
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

        char pattern[1024] = { 0 };
        snprintf(pattern, sizeof(pattern), "SUCCESS\nZZW_%d\nEXIST\nSUCCESS\n\
EXIST\nLinus_%d\nSUCCESS\nNO EXIST\nERROR\nNO EXIST\n",
i, i);
		
        test_kv_case(connfd, cmd, pattern, "ArrayMultipleCommandsCase");
        
    }
    gettimeofday(&end, NULL);
    double time_used = TIME_SUB_MS(end, start);
    printf("ArrayMultipleCommandsCase : Thread %d, time: %fms, qps: %f\n", ctx->thread_id, time_used, (double)(ctx->operations_per_thread * 10 * 1000.0) / time_used);
}

void rbtree_multiple_commands_test(int connfd, int num)
{
    if (!ENABLE_RBTREE_KV_ENGINE) {
        printf("Error: Red-Black Tree engine is not enabled.\n");
        return;
    }
    for(int i = 0; i < num; i++){
        char cmd[1024] = {0};
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

        char pattern[1024] = {0};
        snprintf(pattern, sizeof(pattern), "SUCCESS\nZZW_%d\nEXIST\nSUCCESS\n\
EXIST\nLinus_%d\nSUCCESS\nNO EXIST\nERROR\nNO EXIST\n",
                                    i, i);
        test_kv_case(connfd, cmd, pattern, "RbTreeMultipleCommandsCase");

    }
}

void rbtree_multiple_commands_test_multithread(int connfd, test_context_t* ctx)
{
    if (!ENABLE_RBTREE_KV_ENGINE) {
        printf("Error: Rbtree engine is not enabled.\n");
        return;
    }
    struct timeval start, end;
    int start_num = ctx->thread_id * ctx->operations_per_thread;
    int end_num = start_num + ctx->operations_per_thread;
    gettimeofday(&start, NULL);
    for (int i = start_num; i < end_num; i++) {
        char cmd[1024] = { 0 };
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

        char pattern[1024] = { 0 };
        snprintf(pattern, sizeof(pattern), "SUCCESS\nZZW_%d\nEXIST\nSUCCESS\n\
EXIST\nLinus_%d\nSUCCESS\nNO EXIST\nERROR\nNO EXIST\n",
i, i);

        test_kv_case(connfd, cmd, pattern, "RbTreeMultipleCommandsCase");

    }
    gettimeofday(&end, NULL);
    double time_used = TIME_SUB_MS(end, start);
    printf("RbTreeMultipleCommandsCase : Thread %d, time: %fms, qps: %f\n", ctx->thread_id, time_used, (double)(ctx->operations_per_thread * 10 * 1000.0) / time_used);
}

void hash_multiple_commands_test(int connfd, int num)
{
    if (!ENABLE_HASH_KV_ENGINE) {
        printf("Error: Hash engine is not enabled.\n");
        return;
    }
    for(int i = 0; i < num; i++){
        char cmd[1024] = {0};
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

        char pattern[1024] = {0};
        snprintf(pattern, sizeof(pattern), "SUCCESS\nZZW_%d\nEXIST\nSUCCESS\n\
EXIST\nLinus_%d\nSUCCESS\nNO EXIST\nERROR\nNO EXIST\n",
                                    i, i);
        test_kv_case(connfd, cmd, pattern, "HashMultipleCommandsCase");

    }
}

void hash_multiple_commands_test_multithread(int connfd, test_context_t* ctx)
{
    if (!ENABLE_HASH_KV_ENGINE) {
        printf("Error: Hash engine is not enabled.\n");
        return;
    }
    struct timeval start, end;
    int start_num = ctx->thread_id * ctx->operations_per_thread;
    int end_num = start_num + ctx->operations_per_thread;
    gettimeofday(&start, NULL);
    for (int i = start_num; i < end_num; i++) {
        char cmd[1024] = { 0 };
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

        char pattern[1024] = { 0 };
        snprintf(pattern, sizeof(pattern), "SUCCESS\nZZW_%d\nEXIST\nSUCCESS\n\
EXIST\nLinus_%d\nSUCCESS\nNO EXIST\nERROR\nNO EXIST\n",
i, i);

        test_kv_case(connfd, cmd, pattern, "HashMultipleCommandsCase");

    }
    gettimeofday(&end, NULL);
    double time_used = TIME_SUB_MS(end, start);
    printf("HashMultipleCommandsCase : Thread %d, time: %fms, qps: %f\n", ctx->thread_id, time_used, (double)(ctx->operations_per_thread * 10 * 1000.0) / time_used);
}

void skiptable_multiple_commands_test(int connfd, int num)
{
    if (!ENABLE_SKIPTABLE_KV_ENGINE) {
        printf("Error: Skiptable engine is not enabled.\n");
        return;
    }
    for(int i = 0; i < num; i++){
        char cmd[1024] = {0};
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

        char pattern[1024] = {0};
        snprintf(pattern, sizeof(pattern), "SUCCESS\nZZW_%d\nEXIST\nSUCCESS\nEXIST\nLinus_%d\nSUCCESS\nNO EXIST\nERROR\nNO EXIST\n",
                                    i, i);
        test_kv_case(connfd, cmd, pattern, "SkipTableMultipleCommandsCase");

    }
}

void skiptable_multiple_commands_test_multithread(int connfd, test_context_t* ctx)
{
    if (!ENABLE_SKIPTABLE_KV_ENGINE) {
        printf("Error: Skiptable engine is not enabled.\n");
        return;
    }
    struct timeval start, end;
    int start_num = ctx->thread_id * ctx->operations_per_thread;
    int end_num = start_num + ctx->operations_per_thread;
    gettimeofday(&start, NULL);
    for (int i = start_num; i < end_num; i++) {
        char cmd[1024] = { 0 };
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

        char pattern[1024] = { 0 };
        snprintf(pattern, sizeof(pattern), "SUCCESS\nZZW_%d\nEXIST\nSUCCESS\n\
EXIST\nLinus_%d\nSUCCESS\nNO EXIST\nERROR\nNO EXIST\n",
i, i);

        test_kv_case(connfd, cmd, pattern, "SkiptableMultipleCommandsCase");

    }
    gettimeofday(&end, NULL);
    double time_used = TIME_SUB_MS(end, start);
    printf("SkiptableMultipleCommandsCase : Thread % d, time : % fms, qps : % f\n", ctx->thread_id, time_used, (double)(ctx->operations_per_thread * 10 * 1000.0) / time_used);
}

void all_engine_multiple_commands_test(int connfd, int num)
{
    if (!ENABLE_ARRAY_KV_ENGINE || !ENABLE_RBTREE_KV_ENGINE 
        || !ENABLE_HASH_KV_ENGINE || !ENABLE_SKIPTABLE_KV_ENGINE) {
        printf("Error: Not all engines are enabled.\n");
        return;
    }
    for(int i = 0; i < num; i++){
        char cmd[1024] = {0};
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

        char pattern[1024] = {0};
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
    printf("Usage: %s -s <server_ip> -p <port> -m <mode> -n <repeat_num> [-t <num_threads>]\n\n", prog_name);
    
    printf("Options:\n");
    printf("  -s <server_ip>    Server IP address\n");
    printf("  -p <port>         Server port number\n");
    printf("  -m <mode>         Test mode (see below)\n");
    printf("  -n <repeat_num>   Number of test repetitions\n\n");
    printf("  -t <num_threads>  Number of threads (for multi-thread tests)\n\n");
    
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
    printf("  [26] Test Array Sync Command[Source]\n");
    printf("  [27] Test Array Sync Command[Dest]\n");
    printf("  [28] Test Red-Black Tree Sync Command[Source]\n");
    printf("  [29] Test Red-Block Tree Sync Command[Dest]\n");
    printf("  [30] Test Hash Sync Command[Source]\n");
    printf("  [31] Test Hash Sync Command[Dest]\n");
    printf("  [32] Test Skiptable Sync Command[Source]\n");
    printf("  [33] Test Skiptable Sync Command[Dest]\n");
    printf("  [34] Test Array KV Engine multi thread\n");
    printf("  [35] Test Red-Black Tree KV Engine multi threads\n");
    printf("  [36] Test Hash Table KV Engine multi threads\n");
    printf("  [37] Test SkipTable KV Engine multi threads\n");
    printf("  [38] Test Array with Huge Keys multi threads\n");
    printf("  [39] Test Red-Black Tree with Huge Keys multi threads\n");
    printf("  [40] Test Hash Table with Huge Keys multi threads\n");
    printf("  [41] Test SkipTable with Huge Keys multi threads\n");
	printf("  [42] Test Save Array to Disk multi threads\n");
	printf("  [43] Test Save Red-Black Tree to Disk multi threads\n");
	printf("  [44] Test Save Hash Table to Disk multi threads\n");
	printf("  [45] Test Save SkipTable to Disk multi threads\n");
	printf("  [46] Test Load Array from Disk multi threads\n");
	printf("  [47] Test Load Red-Black Tree from Disk multi threads\n");
	printf("  [48] Test Load Hash Table from Disk multi threads\n");
	printf("  [49] Test Load SkipTable from Disk multi threads\n");
    printf("  [50] Test Array KV Engine Multiple Commands Test multi threads\n");
    printf("  [51] Test Red-Black Tree KV Engine Multiple Commands Test multi threads\n");
    printf("  [52] Test Hash Table KV Engine Multiple Commands Test multi threads\n");
    printf("  [53] Test SkipTable KV Engine Multiple Commands Test multi threads\n");
    printf("  [54] Test Array Range Command Test multi threads\n");
    printf("  [55] Test Red-Black Tree Range Command Test multi threads\n");
    printf("  [56] Test Hash Range Command Test multi threads\n");
    printf("  [57] Test Skiptable Range Command Test multi threads\n");
    printf("  [58] Test Array Sync Command[Source] multi threads\n");
    printf("  [59] Test Array Sync Command[Dest] multi threads\n");
    printf("  [60] Test Red-Black Tree Sync Command[Source] multi threads\n");
    printf("  [61] Test Red-Block Tree Sync Command[Dest] multi threads\n");
    printf("  [62] Test Hash Sync Command[Source] multi threads\n");
    printf("  [63] Test Hash Sync Command[Dest] multi threads\n");
    printf("  [64] Test Skiptable Sync Command[Source] multi threads\n");
    printf("  [65] Test Skiptable Sync Command[Dest] multi threads\n");

    
    printf("Examples:\n");
    printf("  %s -s 127.0.0.1 -p 8080 -m 1 -n 100\n", prog_name);
    printf("  %s -s localhost -p 8888 -m 10 -n 100 -t 4\n\n", prog_name);
}

// 根据引擎类型执行相应操作
void perform_engine_operations(int connfd, test_context_t* ctx)
{
    int i;

    switch (ctx->mode) {
        case 34: // Array 
        {
            for (i = 0; i < ctx->operations_per_thread; i++) {
                // 为每个线程生成唯一的键，避免键冲突
                int key_id = i + (ctx->thread_id * ctx->operations_per_thread);
                array_test_case_multithread(connfd, key_id);
            }
            break;
        }
        case 35: // Red-Black Tree
        {
            for (i = 0; i < ctx->operations_per_thread; i++) {
                // 为每个线程生成唯一的键，避免键冲突
                int key_id = i + (ctx->thread_id * ctx->operations_per_thread);
                rbtree_test_case_multithread(connfd, key_id);
            }
            break;
        }
        case 36: // Hash Table
        {
            for (i = 0; i < ctx->operations_per_thread; i++) {
                // 为每个线程生成唯一的键，避免键冲突
                int key_id = i + (ctx->thread_id * ctx->operations_per_thread);
                hash_test_case_multithread(connfd, key_id);
            }
            break;
        }
        case 37: // SkipTable
        {
            for (i = 0; i < ctx->operations_per_thread; i++) {
                // 为每个线程生成唯一的键，避免键冲突
                int key_id = i + (ctx->thread_id * ctx->operations_per_thread);
                skiptable_test_case_multithread(connfd, key_id);
            }
            break;
        }
        case 38:
        {
            for (i = 0; i < ctx->operations_per_thread; i++) {
                // 为每个线程生成唯一的键，避免键冲突
                int key_id = i + (ctx->thread_id * ctx->operations_per_thread);
                array_test_case_multithread(connfd, key_id);
            }
            break;
        }
        case 39:
        {
            for (i = 0; i < ctx->operations_per_thread; i++) {
                // 为每个线程生成唯一的键，避免键冲突
                int key_id = i + (ctx->thread_id * ctx->operations_per_thread);
                rbtree_test_case_multithread(connfd, key_id);
            }
            break;
        }
        case 40:
        {
            for (i = 0; i < ctx->operations_per_thread; i++) {
                // 为每个线程生成唯一的键，避免键冲突
                int key_id = i + (ctx->thread_id * ctx->operations_per_thread);
                hash_test_case_multithread(connfd, key_id);
            }
            break;
        }
        case 41:
        {
            for (i = 0; i < ctx->operations_per_thread; i++) {
                // 为每个线程生成唯一的键，避免键冲突
                int key_id = i + (ctx->thread_id * ctx->operations_per_thread);
                skiptable_test_case_multithread(connfd, key_id);
            }
            break;
        }
        case 42:
        {
            /*Test Save Array to Disk multi threads*/
            array_save_test_multithread(connfd, ctx);
            break;
        }
        case 43:
        {
			/*Test Save Red-Black Tree to Disk multi threads*/
            rbtree_save_test_multithread(connfd, ctx);
            break;
        }
        case 44:
        {
            /*Test Save Hash Table to Disk multi threads*/
            hash_save_test_multithread(connfd, ctx);
            break;
        }
        case 45:
        {
            /*Test Save SkipTable to Disk multi threads*/
            skiptable_save_test_multithread(connfd, ctx);
            break;
        }
        case 46:
        {
            /*Array Load Test multi threads*/
			array_load_test_multithread(connfd, ctx);
            break;
        }
		case 47:
		{
			/*Red-Black Tree Load Test multi threads*/
            rbtree_load_test_multithread(connfd, ctx);
			break;
		}
		case 48:
		{
			/*Hash Load Test multi threads*/
            hash_load_test_multithread(connfd, ctx);
			break;
		}
		case 49:
		{
			/*SkipTable Load Test multi threads*/
            skiptable_load_test_multithread(connfd, ctx);
			break;
		}
        case 50:
        {
            array_multiple_commands_test_multithread(connfd, ctx);
            break;
        }
        case 51:
        {
            rbtree_multiple_commands_test_multithread(connfd, ctx);
            break;
        }
        case 52:
        {
            hash_multiple_commands_test_multithread(connfd, ctx);
            break;
        }
        case 53:
        {
            skiptable_multiple_commands_test_multithread(connfd, ctx);
            break;
        }
        case 54:
        {
            array_range_test_multithread(connfd, ctx);
            break;
        }
        case 55:
        {
            rbtree_range_test_multithread(connfd, ctx);
            break;
        }
        case 56:
        {
            hash_range_test_multithread(connfd, ctx);
            break;
        }
        case 57:
        {
            skiptable_range_test_multithread(connfd, ctx);
            break;
        }
        case 58:
        {
			array_sync_test_source_multithread(connfd, ctx);
			break;
        }
        case 59:
        {
			array_sync_test_dest_multithread(connfd, ctx);
            break;
        }
        case 60:
        {
			rbtree_sync_test_source_multithread(connfd, ctx);
            break;
        }
        case 61:
        {
			rbtree_sync_test_dest_multithread(connfd, ctx);
			break;
        }
        case 62:
        {
			hash_sync_test_source_multithread(connfd, ctx);
            break;
        }
        case 63:
        {
			hash_sync_test_dest_multithread(connfd, ctx);
			break;
        }
        case 64:
        {
			skiptable_sync_test_source_multithread(connfd, ctx);
			break;
        }
        case 65:
        {
			skiptable_sync_test_dest_multithread(connfd, ctx);
            break;
        }


        if (ctx->failed) {
            break;
        }
    }
    
}

// 线程入口函数
void* thread_test_entry(void* arg)
{
    test_context_t* ctx = (test_context_t*)arg;
    if (!ctx) {
        printf("Invalid thread context\n");
        return NULL;
    }

    // 等待所有线程准备就绪
    sem_wait(ctx->start_sem);

    // 每个线程创建独立的连接
    int connfd = connect_tcpserver(ctx->serverip, ctx->port);
    if (connfd < 0) {
        printf("Thread %d failed to connect to server\n", ctx->thread_id);
        return NULL;
    }

    printf("Thread %d started, performing %d operations\n",
        ctx->thread_id, ctx->operations_per_thread);

    // 执行测试操作
    perform_engine_operations(connfd, ctx);

    close(connfd);
    printf("Thread %d completed\n", ctx->thread_id);
    return NULL;
}

//array: 1, rbtree: 2, hashtable: 3, skiptable: 4
//array_huge_keys: 5, rbtree_huge_keys: 6, hashtable_huge_keys: 7, skiptable_huge_keys: 8
//test_qps_tcpclient -s 127.0.0.1 -p 2048 -m 
int main(int argc, char *argv[])
{
    print_banner();
    print_usage(argv[0]);
    if (argc < 9) {
        //print_usage(argv[0]);
        printf("Invalid parameters, please check Usage!\n");
        return -1;
    }
    int ret = 0;
    int opt;
    test_context_t ctx = {0};
    int num_threads = 1;
    while((opt = getopt(argc, argv, "s:p:m:n:t:?")) != -1){
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
            case 't':
				printf("-t : %s\n", optarg);
				num_threads = atoi(optarg);
                if (num_threads < 1) num_threads = 1;
				break;
            default:
                printf("getopt error, unknown opt\n");
                exit(1);
        }
    }

    if (ctx.mode >= 34) {
        struct timeval start, end;
        pthread_t* threads;
        test_context_t* thread_ctxs;
        sem_t start_sem;

        ctx.engine_type = ctx.mode - 34;
        printf("Starting multi-thread test for engine type %d\n", ctx.engine_type);
        printf("Number of threads: %d, operations per thread: %d\n",
            num_threads, ctx.repeat_num);

        sem_init(&start_sem, 0, 0);

        threads = (pthread_t*)malloc(num_threads * sizeof(pthread_t));
        thread_ctxs = (test_context_t*)malloc(num_threads * sizeof(test_context_t));
        if (!threads || !thread_ctxs) {
            perror("Memory allocation failed");
            return -1;
        }

        switch (ctx.mode) {
            case 42:
            case 43:
            case 44:
            case 45:
            case 46:
            case 47:
            case 48:
            case 49: {
                printf("Please enter filename: ");
                scanf("%s", ctx.filename);
                printf("filename: %s\n", ctx.filename);
                break;
            }
            case 54:
            case 55:
            case 56:
            case 57: {
                printf("Please enter start range: ");
                scanf("%d", &ctx.start_range);
				printf("start range: %d\n", ctx.start_range);
				printf("Please enter end range: ");
				scanf("%d", &ctx.end_range);
				printf("end range: %d\n", ctx.end_range);
                printf("Please enter start num: ");
				scanf("%d", &ctx.start_num);
				printf("start num: %d\n", ctx.start_num);
				printf("Please enter end num: ");
				scanf("%d", &ctx.end_num);
				printf("end num: %d\n", ctx.end_num);
                break;
            }
            case 59:
            case 61:
            case 63:
            case 65: {
                printf("Please Enter Sync IP: ");
                scanf("%s", ctx.syncip);
                printf("Please Enter Sync Port: ");
                scanf("%d", &ctx.syncport);
                break;
            }
        }

        for (int i = 0; i < num_threads; i++) {
            thread_ctxs[i] = ctx;
            thread_ctxs[i].thread_id = i;
            thread_ctxs[i].start_sem = &start_sem;
            thread_ctxs[i].operations_per_thread = ctx.repeat_num;
            thread_ctxs[i].failed = 0;
            if (ctx.filename) {
                strcpy(thread_ctxs[i].filename, ctx.filename);
            }
        }



        // 创建线程
        for (int i = 0; i < num_threads; i++) {
            ret = pthread_create(&threads[i], NULL, thread_test_entry, &thread_ctxs[i]);
            if (ret != 0) {
                perror("pthread_create failed");
                num_threads = i; // 只等待已创建的线程
                break;
            }
        }

        // 等待所有线程准备就绪后同时开始
        gettimeofday(&start, NULL);
        for (int i = 0; i < num_threads; i++) {
            sem_post(&start_sem); // 释放信号量，允许线程开始
        }

        // 等待所有线程完成
        for (int i = 0; i < num_threads; i++) {
            pthread_join(threads[i], NULL);
        }
        gettimeofday(&end, NULL);

        // 计算并输出结果
        int time_used = TIME_SUB_MS(end, start);
        printf("\nMulti-thread test completed:\n");
        printf("Total time: %dms\n", time_used);
        switch (ctx.mode) {
            case 34:
            case 35:
            case 36:
            case 37: 
            case 38:
            case 39:
            case 40:
            case 41:{
                g_status.total_operations = num_threads * ctx.repeat_num * 10;
                printf("Total operations: %d\n", g_status.total_operations);
                printf("QPS: %.2f\n", time_used > 0 ?
                    (g_status.total_operations * 1000.0 / time_used) : 0);
                break;
            }
        }
        

        // 清理资源
        sem_destroy(&start_sem);
        free(threads);
        free(thread_ctxs);
        return 0;
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
        gettimeofday(&end, NULL);
        int time_used = TIME_SUB_MS(end, start);
        printf("time: %dms, qps: %d\n", time_used, ctx.repeat_num * 10 * 1000 / time_used);
    }
    if (2 == ctx.mode) {
        printf("rbtree test case\n");
        for(int i = 0; i < ctx.repeat_num; i++){
        #if ENABLE_RBTREE_KV_ENGINE
            rbtree_test_case(connfd);
        #endif
        }
        gettimeofday(&end, NULL);
        int time_used = TIME_SUB_MS(end, start);
        printf("time: %dms, qps: %d\n", time_used, ctx.repeat_num * 10 * 1000 / time_used);
    }
    if (3 == ctx.mode) {
        printf("hashtable test case\n");
        for(int i = 0; i < ctx.repeat_num; i++){
        #if ENABLE_HASH_KV_ENGINE
            hash_test_case(connfd);
        #endif
        }
        gettimeofday(&end, NULL);
        int time_used = TIME_SUB_MS(end, start);
        printf("time: %dms, qps: %d\n", time_used, ctx.repeat_num * 10 * 1000 / time_used);
    }
    if (4 == ctx.mode) {
        printf("skiptable test case\n");
        for(int i = 0; i < ctx.repeat_num; i++){
        #if ENABLE_SKIPTABLE_KV_ENGINE
            skiptable_test_case(connfd);
        #endif
        }
        gettimeofday(&end, NULL);
        int time_used = TIME_SUB_MS(end, start);
        printf("time: %dms, qps: %d\n", time_used, ctx.repeat_num * 10 * 1000 / time_used);
    }
    if (5 == ctx.mode) {
        printf("array huge keys test case\n");
    #if ENABLE_ARRAY_KV_ENGINE
        array_test_case_huge_keys(connfd, ctx.repeat_num);
    #endif
        gettimeofday(&end, NULL);
        int time_used = TIME_SUB_MS(end, start);
        printf("time: %dms, qps: %d\n", time_used, ctx.repeat_num * 10 * 1000 / time_used);
    }
    if (6 == ctx.mode) {
        printf("rbtree huge keys test case\n");
    #if ENABLE_RBTREE_KV_ENGINE
        rbtree_test_case_huge_keys(connfd, ctx.repeat_num);
    #endif
        gettimeofday(&end, NULL);
        int time_used = TIME_SUB_MS(end, start);
        printf("time: %dms, qps: %d\n", time_used, ctx.repeat_num * 10 * 1000 / time_used);
    }
    if (7 == ctx.mode) {
        printf("hashtable huge keys test case\n");
    #if ENABLE_HASH_KV_ENGINE
        hash_test_case_huge_keys(connfd, ctx.repeat_num);
    #endif
        gettimeofday(&end, NULL);
        int time_used = TIME_SUB_MS(end, start);
        printf("time: %dms, qps: %d\n", time_used, ctx.repeat_num * 10 * 1000 / time_used);
    }
    if (8 == ctx.mode) {
        printf("skiptable huge keys test case\n");
    #if ENABLE_SKIPTABLE_KV_ENGINE
        skiptable_test_case_huge_keys(connfd, ctx.repeat_num);
    #endif
        gettimeofday(&end, NULL);
        int time_used = TIME_SUB_MS(end, start);
        printf("time: %dms, qps: %d\n", time_used, ctx.repeat_num * 10 * 1000 / time_used);
    }
    if (9 == ctx.mode) {
        printf("save array to harddisk test case\n");
        printf("Please Enter filename: ");
        scanf("%s", ctx.filename);
        array_save_test(connfd, ctx.repeat_num, ctx.filename);
        gettimeofday(&end, NULL);
        int time_used = TIME_SUB_MS(end, start);
        printf("time: %dms, qps: %d\n", time_used, ctx.repeat_num * 1000 / time_used);
    }
    if (10 == ctx.mode) {
        printf("save rbtree to harddisk test case\n");
        printf("Please Enter filename: ");
        scanf("%s", ctx.filename);
        rbtree_save_test(connfd, ctx.repeat_num, ctx.filename);
        gettimeofday(&end, NULL);
        int time_used = TIME_SUB_MS(end, start);
        printf("time: %dms, qps: %d\n", time_used, ctx.repeat_num * 1000 / time_used);
    }
    if (11 == ctx.mode) {
        printf("save hashtable to harddisk test case\n");
        printf("Please Enter filename: ");
        scanf("%s", ctx.filename);
        hash_save_test(connfd, ctx.repeat_num, ctx.filename);
        gettimeofday(&end, NULL);
        int time_used = TIME_SUB_MS(end, start);
        printf("time: %dms, qps: %d\n", time_used, ctx.repeat_num * 1000 / time_used);
    }
    if (12 == ctx.mode) {
        printf("save skiptable to harddisk test case\n");
        printf("Please Enter filename: ");
        scanf("%s", ctx.filename);
        skiptable_save_test(connfd, ctx.repeat_num, ctx.filename);
        gettimeofday(&end, NULL);
        int time_used = TIME_SUB_MS(end, start);
        printf("time: %dms, qps: %d\n", time_used, ctx.repeat_num * 1000 / time_used);
    }
    if (13 == ctx.mode) {
        printf("load array from harddisk test case\n");
        printf("Please Enter filename: ");
        scanf("%s", ctx.filename);
        array_load_test(connfd, ctx.repeat_num, ctx.filename);
        gettimeofday(&end, NULL);
        int time_used = TIME_SUB_MS(end, start);
        printf("time: %dms, qps: %d\n", time_used, ctx.repeat_num * 2 * 1000 / time_used);
    }
    if (14 == ctx.mode) {
        printf("load rbtree from harddisk test case\n");
        printf("Please Enter filename: ");
        scanf("%s", ctx.filename);
        rbtree_load_test(connfd, ctx.repeat_num, ctx.filename);
        gettimeofday(&end, NULL);
        int time_used = TIME_SUB_MS(end, start);
        printf("time: %dms, qps: %d\n", time_used, ctx.repeat_num * 2 * 1000 / time_used);
    }
    if (15 == ctx.mode) {
        printf("load hashtable from harddisk test case\n");
        printf("Please Enter filename: ");
        scanf("%s", ctx.filename);
        hash_load_test(connfd, ctx.repeat_num, ctx.filename);
        gettimeofday(&end, NULL);
        int time_used = TIME_SUB_MS(end, start);
        printf("time: %dms, qps: %d\n", time_used, ctx.repeat_num * 2 * 1000 / time_used);
    }
    if (16 == ctx.mode) {
        printf("load skiptable from harddisk test case\n");
        printf("Please Enter filename: ");
        scanf("%s", ctx.filename);
        skiptable_load_test(connfd, ctx.repeat_num, ctx.filename);
        gettimeofday(&end, NULL);
        int time_used = TIME_SUB_MS(end, start);
        printf("time: %dms, qps: %d\n", time_used, ctx.repeat_num * 2 * 1000 / time_used);
    }
    if (17 == ctx.mode) {
        printf("Test Array KV Engine Multiple Commands\n");
        array_multiple_commands_test(connfd, ctx.repeat_num);
        gettimeofday(&end, NULL);
        int time_used = TIME_SUB_MS(end, start);
        printf("time: %dms, qps: %d\n", time_used, ctx.repeat_num * 10 * 1000 / time_used);
    }
    if (18 == ctx.mode) {
        printf("Test Rbtree KV Engine Multiple Commands\n");
        rbtree_multiple_commands_test(connfd, ctx.repeat_num);
        gettimeofday(&end, NULL);
        int time_used = TIME_SUB_MS(end, start);
        printf("time: %dms, qps: %d\n", time_used, ctx.repeat_num * 10 * 1000 / time_used);
    }
    if (19 == ctx.mode) {
        printf("Test Hash KV Engine Multiple Commands\n");
        hash_multiple_commands_test(connfd, ctx.repeat_num);
        gettimeofday(&end, NULL);
        int time_used = TIME_SUB_MS(end, start);
        printf("time: %dms, qps: %d\n", time_used, ctx.repeat_num * 10 * 1000 / time_used);
    }
    if (20 == ctx.mode) {
        printf("Test Skiptable KV Engine Multiple Commands\n");
        skiptable_multiple_commands_test(connfd, ctx.repeat_num);
        gettimeofday(&end, NULL);
        int time_used = TIME_SUB_MS(end, start);
        printf("time: %dms, qps: %d\n", time_used, ctx.repeat_num * 10 * 1000 / time_used);
    }
    if (21 == ctx.mode) {
        printf("Test All Engine Multiple Commands Test\n");
        all_engine_multiple_commands_test(connfd, ctx.repeat_num);
        gettimeofday(&end, NULL);
        int time_used = TIME_SUB_MS(end, start);
        printf("time: %dms, qps: %d\n", time_used, ctx.repeat_num * 40 * 1000 / time_used);
    }
    if (22 == ctx.mode) {
        printf("Test Array Range Command Test\n");
        printf("Please Enter start range: ");
        int start_range = 0;
        scanf("%d", &start_range);
        printf("Please Enter end range: ");
        int end_range = 0;
        scanf("%d", &end_range);
        int start_num = 0;
        printf("Please Enter start num: ");
        scanf("%d", &start_num);
        int end_num = 0;
        printf("Please Enter end num: ");
        scanf("%d", &end_num);
        printf("SET Range[%d, %d]\n", start_range, end_range);
        printf("GET Range[%d, %d]\n", start_num, end_num);
        array_range_test(connfd, start_range, end_range, start_num, end_num);
    }
    if (23 == ctx.mode) {
        printf("Test Red-Black Tree Range Command Test\n");
        printf("Please Enter start range: ");
        int start_range = 0;
        scanf("%d", &start_range);
        printf("Please Enter end range: ");
        int end_range = 0;
        scanf("%d", &end_range);
        int start_num = 0;
        printf("Please Enter start num: ");
        scanf("%d", &start_num);
        int end_num = 0;
        printf("Please Enter end num: ");
        scanf("%d", &end_num);
        printf("RSET Range[%d, %d]\n", start_range, end_range);
        printf("RGET Range[%d, %d]\n", start_num, end_num);
        rbtree_range_test(connfd, start_range, end_range, start_num, end_num);
    }
    if (24 == ctx.mode) {
        printf("Test Hash Range Command Test\n");
        printf("Please Enter start range: ");
        int start_range = 0;
        scanf("%d", &start_range);
        printf("Please Enter end range: ");
        int end_range = 0;
        scanf("%d", &end_range);
        int start_num = 0;
        printf("Please Enter start num: ");
        scanf("%d", &start_num);
        int end_num = 0;
        printf("Please Enter end num: ");
        scanf("%d", &end_num);
        printf("HSET Range[%d, %d]\n", start_range, end_range);
        printf("HGET Range[%d, %d]\n", start_num, end_num);
        hash_range_test(connfd, start_range, end_range, start_num, end_num);
    }
    if (25 == ctx.mode) {
        printf("Test Skiptable Range Command Test\n");
        printf("Please Enter start range: ");
        int start_range = 0;
        scanf("%d", &start_range);
        printf("Please Enter end range: ");
        int end_range = 0;
        scanf("%d", &end_range);
        int start_num = 0;
        printf("Please Enter start num: ");
        scanf("%d", &start_num);
        int end_num = 0;
        printf("Please Enter end num: ");
        scanf("%d", &end_num);
        printf("SSET Range[%d, %d]\n", start_range, end_range);
        printf("SGET Range[%d, %d]\n", start_num, end_num);
        skiptable_range_test(connfd, start_range, end_range, start_num, end_num);
    }
    if (26 == ctx.mode) {
        printf("Test Array Sync Command[Source]\n");
        /*被同步服务器，仅设定本地数据以及接受GETARRAY请求后返回数据*/
        array_sync_test_source(connfd, ctx.repeat_num);
    }
    if (27 == ctx.mode) {
        printf("Test Array Sync Command[Dest]\n");
        /*同步服务器，发送SYNC + 目标服务器IP + 目标服务器端口并接受数据后存入本地，然后验证*/
        char source_ip[128] = {0};
        int source_port = -1;
        printf("Please Enter Source IP: ");
        scanf("%s", source_ip);
        printf("Please Enter Source Port: ");
        scanf("%d", &source_port);
        array_sync_test_dest(connfd, ctx.repeat_num, source_ip, source_port);
    }
    if (28 == ctx.mode) {
        printf("Test Rbtree Sync Command[Source]\n");
        rbtree_sync_test_source(connfd, ctx.repeat_num);
    }
    if (29 == ctx.mode) {
        printf("Test Rbtree Sync Command[Dest]\n");
        char source_ip[128] = {0};
        int source_port = -1;
        printf("Please Enter Source IP: ");
        scanf("%s", source_ip);
        printf("Please Enter Source Port: ");
        scanf("%d", &source_port);
        rbtree_sync_test_dest(connfd, ctx.repeat_num, source_ip, source_port);
    }
    if (30 == ctx.mode) {
        printf("Test Hash Sync Command[Source]\n");
        hash_sync_test_source(connfd, ctx.repeat_num);
    }
    if (31 == ctx.mode) {
        printf("Test Hash Sync Command[Dest]\n");
        char source_ip[128] = {0};
        int source_port = -1;
        printf("Please Enter Source IP: ");
        scanf("%s", source_ip);
        printf("Please Enter Source Port: ");
        scanf("%d", &source_port);
        hash_sync_test_dest(connfd, ctx.repeat_num, source_ip, source_port);
    }
    if (32 == ctx.mode) {
        printf("Test Skiptable Sync Command[Source]\n");
        skiptable_sync_test_source(connfd, ctx.repeat_num);
    }
    if (33 == ctx.mode) {
        printf("Test Skiptable Sync Command[Dest]\n");
        char source_ip[128] = {0};
        int source_port = -1;
        printf("Please Enter Source IP: ");
        scanf("%s", source_ip);
        printf("Please Enter Source Port: ");
        scanf("%d", &source_port);
        skiptable_sync_test_dest(connfd, ctx.repeat_num, source_ip, source_port);
    }

    close(connfd);
    connfd = -1;
    return 0;

}