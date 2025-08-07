#ifndef __KV_STORE_EPOLL_HPP__
#define __KV_STORE_EPOLL_HPP__

#include "kv_store.hpp"
#include "kv_log.h"
#include "kv_save.h"

#ifdef __cplusplus

#include <queue>

using namespace std;

class KVStoreEpollConnection : public IConnection, public std::enable_shared_from_this<KVStoreEpollConnection> 
{
public:
    KVStoreEpollConnection(int fd) : IConnection(fd), 
                                    r_buf(),
                                    w_buf(),
                                    send_queue_(),
                                    current_send_offset_(0) 
    {
        r_buf.reserve(4096); /*预分配接收缓冲区*/
        w_buf.reserve(4096); /*预分配发送缓冲区*/
    }

private:
    constexpr static int MAX_TOKENS = 128;
    #if 0
    int 
    split_token() 
    {
        int idx = 0;
        char *token = strtok(r_buf, " ");

        while (token != NULL) {
            tokens[idx++] = token;
            token = strtok(NULL, " ");
        }
        return idx;
    }
    
    int 
    parse_protocol(int count) 
    {
        if (count < 1) {
            return -1;
        }

        int cmd = static_cast<int>(Command::START);
        for(cmd = static_cast<int>(Command::START); cmd < static_cast<int>(Command::COUNT); ++cmd){
            if(strcmp(tokens[0], commands[cmd]) == 0){
                break;
            }
        }
        int res = 0;
        char *value = nullptr;
        switch(cmd) {
            #if ENABLE_ARRAY_KV_ENGINE
            /*array*/
            case static_cast<int>(Command::SET):
                KV_LOG("SET\n");
                if(count < 3){
                    KV_LOG("invalid set command\n");
                    snprintf(w_buf, sizeof(w_buf), "FAILED");
                    return -1;
                }
                res = kvs_array_set(&global_array, tokens[1], tokens[2]);
                if(res == 0){
                    snprintf(w_buf, sizeof(w_buf), "SUCCESS");
                } else if (res < 0) {
                    snprintf(w_buf, sizeof(w_buf), "ERROR");
                } else {
                    snprintf(w_buf, sizeof(w_buf), "EXIST");
                }
                break;
            case static_cast<int>(Command::GET):
                KV_LOG("GET\n");
                value = kvs_array_get(&global_array, tokens[1]);
                if(value){
                    KV_LOG("GET success : %s\n", value);
                    snprintf(w_buf, sizeof(w_buf), "%s", value);
                }else{
                    snprintf(w_buf, sizeof(w_buf), "NO EXIST");
                }
                break;
            case static_cast<int>(Command::DEL):
                KV_LOG("DEL\n");
                res = kvs_array_delete(&global_array, tokens[1]);
                if (res < 0) {
                    snprintf(w_buf, sizeof(w_buf), "ERROR");
                } else if (res == 0) {
                    snprintf(w_buf, sizeof(w_buf), "SUCCESS");
                } else {
                    snprintf(w_buf, sizeof(w_buf), "NO EXIST");
                }
                break;
            case static_cast<int>(Command::MOD):
                KV_LOG("MOD\n");
                res = kvs_array_modify(&global_array, tokens[1], tokens[2]);
                //printf("kvs_array_modify res : %d\n", res);
                if (res < 0) {
                    snprintf(w_buf, sizeof(w_buf), "ERROR");
                } else if (res == 0) {
                    snprintf(w_buf, sizeof(w_buf), "SUCCESS");
                } else {
                    snprintf(w_buf, sizeof(w_buf), "NO EXIST");
                }
                break;
            case static_cast<int>(Command::EXIST):
                KV_LOG("EXIST\n");
                res = kvs_array_exist(&global_array, tokens[1]);
                if (res == 0) {
                    snprintf(w_buf, sizeof(w_buf), "EXIST");
                } else {
                    snprintf(w_buf, sizeof(w_buf), "NO EXIST");
                }
                break;
            case static_cast<int>(Command::SAVE):
                KV_LOG("SAVE\n");
                res = kvs_array_save(tokens[1]);
                if (res == 0) {
                    snprintf(w_buf, sizeof(w_buf), "SUCCESS");
                } else {
                    snprintf(w_buf, sizeof(w_buf), "ERROR");
                }
                break;
            case static_cast<int>(Command::LOAD):
                KV_LOG("LOAD\n");
                res = kvs_array_load(tokens[1]);
                if (res == 0) {
                    snprintf(w_buf, sizeof(w_buf), "SUCCESS");
                } else {
                    snprintf(w_buf, sizeof(w_buf), "ERROR");
                }
                break;
            #endif
            #if ENABLE_RBTREE_KV_ENGINE
            /*rbtree*/
                case static_cast<int>(Command::RSET):
                KV_LOG("RSET\n");
                if(count < 3){
                    KV_LOG("invalid set command\n");
                    snprintf(w_buf, sizeof(w_buf), "FAILED");
                    return -1;
                }
                res = kvs_rbtree_set(&global_rbtree, tokens[1], tokens[2]);
                if(res == 0){
                    snprintf(w_buf, sizeof(w_buf), "SUCCESS");
                } else if (res < 0) {
                    snprintf(w_buf, sizeof(w_buf), "ERROR");
                } else {
                    snprintf(w_buf, sizeof(w_buf), "EXIST");
                }
                break;
            case static_cast<int>(Command::RGET):
                KV_LOG("GET\n");
                value = kvs_rbtree_get(&global_rbtree, tokens[1]);
                if(value){
                    KV_LOG("RGET success : %s\n", value);
                    snprintf(w_buf, sizeof(w_buf), "%s", value);
                }else{
                    snprintf(w_buf, sizeof(w_buf), "NO EXIST");
                }
                break;
            case static_cast<int>(Command::RDEL):
                KV_LOG("RDEL\n");
                res = kvs_rbtree_delete(&global_rbtree, tokens[1]);
                if (res < 0) {
                    snprintf(w_buf, sizeof(w_buf), "ERROR");
                } else if (res == 0) {
                    snprintf(w_buf, sizeof(w_buf), "SUCCESS");
                } else {
                    snprintf(w_buf, sizeof(w_buf), "NO EXIST");
                }
                break;
            case static_cast<int>(Command::RMOD):
                KV_LOG("RMOD\n");
                res = kvs_rbtree_modify(&global_rbtree, tokens[1], tokens[2]);
                if (res < 0) {
                    snprintf(w_buf, sizeof(w_buf), "ERROR");
                } else if (res == 0) {
                    snprintf(w_buf, sizeof(w_buf), "SUCCESS");
                } else {
                    snprintf(w_buf, sizeof(w_buf), "NO EXIST");
                }
                break;
            case static_cast<int>(Command::REXIST):
                KV_LOG("REXIST\n");
                res = kvs_rbtree_exist(&global_rbtree, tokens[1]);
                if (res == 0) {
                    snprintf(w_buf, sizeof(w_buf), "EXIST");
                } else {
                    snprintf(w_buf, sizeof(w_buf), "NO EXIST");
                }
                break;
            case static_cast<int>(Command::RSAVE):
                KV_LOG("RSAVE\n");
                res = kvs_rbtree_save(tokens[1]);
                if (res == 0) {
                    snprintf(w_buf, sizeof(w_buf), "SUCCESS");
                } else {
                    snprintf(w_buf, sizeof(w_buf), "ERROR");
                }
                break;
            case static_cast<int>(Command::RLOAD):
                KV_LOG("RLOAD\n");
                res = kvs_rbtree_load(tokens[1]);
                if (res == 0) {
                    snprintf(w_buf, sizeof(w_buf), "SUCCESS");
                } else {
                    snprintf(w_buf, sizeof(w_buf), "ERROR");
                }
                break;
            #endif
            #if ENABLE_HASH_KV_ENGINE
            /*hash*/
            case static_cast<int>(Command::HSET):
                KV_LOG("HSET\n");
                if(count < 3){
                    KV_LOG("invalid set command\n");
                    snprintf(w_buf, sizeof(w_buf), "FAILED");
                    return -1;
                }
                res = kvs_hash_set(&global_hash, tokens[1], tokens[2]);
                if(res == 0){
                    snprintf(w_buf, sizeof(w_buf), "SUCCESS");
                } else if (res < 0) {
                    snprintf(w_buf, sizeof(w_buf), "ERROR");
                } else {
                    snprintf(w_buf, sizeof(w_buf), "EXIST");
                }
                break;
            case static_cast<int>(Command::HGET):
                KV_LOG("HGET\n");
                value = kvs_hash_get(&global_hash, tokens[1]);
                if(value){
                    KV_LOG("HGET success : %s\n", value);
                    snprintf(w_buf, sizeof(w_buf), "%s", value);
                }else{
                    snprintf(w_buf, sizeof(w_buf), "NO EXIST");
                }
                break;
            case static_cast<int>(Command::HDEL):
                KV_LOG("HDEL\n");
                res = kvs_hash_delete(&global_hash, tokens[1]);
                if (res < 0) {
                    snprintf(w_buf, sizeof(w_buf), "ERROR");
                } else if (res == 0) {
                    snprintf(w_buf, sizeof(w_buf), "SUCCESS");
                } else {
                    snprintf(w_buf, sizeof(w_buf), "NO EXIST");
                }
                break;
            case static_cast<int>(Command::HMOD):
                KV_LOG("HMOD\n");
                res = kvs_hash_modify(&global_hash, tokens[1], tokens[2]);
                if (res < 0) {
                    snprintf(w_buf, sizeof(w_buf), "ERROR");
                } else if (res == 0) {
                    snprintf(w_buf, sizeof(w_buf), "SUCCESS");
                } else {
                    snprintf(w_buf, sizeof(w_buf), "NO EXIST");
                }
                break;
            case static_cast<int>(Command::HEXIST):
                KV_LOG("HEXIST\n");
                res = kvs_hash_exist(&global_hash, tokens[1]);
                if (res == 0) {
                    snprintf(w_buf, sizeof(w_buf), "EXIST");
                } else {
                    snprintf(w_buf, sizeof(w_buf), "NO EXIST");
                }
                break;
            case static_cast<int>(Command::HSAVE):
                KV_LOG("HSAVE\n");
                res = kvs_hash_save(tokens[1]);
                if (res == 0) {
                    snprintf(w_buf, sizeof(w_buf), "SUCCESS");
                } else {
                    snprintf(w_buf, sizeof(w_buf), "ERROR");
                }
                break;
            case static_cast<int>(Command::HLOAD):
                KV_LOG("HLOAD\n");
                res = kvs_hash_load(tokens[1]);
                if (res == 0) {
                    snprintf(w_buf, sizeof(w_buf), "SUCCESS");
                } else {
                    snprintf(w_buf, sizeof(w_buf), "ERROR");
                }
                break;
            #endif
             #if ENABLE_SKIPTABLE_KV_ENGINE
            /*skiptable*/
            case static_cast<int>(Command::SSET):
                KV_LOG("SSET\n");
                if(count < 3){
                    KV_LOG("invalid set command\n");
                    snprintf(w_buf, sizeof(w_buf), "FAILED");
                    return -1;
                }
                res = kvs_skiptable_set(&global_skiptable, tokens[1], tokens[2]);
                if(res == 0){
                    snprintf(w_buf, sizeof(w_buf), "SUCCESS");
                } else if (res < 0) {
                    snprintf(w_buf, sizeof(w_buf), "ERROR");
                } else {
                    snprintf(w_buf, sizeof(w_buf), "EXIST");
                }
                break;
            case static_cast<int>(Command::SGET):
                KV_LOG("SGET\n");
                value = kvs_skiptable_get(&global_skiptable, tokens[1]);
                if(value){
                    KV_LOG("SGET success : %s\n", value);
                    snprintf(w_buf, sizeof(w_buf), "%s", value);
                }else{
                    snprintf(w_buf, sizeof(w_buf), "NO EXIST");
                }
                break;
            case static_cast<int>(Command::SDEL):
                KV_LOG("SDEL\n");
                res = kvs_skiptable_delete(&global_skiptable, tokens[1]);
                if (res < 0) {
                    snprintf(w_buf, sizeof(w_buf), "ERROR");
                } else if (res == 0) {
                    snprintf(w_buf, sizeof(w_buf), "SUCCESS");
                } else {
                    snprintf(w_buf, sizeof(w_buf), "NO EXIST");
                }
                break;
            case static_cast<int>(Command::SMOD):
                KV_LOG("SMOD\n");
                res = kvs_skiptable_modify(&global_skiptable, tokens[1], tokens[2]);
                if (res < 0) {
                    snprintf(w_buf, sizeof(w_buf), "ERROR");
                } else if (res == 0) {
                    snprintf(w_buf, sizeof(w_buf), "SUCCESS");
                } else {
                    snprintf(w_buf, sizeof(w_buf), "NO EXIST");
                }
                break;
            case static_cast<int>(Command::SEXIST):
                KV_LOG("SEXIST\n");
                res = kvs_skiptable_exist(&global_skiptable, tokens[1]);
                if (res == 0) {
                    snprintf(w_buf, sizeof(w_buf), "EXIST");
                } else {
                    snprintf(w_buf, sizeof(w_buf), "NO EXIST");
                }
                break;
            case static_cast<int>(Command::SSAVE):
                KV_LOG("SSAVE\n");
                res = kvs_skiptable_save(tokens[1]);
                if (res == 0) {
                    snprintf(w_buf, sizeof(w_buf), "SUCCESS");
                } else {
                    snprintf(w_buf, sizeof(w_buf), "ERROR");
                }
                break;
            case static_cast<int>(Command::SLOAD):
                KV_LOG("SLOAD\n");
                res = kvs_skiptable_load(tokens[1]);
                if (res == 0) {
                    snprintf(w_buf, sizeof(w_buf), "SUCCESS");
                } else {
                    snprintf(w_buf, sizeof(w_buf), "ERROR");
                }
                break;
            #endif
            default:
                KV_LOG("unknow command, echo...\n");
                snprintf(w_buf, sizeof(w_buf), r_buf);
                break;
                //return -1;
        }
        if(strlen(w_buf) > 0){

            Reactor::get_instance().modify_handler(fd_, EPOLLIN | EPOLLOUT | EPOLLET, shared_from_this());
        }
        return 0;
    }
    #endif

