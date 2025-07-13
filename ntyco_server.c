/*
 *  Author : WangBoJing , email : 1989wangbojing@gmail.com
 * 
 *  Copyright Statement:
 *  --------------------
 *  This software is protected by Copyright and the information contained
 *  herein is confidential. The software may not be copied and the information
 *  contained herein may not be used or disclosed except with the written
 *  permission of Author. (C) 2017
 * 
 *

****       *****                                      *****
  ***        *                                       **    ***
  ***        *         *                            *       **
  * **       *         *                           **        **
  * **       *         *                          **          *
  *  **      *        **                          **          *
  *  **      *       ***                          **
  *   **     *    ***********    *****    *****  **                   ****
  *   **     *        **           **      **    **                 **    **
  *    **    *        **           **      *     **                 *      **
  *    **    *        **            *      *     **                **      **
  *     **   *        **            **     *     **                *        **
  *     **   *        **             *    *      **               **        **
  *      **  *        **             **   *      **               **        **
  *      **  *        **             **   *      **               **        **
  *       ** *        **              *  *       **               **        **
  *       ** *        **              ** *        **          *   **        **
  *        ***        **               * *        **          *   **        **
  *        ***        **     *         **          *         *     **      **
  *         **        **     *         **          **       *      **      **
  *         **         **   *          *            **     *        **    **
*****        *          ****           *              *****           ****
                                       *
                                      *
                                  *****
                                  ****



 *
 */
 

#include "nty_coroutine.h"
#include "ntyco_server.h"
#include "kv_store_array.h"
#include "kv_log.h"
#include <arpa/inet.h>

#define MAX_CLIENT_NUM			1000000
#define TIME_SUB_MS(tv1, tv2)  ((tv1.tv_sec - tv2.tv_sec) * 1000 + (tv1.tv_usec - tv2.tv_usec) / 1000)
#define KVSTORE_MAX_TOKENS		128

const char* commands[COUNT] = {
    "SET", "GET", "DEL", "MOD",
};

int kvstore_parser_protocol(struct conn_item *item, char **tokens, int count)
{

	if (count < 1) {
		return -1;
	}

	int cmd = START;
	for(cmd = START; cmd < COUNT; ++cmd){
		if(strcmp(tokens[0], commands[cmd]) == 0){
			break;
		}
	}
	int res = 0;
	char *value = NULL;
	memset(item->wbuffer, 0, sizeof(item->wbuffer));
	switch(cmd) {
		case SET:
			KV_LOG("SET\n");
			if(count < 3){
				KV_LOG("invalid set command\n");
				snprintf(item->wbuffer, sizeof(item->wbuffer), "FAILED");
				return -1;
			}
			res = kvs_array_set(tokens[1], tokens[2]);
			if(res == 0){
				snprintf(item->wbuffer, sizeof(item->wbuffer), "SUCCESS");
			}
			break;
		case GET:
			KV_LOG("GET\n");
			value = kvs_array_get(tokens[1]);
			if(value){
				KV_LOG("GET success : %s\n", value);
				snprintf(item->wbuffer, sizeof(item->wbuffer), "%s", value);
			}else{
				snprintf(item->wbuffer, sizeof(item->wbuffer), "NO EXIST");
			}
			break;
		case DEL:
			KV_LOG("DEL\n");
		 	res = kvs_array_delete(tokens[1]);
			if (res < 0) {
				snprintf(item->wbuffer, sizeof(item->wbuffer), "ERROR");
			} else if (res == 0) {
				snprintf(item->wbuffer, sizeof(item->wbuffer), "SUCCESS");
			} else {
				snprintf(item->wbuffer, sizeof(item->wbuffer), "NO EXIST");
			}
			break;
		case MOD:
			KV_LOG("MOD\n");
			res = kvs_array_modify(tokens[1], tokens[2]);
			if (res < 0) {
				snprintf(item->wbuffer, sizeof(item->wbuffer), "ERROR");
			} else if (res == 0) {
				snprintf(item->wbuffer, sizeof(item->wbuffer), "SUCCESS");
			} else {
				snprintf(item->wbuffer, sizeof(item->wbuffer), "NO EXIST");
			}
			break;
		default:
			KV_LOG("unknow command\n");
			snprintf(item->wbuffer, sizeof(item->wbuffer), "ERROR");
			return -1;
	}
	return 0;
}