    std::string 
    process_command(const std::string& command)
    {
        char command_buf[1024] = {0};
        strncpy(command_buf, command.c_str(), sizeof(command_buf));
        command_buf[sizeof(command_buf) - 1] = '\0';
        
        int count = 0;
        char* token = strtok(command_buf, " ");
        while (token != NULL && count < MAX_TOKENS) {
            tokens[count++] = token;
            token = strtok(NULL, " ");
        }

        return parse_protocol_to_string(count);
    }

    std::string 
    parse_protocol_to_string(int count) 
    {
        if (count < 1) {
            KV_LOG("ERROR : empty command\n");
            return "";
        }
        char response_buf[1024] = {0};
        std::string response;

        int cmd = static_cast<int>(Command::START);
        for(cmd = static_cast<int>(Command::START); cmd < static_cast<int>(Command::COUNT); ++cmd){
            if(strcmp(tokens[0], commands[cmd]) == 0){
                break;
            }
        }
        int res = 0;
        char *value = nullptr;
        switch(cmd) {
            #if ENABLE_ARRAY_KV_ENGINE
            /*array*/
            case static_cast<int>(Command::SET):
                KV_LOG("SET\n");
                if(count < 3){
                    KV_LOG("invalid set command\n");
                    // snprintf(response_buf, sizeof(response_buf), "FAILED");
                    response = "FAILED\n";
                    return response;
                }
                res = kvs_array_set(&global_array, tokens[1], tokens[2]);
                if(res == 0){
                    snprintf(response_buf, sizeof(response_buf), "SUCCESS");
                    // response = "SUCCESS";
                } else if (res < 0) {
                    snprintf(response_buf, sizeof(response_buf), "ERROR");
                    // response = "ERROR";
                } else {
                    snprintf(response_buf, sizeof(response_buf), "EXIST");
                    // response = "EXIST";
                }
                break;
            case static_cast<int>(Command::GET):
                KV_LOG("GET\n");
                value = kvs_array_get(&global_array, tokens[1]);
                if(value){
                    KV_LOG("GET success : %s\n", value);
                    snprintf(response_buf, sizeof(response_buf), "%s", value);
                    // response = response_buf;
                }else{
                    snprintf(response_buf, sizeof(response_buf), "NO EXIST");
                }
                break;
            case static_cast<int>(Command::DEL):
                KV_LOG("DEL\n");
                res = kvs_array_delete(&global_array, tokens[1]);
                if (res < 0) {
                    snprintf(response_buf, sizeof(response_buf), "ERROR");
                } else if (res == 0) {
                    snprintf(response_buf, sizeof(response_buf), "SUCCESS");
                } else {
                    snprintf(response_buf, sizeof(response_buf), "NO EXIST");
                }
                break;
            case static_cast<int>(Command::MOD):
                KV_LOG("MOD\n");
                res = kvs_array_modify(&global_array, tokens[1], tokens[2]);
                //printf("kvs_array_modify res : %d\n", res);
                if (res < 0) {
                    snprintf(response_buf, sizeof(response_buf), "ERROR");
                } else if (res == 0) {
                    snprintf(response_buf, sizeof(response_buf), "SUCCESS");
                } else {
                    snprintf(response_buf, sizeof(response_buf), "NO EXIST");
                }
                break;
            case static_cast<int>(Command::EXIST):
                KV_LOG("EXIST\n");
                res = kvs_array_exist(&global_array, tokens[1]);
                if (res == 0) {
                    snprintf(response_buf, sizeof(response_buf), "EXIST");
                } else {
                    snprintf(response_buf, sizeof(response_buf), "NO EXIST");
                }
                break;
            case static_cast<int>(Command::SAVE):
                KV_LOG("SAVE\n");
                res = kvs_array_save(tokens[1]);
                if (res == 0) {
                    snprintf(response_buf, sizeof(response_buf), "SUCCESS");
                } else {
                    snprintf(response_buf, sizeof(response_buf), "ERROR");
                }
                break;
            case static_cast<int>(Command::LOAD):
                KV_LOG("LOAD\n");
                res = kvs_array_load(tokens[1]);
                if (res == 0) {
                    snprintf(response_buf, sizeof(response_buf), "SUCCESS");
                } else {
                    snprintf(response_buf, sizeof(response_buf), "ERROR");
                }
                break;
            case static_cast<int>(Command::RANGE):
            {
                KV_LOG("RANGE\n");
                kvs_item_t* res_array = NULL;
                int res_count = 0;
                res = kvs_array_range(&global_array, tokens[1], tokens[2], &res_array, &res_count);
                if (res == 0) {
                    KV_LOG("res == 0, res_count: %d\n", res_count);
                    if (res_array == NULL) {
                        KV_LOG("res_array == NULL\n");
                    }
                    for (int i = 0; i < res_count; i++) {
                        if (res_array[i].key == NULL) {
                            KV_LOG("res_array[%d].key == NULL\n", i);
                        }
                        response += res_array[i].key;
                        response += " ";
                        if (res_array[i].value == NULL) {
                            KV_LOG("res_array[%d].value == NULL\n", i);
                        }
                        response += res_array[i].value;
                        if (i != res_count - 1) {
                            response += "\n";
                        }
                        kvs_free(res_array[i].key);
                        kvs_free(res_array[i].value);
                        KV_LOG("response: %s\n", response.c_str());
                    }
                    kvs_free(res_array);
                    KV_LOG("RANGE success, response : %s\n", response.c_str());
                    return response;
                }
                else if (res > 0) {
                    snprintf(response_buf, sizeof(response_buf), "EMPTY");
                }
                else {
                    snprintf(response_buf, sizeof(response_buf), "ERROR");
                }
            }
                break;
            #endif
            #if ENABLE_RBTREE_KV_ENGINE
            /*rbtree*/
                case static_cast<int>(Command::RSET):
                KV_LOG("RSET\n");
                if(count < 3){
                    KV_LOG("invalid set command\n");
                    // snprintf(response_buf, sizeof(response_buf), "FAILED");
                    response = "FAILED\n";
                    return response;
                }
                res = kvs_rbtree_set(&global_rbtree, tokens[1], tokens[2]);
                if(res == 0){
                    snprintf(response_buf, sizeof(response_buf), "SUCCESS");
                } else if (res < 0) {
                    snprintf(response_buf, sizeof(response_buf), "ERROR");
                } else {
                    snprintf(response_buf, sizeof(response_buf), "EXIST");
                }
                break;
            case static_cast<int>(Command::RGET):
                KV_LOG("GET\n");
                value = kvs_rbtree_get(&global_rbtree, tokens[1]);
                if(value){
                    KV_LOG("RGET success : %s\n", value);
                    snprintf(response_buf, sizeof(response_buf), "%s", value);
                }else{
                    snprintf(response_buf, sizeof(response_buf), "NO EXIST");
                }
                break;
            case static_cast<int>(Command::RDEL):
                KV_LOG("RDEL\n");
                res = kvs_rbtree_delete(&global_rbtree, tokens[1]);
                if (res < 0) {
                    snprintf(response_buf, sizeof(response_buf), "ERROR");
                } else if (res == 0) {
                    snprintf(response_buf, sizeof(response_buf), "SUCCESS");
                } else {
                    snprintf(response_buf, sizeof(response_buf), "NO EXIST");
                }
                break;
            case static_cast<int>(Command::RMOD):
                KV_LOG("RMOD\n");
                res = kvs_rbtree_modify(&global_rbtree, tokens[1], tokens[2]);
                if (res < 0) {
                    snprintf(response_buf, sizeof(response_buf), "ERROR");
                } else if (res == 0) {
                    snprintf(response_buf, sizeof(response_buf), "SUCCESS");
                } else {
                    snprintf(response_buf, sizeof(response_buf), "NO EXIST");
                }
                break;
            case static_cast<int>(Command::REXIST):
                KV_LOG("REXIST\n");
                res = kvs_rbtree_exist(&global_rbtree, tokens[1]);
                if (res == 0) {
                    snprintf(response_buf, sizeof(response_buf), "EXIST");
                } else {
                    snprintf(response_buf, sizeof(response_buf), "NO EXIST");
                }
                break;
            case static_cast<int>(Command::RSAVE):
                KV_LOG("RSAVE\n");
                res = kvs_rbtree_save(tokens[1]);
                if (res == 0) {
                    snprintf(response_buf, sizeof(response_buf), "SUCCESS");
                } else {
                    snprintf(response_buf, sizeof(response_buf), "ERROR");
                }
                break;
            case static_cast<int>(Command::RLOAD):
                KV_LOG("RLOAD\n");
                res = kvs_rbtree_load(tokens[1]);
                if (res == 0) {
                    snprintf(response_buf, sizeof(response_buf), "SUCCESS");
                } else {
                    snprintf(response_buf, sizeof(response_buf), "ERROR");
                }
                break;
            case static_cast<int>(Command::RRANGE): {
                KV_LOG("RRANGE\n");
                kvs_item_t* res_array = NULL;
                int res_count = 0;
                res = kvs_rbtree_range(&global_rbtree, tokens[1], tokens[2], &res_array, &res_count);
                if (res == 0) {
                    KV_LOG("res == 0, res_count: %d\n", res_count);
                    if (res_array == NULL) {
                        KV_LOG("res_array == NULL\n");
                    }
                    for (int i = 0; i < res_count; i++) {
                        if (res_array[i].key == NULL) {
                            KV_LOG("res_array[%d].key == NULL\n", i);
                        }
                        response += res_array[i].key;
                        response += " ";
                        if (res_array[i].value == NULL) {
                            KV_LOG("res_array[%d].value == NULL\n", i);
                        }
                        response += res_array[i].value;
                        if (i != res_count - 1) {
                            response += "\n";
                        }
                        kvs_free(res_array[i].key);
                        kvs_free(res_array[i].value);
                        KV_LOG("response: %s\n", response.c_str());
                    }
                    kvs_free(res_array);
                    KV_LOG("RANGE success, response : %s\n", response.c_str());
                    return response;
                } else if (res > 0) {
                    snprintf(response_buf, sizeof(response_buf), "EMPTY");
                }
                else {
                    snprintf(response_buf, sizeof(response_buf), "ERROR");
                }
                break;
            }
            #endif
            #if ENABLE_HASH_KV_ENGINE
            /*hash*/
            case static_cast<int>(Command::HSET):
                KV_LOG("HSET\n");
                if(count < 3){
                    KV_LOG("invalid set command\n");
                    // snprintf(response_buf, sizeof(response_buf), "FAILED");
                    response = "FAILED\n";
                    return response;
                }
                res = kvs_hash_set(&global_hash, tokens[1], tokens[2]);
                if(res == 0){
                    snprintf(response_buf, sizeof(response_buf), "SUCCESS");
                } else if (res < 0) {
                    snprintf(response_buf, sizeof(response_buf), "ERROR");
                } else {
                    snprintf(response_buf, sizeof(response_buf), "EXIST");
                }
                break;
            case static_cast<int>(Command::HGET):
                KV_LOG("HGET\n");
                value = kvs_hash_get(&global_hash, tokens[1]);
                if(value){
                    KV_LOG("HGET success : %s\n", value);
                    snprintf(response_buf, sizeof(response_buf), "%s", value);
                }else{
                    snprintf(response_buf, sizeof(response_buf), "NO EXIST");
                }
                break;
            case static_cast<int>(Command::HDEL):
                KV_LOG("HDEL\n");
                res = kvs_hash_delete(&global_hash, tokens[1]);
                if (res < 0) {
                    snprintf(response_buf, sizeof(response_buf), "ERROR");
                } else if (res == 0) {
                    snprintf(response_buf, sizeof(response_buf), "SUCCESS");
                } else {
                    snprintf(response_buf, sizeof(response_buf), "NO EXIST");
                }
                break;
            case static_cast<int>(Command::HMOD):
                KV_LOG("HMOD\n");
                res = kvs_hash_modify(&global_hash, tokens[1], tokens[2]);
                if (res < 0) {
                    snprintf(response_buf, sizeof(response_buf), "ERROR");
                } else if (res == 0) {
                    snprintf(response_buf, sizeof(response_buf), "SUCCESS");
                } else {
                    snprintf(response_buf, sizeof(response_buf), "NO EXIST");
                }
                break;
            case static_cast<int>(Command::HEXIST):
                KV_LOG("HEXIST\n");
                res = kvs_hash_exist(&global_hash, tokens[1]);
                if (res == 0) {
                    snprintf(response_buf, sizeof(response_buf), "EXIST");
                } else {
                    snprintf(response_buf, sizeof(response_buf), "NO EXIST");
                }
                break;
            case static_cast<int>(Command::HSAVE):
                KV_LOG("HSAVE\n");
                res = kvs_hash_save(tokens[1]);
                if (res == 0) {
                    snprintf(response_buf, sizeof(response_buf), "SUCCESS");
                } else {
                    snprintf(response_buf, sizeof(response_buf), "ERROR");
                }
                break;
            case static_cast<int>(Command::HLOAD):
                KV_LOG("HLOAD\n");
                res = kvs_hash_load(tokens[1]);
                if (res == 0) {
                    snprintf(response_buf, sizeof(response_buf), "SUCCESS");
                } else {
                    snprintf(response_buf, sizeof(response_buf), "ERROR");
                }
                break;
            case static_cast<int>(Command::HRANGE): {
                KV_LOG("HRANGE\n");
                kvs_item_t* res_array = NULL;
                int res_count = 0;
                res = kvs_hash_range(&global_hash, tokens[1], tokens[2], &res_array, &res_count);
                if (res == 0) {
                    KV_LOG("res == 0, res_count: %d\n", res_count);
                    if (res_array == NULL) {
                        KV_LOG("res_array == NULL\n");
                    }
                    for (int i = 0; i < res_count; i++) {
                        if (res_array[i].key == NULL) {
                            KV_LOG("res_array[%d].key == NULL\n", i);
                        }
                        response += res_array[i].key;
                        response += " ";
                        if (res_array[i].value == NULL) {
                            KV_LOG("res_array[%d].value == NULL\n", i);
                        }
                        response += res_array[i].value;
                        if (i != res_count - 1) {
                            response += "\n";
                        }
                        kvs_free(res_array[i].key);
                        kvs_free(res_array[i].value);
                        KV_LOG("response: %s\n", response.c_str());
                    }
                    kvs_free(res_array);
                    KV_LOG("HRANGE success, response : %s\n", response.c_str());
                    return response;
                }
                else if (res > 0) {
                    snprintf(response_buf, sizeof(response_buf), "EMPTY");
                }
                else {
                    snprintf(response_buf, sizeof(response_buf), "ERROR");
                }
                break;
            }
            #endif
            #if ENABLE_SKIPTABLE_KV_ENGINE
            /*skiptable*/
            case static_cast<int>(Command::SSET):
                KV_LOG("SSET\n");
                if(count < 3){
                    KV_LOG("invalid set command\n");
                    // snprintf(response_buf, sizeof(response_buf), "FAILED");
                    response = "FAILED\n";
                    return response;
                }
                res = kvs_skiptable_set(&global_skiptable, tokens[1], tokens[2]);
                if(res == 0){
                    snprintf(response_buf, sizeof(response_buf), "SUCCESS");
                } else if (res < 0) {
                    snprintf(response_buf, sizeof(response_buf), "ERROR");
                } else {
                    snprintf(response_buf, sizeof(response_buf), "EXIST");
                }
                break;
            case static_cast<int>(Command::SGET):
                KV_LOG("SGET\n");
                value = kvs_skiptable_get(&global_skiptable, tokens[1]);
                if(value){
                    KV_LOG("SGET success : %s\n", value);
                    snprintf(response_buf, sizeof(response_buf), "%s", value);
                }else{
                    snprintf(response_buf, sizeof(response_buf), "NO EXIST");
                }
                break;
            case static_cast<int>(Command::SDEL):
                KV_LOG("SDEL\n");
                res = kvs_skiptable_delete(&global_skiptable, tokens[1]);
                if (res < 0) {
                    snprintf(response_buf, sizeof(response_buf), "ERROR");
                } else if (res == 0) {
                    snprintf(response_buf, sizeof(response_buf), "SUCCESS");
                } else {
                    snprintf(response_buf, sizeof(response_buf), "NO EXIST");
                }
                break;
            case static_cast<int>(Command::SMOD):
                KV_LOG("SMOD\n");
                res = kvs_skiptable_modify(&global_skiptable, tokens[1], tokens[2]);
                if (res < 0) {
                    snprintf(response_buf, sizeof(response_buf), "ERROR");
                } else if (res == 0) {
                    snprintf(response_buf, sizeof(response_buf), "SUCCESS");
                } else {
                    snprintf(response_buf, sizeof(response_buf), "NO EXIST");
                }
                break;
            case static_cast<int>(Command::SEXIST):
                KV_LOG("SEXIST\n");
                res = kvs_skiptable_exist(&global_skiptable, tokens[1]);
                if (res == 0) {
                    snprintf(response_buf, sizeof(response_buf), "EXIST");
                } else {
                    snprintf(response_buf, sizeof(response_buf), "NO EXIST");
                }
                break;
            case static_cast<int>(Command::SSAVE):
                KV_LOG("SSAVE\n");
                res = kvs_skiptable_save(tokens[1]);
                if (res == 0) {
                    snprintf(response_buf, sizeof(response_buf), "SUCCESS");
                } else {
                    snprintf(response_buf, sizeof(response_buf), "ERROR");
                }
                break;
            case static_cast<int>(Command::SLOAD):
                KV_LOG("SLOAD\n");
                res = kvs_skiptable_load(tokens[1]);
                if (res == 0) {
                    snprintf(response_buf, sizeof(response_buf), "SUCCESS");
                } else {
                    snprintf(response_buf, sizeof(response_buf), "ERROR");
                }
                break;
            case static_cast<int>(Command::SRANGE): {
                KV_LOG("SRANGE\n");
                kvs_item_t* res_array = NULL;
                int res_count = 0;
                res = kvs_skiptable_range(&global_skiptable, tokens[1], tokens[2], &res_array, &res_count);
                if (res == 0) {
                    KV_LOG("res == 0, res_count: %d\n", res_count);
                    if (res_array == NULL) {
                        KV_LOG("res_array == NULL\n");
                    }
                    for (int i = 0; i < res_count; i++) {
                        if (res_array[i].key == NULL) {
                            KV_LOG("res_array[%d].key == NULL\n", i);
                        }
                        response += res_array[i].key;
                        response += " ";
                        if (res_array[i].value == NULL) {
                            KV_LOG("res_array[%d].value == NULL\n", i);
                        }
                        response += res_array[i].value;
                        if (i != res_count - 1) {
                            response += "\n";
                        }
                        kvs_free(res_array[i].key);
                        kvs_free(res_array[i].value);
                        KV_LOG("response: %s\n", response.c_str());
                    }
                    kvs_free(res_array);
                    KV_LOG("SRANGE success, response : %s\n", response.c_str());
                    return response;
                }
                else if (res > 0) {
                    snprintf(response_buf, sizeof(response_buf), "EMPTY");
                }
                else {
                    snprintf(response_buf, sizeof(response_buf), "ERROR");
                }
                break;
            }
            #endif
            default:
                KV_LOG("unknow command, echo...\n");
                // snprintf(response_buf, sizeof(response_buf), r_buf.data());
                response = r_buf.data();
                break;
                //return -1;
        }
        response = response_buf;
        // if(strlen(w_buf) > 0){

        //     Reactor::get_instance().modify_handler(fd_, EPOLLIN | EPOLLOUT | EPOLLET, shared_from_this());
        // }
        return response;
    }