int kv_store_split_token(char *msg, char** tokens)
{
	KV_LOG("kv_store_split_token\n");
	int idx = 0;
    char *token = strtok(msg, " ");
    while (token != NULL) {
        tokens[idx++] = token;
        token = strtok(NULL, " ");
    }
    return idx;
}

int kvstore_request(struct conn_item *item)
{
	KV_LOG("recv : %s\n", item->rbuffer);
	char *msg = item->rbuffer;
	char *tokens[KVSTORE_MAX_TOKENS];

	int count = kv_store_split_token(msg, tokens);
	KV_LOG("count = %d\n", count);

	int idx = 0;
	for (idx = 0; idx < count; idx++) {
		KV_LOG("idx: %d, content: %s\n",idx, tokens[idx]);
	}

	kvstore_parser_protocol(item, tokens, count);

	return 0;
}


void server_reader(void *arg) {
	int fd = *(int *)arg;
	free(arg);
	int ret = 0;

 
	struct pollfd fds;
	fds.fd = fd;
	fds.events = POLLIN;

	while (1) {
#if 0	
		char buf[1024] = {0};
		ret = nty_recv(fd, buf, 1024, 0);
		if (ret > 0) {
			if(fd > MAX_CLIENT_NUM) 
			printf("read from server: %.*s\n", ret, buf);

			ret = nty_send(fd, buf, strlen(buf), 0);
			if (ret == -1) {
				nty_close(fd);
				break;
			}
		} else if (ret == 0) {	
			nty_close(fd);
			break;
		}

#endif
		struct conn_item item = {0};
		ret = nty_recv(fd, item.rbuffer, BUFFER_LENGTH, 0);
		if (ret > 0) {
			if(fd > MAX_CLIENT_NUM) {
				KV_LOG("read from server: %.*s\n", ret, item.rbuffer);
			}
			
			kvstore_request(&item);
			item.wlen = strlen(item.wbuffer);

			ret = nty_send(fd, item.wbuffer, BUFFER_LENGTH, 0);
			if (ret == -1) {
				nty_close(fd);
				break;
			}
		} else if (ret == 0) {	
			nty_close(fd);
			break;
		}
	}
	
}


void server(void *arg) {

	unsigned short port = *(unsigned short *)arg;
	free(arg);

	int fd = nty_socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) return ;

	struct sockaddr_in local, remote;
	local.sin_family = AF_INET;
	local.sin_port = htons(port);
	local.sin_addr.s_addr = INADDR_ANY;
	bind(fd, (struct sockaddr*)&local, sizeof(struct sockaddr_in));

	listen(fd, 20);
	printf("listen port : %d\n", port);

	
	struct timeval tv_begin;
	gettimeofday(&tv_begin, NULL);

	while (1) {
		socklen_t len = sizeof(struct sockaddr_in);
		int cli_fd = nty_accept(fd, (struct sockaddr*)&remote, &len);
		if (cli_fd % 1000 == 999) {

			struct timeval tv_cur;
			memcpy(&tv_cur, &tv_begin, sizeof(struct timeval));
			
			gettimeofday(&tv_begin, NULL);
			int time_used = TIME_SUB_MS(tv_begin, tv_cur);
			
			printf("client fd : %d, time_used: %d\n", cli_fd, time_used);
		}
		printf("new client comming\n");

		nty_coroutine *read_co;
		int *arg = malloc(sizeof(int));
		*arg = cli_fd;
		nty_coroutine_create(&read_co, server_reader, arg);

	}
	
}



int ntyco_entry(int port) {
	printf("ntyco_entry\n");
	nty_coroutine *co = NULL;

	int i = 0;
	unsigned short base_port = port;
	for (i = 0;i < 1;i ++) {
		unsigned short *port = calloc(1, sizeof(unsigned short));
		*port = base_port + i;
		nty_coroutine_create(&co, server, port); ////////no run
	}

	nty_schedule_run(); //run

	return 0;
}