    void 
    process_received_data()
    {
        auto it = std::find(r_buf.begin(), r_buf.end(), '\n');
        if (it == r_buf.end()) {
            KV_LOG("no complete command\n");
            return;
        }
        while (it != r_buf.end()) {
            std::string command(r_buf.begin(), it);
            r_buf.erase(r_buf.begin(), it + 1); /*连带换行符删除*/
            std::string response = process_command(command);
            KV_LOG("response: %s\n", response.c_str());
            send_queue_.push(response + '\n');
            //if (1 == send_queue_.size()) {
                //Reactor::get_instance().modify_handler(fd_, EPOLLIN | EPOLLOUT | EPOLLET, shared_from_this());
            //}
            it = std::find(r_buf.begin(), r_buf.end(), '\n');
        }
        Reactor::get_instance().modify_handler(fd_, EPOLLIN | EPOLLOUT | EPOLLET, shared_from_this());
    }

    void 
    handle_read()
    {
        char temp_buf[4096] = {0};
        while(true){
            memset(temp_buf, 0, sizeof(temp_buf));
            int n = recv(fd_, temp_buf, sizeof(temp_buf), 0);
            if (n > 0) {
                KV_LOG("read: %s, bytes: %d\n", temp_buf, n);
                r_buf.insert(r_buf.end(), temp_buf, temp_buf + n);
                process_received_data();                
                // int count = split_token();
                // KV_LOG("tokens count: %d\n", count);
                // for(int i = 0; i < count; ++i){
                    // KV_LOG("token: %s\n", tokens[i]);
                // }
                // parse_protocol(count);
                // send(fd_, r_buf, n, 0);
            } else if (n == 0) {
                KV_LOG("client close\n");
                Reactor::get_instance().unregister_handler(fd_);
                break;
            } else {
                if(errno == EAGAIN || errno == EWOULDBLOCK){
                    break;
                }
                perror("read");
                Reactor::get_instance().unregister_handler(fd_);
                break;
            }   
        }
    }

    void 
    handle_write() 
    {
        std::string response;
        while(1) {
            if (send_queue_.empty()) {
                KV_LOG("send queue empty\n");
                Reactor::get_instance().modify_handler(fd_, 
                                                        EPOLLIN | EPOLLET, 
                                                        shared_from_this());
                break;
            }
            // static size_t sent_bytes = 0;
            // size_t total_len = strlen(w_buf);
            // if(total_len == 0 || sent_bytes >= total_len){
            //     sent_bytes = 0;
            //     Reactor::get_instance().modify_handler(fd_, EPOLLIN | EPOLLET, shared_from_this());
            //     return;
            // }
            response += send_queue_.front();
            send_queue_.pop();
            
            // while (true) {
            //     int n = send(fd_, w_buf + sent_bytes, total_len - sent_bytes, 0);
            //     if(n > 0){
            //         sent_bytes += n;
            //         if ( sent_bytes >= total_len ) {
            //             /*全部发送完成*/
            //             KV_LOG("all send\n");
            //             memset(w_buf, 0, sizeof(w_buf));
            //             sent_bytes = 0;
            //             Reactor::get_instance().modify_handler(fd_, EPOLLIN | EPOLLET, shared_from_this());
            //             break;
            //         }
            //         KV_LOG("send: %s bytes: %d\n", w_buf + sent_bytes, n);
            //     } else if (n == 0) {
            //         KV_LOG("n == 0\n");
            //         Reactor::get_instance().unregister_handler(fd_);
            //         close(fd_);
            //         break;
            //     } else {
            //         if (errno == EAGAIN || errno == EWOULDBLOCK) {
            //             /*缓冲区满，等待下次写事件*/
            //             KV_LOG("errno == EAGAIN || errno == EWOULDBLOCK\n");
            //             Reactor::get_instance().modify_handler(fd_, EPOLLIN | EPOLLOUT | EPOLLET, shared_from_this());
            //             // Reactor::get_instance().modify_handler(fd_, EPOLLIN | EPOLLET, shared_from_this());
            //             break;
            //         }
            //         perror("send");
            //         Reactor::get_instance().unregister_handler(fd_);
            //         close(fd_);
            //         break;
            //     }
            // }
        }
        int n = send(fd_, response.data(), response.size(), 0);
        KV_LOG("send: %s, bytes: %ld, ret = %d\n", response.data(), response.size(), n);
        if (n > 0) {
            KV_LOG("send: %s bytes: %ld success\n", response.data(), response.size());
        } else if (n == 0) {
            KV_LOG("n == 0, client closed\n");
            Reactor::get_instance().unregister_handler(fd_);
            close(fd_);
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                /*缓冲区满，等待下次写事件*/
                KV_LOG("errno == EAGAIN || errno == EWOULDBLOCK\n");
                Reactor::get_instance().modify_handler(fd_, EPOLLIN | EPOLLOUT | EPOLLET, shared_from_this());
            } else {
                KV_LOG("send error\n");
                Reactor::get_instance().unregister_handler(fd_);
                close(fd_);
            }
        }
    }
    
    void 
    handle_error() 
    {
        Reactor::get_instance().unregister_handler(fd_);
    }

    enum class State{
        READ_HEADER, 
        READ_BODY
    };

    enum class Command{
        START = 0, 
        SET = START, 
        GET, 
        DEL, 
        MOD,
        EXIST,
        SAVE,
        LOAD,
        RANGE,
        RSET,
        RGET,
        RDEL,
        RMOD,
        REXIST,
        RSAVE,
        RLOAD,
        RRANGE,
        HSET,
        HGET,
        HDEL,
        HMOD,
        HEXIST,
        HSAVE,
        HLOAD,
        HRANGE,
        SSET,
        SGET,
        SDEL,
        SMOD,
        SEXIST,
        SSAVE,
        SLOAD,
        SRANGE,
        COUNT,
    };

    State state_ = State::READ_HEADER;

    char *tokens[MAX_TOKENS];
    std::vector<char> r_buf;
    std::vector<char> w_buf;
    std::queue<std::string> send_queue_;
    size_t current_send_offset_ = 0;
    //char r_buf[1024] = {0};
    //char w_buf[1024] = {0};
    const char* commands[static_cast<int>(Command::COUNT)] = {
        "SET", "GET", "DEL", "MOD","EXIST","SAVE", "LOAD", "RANGE",
        "RSET", "RGET", "RDEL", "RMOD","REXIST","RSAVE","RLOAD", "RRANGE",
        "HSET", "HGET", "HDEL", "HMOD","HEXIST","HSAVE","HLOAD", "HRANGE",
        "SSET", "SGET", "SDEL", "SMOD","SEXIST","SSAVE","SLOAD", "SRANGE",
    };

};

class KVStoreEpollAcceptor : public IAcceptor, public std::enable_shared_from_this<KVStoreEpollAcceptor> 
{
public:
    KVStoreEpollAcceptor(int port) : IAcceptor(port){}
    KVStoreEpollAcceptor(const std::string &ip, int port) : IAcceptor(ip, port){}

    void init(){
        Reactor::get_instance().register_handler(server_fd_, EPOLLIN, shared_from_this());
    }
private:
    void on_new_connection(int new_fd) override{
        auto client = std::make_shared<KVStoreEpollConnection>(new_fd);
        // Reactor::get_instance().register_handler(new_fd, EPOLLIN | EPOLLET | EPOLLONESHOT, client);
        printf("new connection: %d\n", new_fd);
        Reactor::get_instance().register_handler(new_fd, EPOLLIN | EPOLLET, client);
    }
};

#endif

#endif
