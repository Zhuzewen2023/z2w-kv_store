#ifndef __KV_STORE_EPOLL_HPP__
#define __KV_STORE_EPOLL_HPP__

#include "kv_store.hpp"
#include "kv_log.h"
#include "kv_save.h"
#include <stddef.h>
#include <pthread.h>

#ifdef __cplusplus

#include <queue>
#include <chrono>
#include <vector>
#include <mutex>

#include "kv_store_sync_client.hpp"

using namespace std;

// 为每个全局引擎添加互斥锁（关键修复）
struct KVEngineLocks {
    static pthread_mutex_t array_lock;    // 数组引擎锁
    static pthread_mutex_t rbtree_lock;   // 红黑树引擎锁
    static pthread_mutex_t hash_lock;     // 哈希表引擎锁
    static pthread_mutex_t skiptable_lock;// 跳表引擎锁
};

class KVStoreEpollConnection : public IConnection, public std::enable_shared_from_this<KVStoreEpollConnection> 
{
public:
    KVStoreEpollConnection(int fd) : IConnection(fd), 
                                    r_buf(),
                                    w_buf(),
                                    send_queue_(),
                                    current_send_offset_(0) 
    {
        r_buf.reserve(512000); /*预分配接收缓冲区*/
        w_buf.reserve(512000); /*预分配发送缓冲区*/
    }

private:
    constexpr static int MAX_TOKENS = 256;

    std::vector<SyncData> 
    get_local_array_data()
    {
        std::vector<SyncData> result;

        #if ENABLE_ARRAY_KV_ENGINE
        kvs_item_t* items = nullptr;
        int count = -1;
        int res = kvs_array_get_all(&global_array, &items, &count);
        if (res < 0) {
            KV_LOG("kvs_array_get_all failed, res : %d\n", res);
            return result;
        }
        if (count == 0) {
            KV_LOG("kvs_array_get_all count is 0\n");
            return result;
        }
        KV_LOG("kvs_array_get_all count: %d\n", count);
        for (int i = 0; i < count; i++) {
            if (items[i].key && items[i].value) {
                std::string key = items[i].key;
                std::string value = items[i].value;
#if USE_TIMESTAMP
                result.emplace_back(key, value, items[i].timestamp);
#else
                result.emplace_back(key, value);
#endif
                if (items[i].key) {
                    KV_LOG("free key: %s\n", items[i].key);
                    kvs_free(items[i].key);
                    items[i].key = NULL;
                }
                if (items[i].value) {
                    KV_LOG("free value: %s\n", items[i].value);
                    kvs_free(items[i].value);
                    items[i].value = NULL;
                }
            }
        }
        if (items) {
            KV_LOG("free items\n");
            kvs_free(items);
            items = NULL;
        }
		#endif
        KV_LOG("get_local_array_data() end\n");
        return result;
    }

	std::vector<SyncData>
	get_local_rbtree_data()
	{
		std::vector<SyncData> result;
		#if ENABLE_RBTREE_KV_ENGINE
		kvs_item_t* items = nullptr;
        int count = -1;
        int res = kvs_rbtree_get_all(&global_rbtree, &items, &count);
        if (res < 0) {
            KV_LOG("kvs_rbtree_get_all failed, res : %d\n", res);
            return result;
        }
        if (count == 0) {
            KV_LOG("kvs_rbtree_get_all count is 0\n");
            return result;
        }
        KV_LOG("kvs_rbtree_get_all count: %d\n", count);
        for (int i = 0; i < count; i++) {
            if (items[i].key && items[i].value) {
                std::string key = items[i].key;
                std::string value = items[i].value;
#if USE_TIMESTAMP
                result.emplace_back(key, value, items[i].timestamp);
#else
                result.emplace_back(key, value);
#endif
                if (items[i].key) {
                    KV_LOG("free key: %s\n", items[i].key);
                    kvs_free(items[i].key);
                    items[i].key = NULL;
                }
                if (items[i].value) {
                    KV_LOG("free value: %s\n", items[i].value);
                    kvs_free(items[i].value);
                    items[i].value = NULL;
                }
            }
        }
        if (items) {
            KV_LOG("free items\n");
            kvs_free(items);
            items = NULL;
        }
		#endif
		KV_LOG("get_local_rbtree_data() end\n");
		return result;
	}

	std::vector<SyncData>
	get_local_hash_data()
	{
		std::vector<SyncData> result;
		#if ENABLE_HASH_KV_ENGINE
        kvs_item_t* items = nullptr;
        int count = -1;
        int res = kvs_hash_get_all(&global_hash, &items, &count);
        if (res < 0) {
            KV_LOG("kvs_hash_get_all failed, res : %d\n", res);
            return result;
        }
        if (count == 0) {
            KV_LOG("kvs_hash_get_all count is 0\n");
            return result;
        }
        KV_LOG("kvs_hash_get_all count: %d\n", count);
        for (int i = 0; i < count; i++) {
            if (items[i].key && items[i].value) {
                std::string key = items[i].key;
                std::string value = items[i].value;
#if USE_TIMESTAMP
                result.emplace_back(key, value, items[i].timestamp);
#else
                result.emplace_back(key, value);
#endif
                if (items[i].key) {
                    KV_LOG("free key: %s\n", items[i].key);
                    kvs_free(items[i].key);
                    items[i].key = NULL;
                }
                if (items[i].value) {
                    KV_LOG("free value: %s\n", items[i].value);
                    kvs_free(items[i].value);
                    items[i].value = NULL;
                }
            }
        }
        if (items) {
            KV_LOG("free items\n");
            kvs_free(items);
            items = NULL;
        }
		#endif
		KV_LOG("get_local_hash_data() end\n");
		return result;
	}

	std::vector<SyncData>
	get_local_skiptable_data()
	{
		std::vector<SyncData> result;
		#if ENABLE_SKIPTABLE_KV_ENGINE
        kvs_item_t* items = nullptr;
        int count = -1;
        int res = kvs_skiptable_get_all(&global_skiptable, &items, &count);
        if (res < 0) {
            KV_LOG("kvs_skiptable_get_all failed, res : %d\n", res);
            return result;
        }
        if (count == 0) {
            KV_LOG("kvs_skiptable_get_all count is 0\n");
            return result;
        }
        KV_LOG("kvs_skiptable_get_all count: %d\n", count);
        for (int i = 0; i < count; i++) {
            if (items[i].key && items[i].value) {
                std::string key = items[i].key;
                std::string value = items[i].value;
#if USE_TIMESTAMP
                result.emplace_back(key, value, items[i].timestamp);
#else
                result.emplace_back(key, value);
#endif
                if (items[i].key) {
                    KV_LOG("free key: %s\n", items[i].key);
                    kvs_free(items[i].key);
                    items[i].key = NULL;
                }
                if (items[i].value) {
                    KV_LOG("free value: %s\n", items[i].value);
                    kvs_free(items[i].value);
                    items[i].value = NULL;
                }
            }
        }
        if (items) {
            KV_LOG("free items\n");
            kvs_free(items);
            items = NULL;
        }
		#endif
		KV_LOG("get_local_skiptable_data() end\n");
		return result;
	}
    
    std::string 
    process_command(const std::string& command)
    {
        KV_LOG("process_command : %s\n", command.c_str());
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
				//pthread_mutex_lock(&KVEngineLocks::array_lock);
                res = kvs_array_set(&global_array, tokens[1], tokens[2]);
                //pthread_mutex_unlock(&KVEngineLocks::array_lock);
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
                //pthread_mutex_lock(&KVEngineLocks::array_lock);
                value = kvs_array_get(&global_array, tokens[1]);
                //pthread_mutex_unlock(&KVEngineLocks::array_lock);
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
                //pthread_mutex_lock(&KVEngineLocks::array_lock);
                res = kvs_array_delete(&global_array, tokens[1]);
                //pthread_mutex_unlock(&KVEngineLocks::array_lock);
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
                //pthread_mutex_lock(&KVEngineLocks::array_lock);
                res = kvs_array_modify(&global_array, tokens[1], tokens[2]);
                //pthread_mutex_unlock(&KVEngineLocks::array_lock);
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
                //pthread_mutex_lock(&KVEngineLocks::array_lock);
                res = kvs_array_exist(&global_array, tokens[1]);
                //pthread_mutex_unlock(&KVEngineLocks::array_lock);
                if (res == 0) {
                    snprintf(response_buf, sizeof(response_buf), "EXIST");
                } else {
                    snprintf(response_buf, sizeof(response_buf), "NO EXIST");
                }
                break;
            case static_cast<int>(Command::SAVE):
                KV_LOG("SAVE\n");
                //pthread_mutex_lock(&KVEngineLocks::array_lock);
                res = kvs_array_save(tokens[1]);
                //pthread_mutex_unlock(&KVEngineLocks::array_lock);
                if (res == 0) {
                    snprintf(response_buf, sizeof(response_buf), "SUCCESS");
                } else {
                    snprintf(response_buf, sizeof(response_buf), "ERROR");
                }
                break;
            case static_cast<int>(Command::LOAD):
                KV_LOG("LOAD\n");
                //pthread_mutex_lock(&KVEngineLocks::array_lock);
                res = kvs_array_load(tokens[1]);
                //pthread_mutex_unlock(&KVEngineLocks::array_lock);
                if (res == 0) {
                    snprintf(response_buf, sizeof(response_buf), "SUCCESS");
                } else {
                    snprintf(response_buf, sizeof(response_buf), "ERROR");
                }
                break;
            case static_cast<int>(Command::RANGE): {
                KV_LOG("RANGE\n");
                kvs_item_t* res_array = NULL;
                int res_count = 0;
               // pthread_mutex_lock(&KVEngineLocks::array_lock);
                res = kvs_array_range(&global_array, tokens[1], tokens[2], &res_array, &res_count);
                //pthread_mutex_unlock(&KVEngineLocks::array_lock);
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
            
                break;
            }
            case static_cast<int>(Command::SYNC): {
                KV_LOG("SYNC\n");
                if (count < 3) {
                    KV_LOG("invalid sync command\n");
                    // snprintf(response_buf, sizeof(response_buf), "FAILED");
                    KV_LOG("Error: invalid sync command\n");
                    response = "FAILED";
                    return response;
                }
                std::string remote_ip = tokens[1];
                int remote_port = std::stoi(tokens[2]);
                KV_LOG("remote_ip: %s, remote_port: %d\n", remote_ip.c_str(), remote_port);
                SyncClient sync_client(remote_ip, remote_port);
                if (0 != sync_client.connect()) {
                    KV_LOG("Error: failed to connect remote server\n");
                    response = "FAILED";
                    return response;
                }
                KV_LOG("connect remote server success\n");

                if (0 != sync_client.send_command("GETARRAY\n")) {
                    KV_LOG("Error: failed to send command GETARRAY to remote server\n");
                    response = "FAILED";
                    return response;
                }

                std::string remote_data_str = sync_client.receive_response();
                if (remote_data_str.empty()) {
                    KV_LOG("Error: failed to receive response from remote server\n");
                    response = "FAILED";
                    return response;
                }

                KV_LOG("remote_data_str: %s\n", remote_data_str.c_str());
                std::vector<SyncData> remote_data = sync_client.parse_remote_data_str(remote_data_str);
                if (remote_data.empty()) {
                    KV_LOG("Error: failed to parse remote data\n");
                    response = "FAILED";
                    return response;
                }
                //pthread_mutex_lock(&KVEngineLocks::array_lock);
                if (0 != sync_client.sync_data_to_local_array_engine(remote_data)) {
                    KV_LOG("Error: failed to sync data to local array engine\n");
                    response = "FAILED";
                }
                else {
                    KV_LOG("sync data to local array engine success\n");
                    response = "SUCCESS";
                }
                //pthread_mutex_unlock(&KVEngineLocks::array_lock);
                return response;
                break;
            }
            case static_cast<int>(Command::GETARRAY) : {
                KV_LOG("GETARRAY\n");
                //pthread_mutex_lock(&KVEngineLocks::array_lock);
                std::vector<SyncData> local_data = get_local_array_data();
                //pthread_mutex_unlock(&KVEngineLocks::array_lock);
                if (local_data.empty()) {
                    KV_LOG("local_data is empty\n");
                    response = "EMPTY";
                    return response;
                }
                KV_LOG("local_data size: %lu\n", local_data.size());
                for (auto& data : local_data) {
                    response += data.key;
                    response += " ";
                    response += data.value;
                    response += " ";
#if USE_TIMESTAMP
                    response += std::to_string(data.timestamp);
#endif
                    response += "\n";
                    //KV_LOG("response: %s", response.c_str());
                }
                return response;
                break;
            }
            #endif
            #if ENABLE_RBTREE_KV_ENGINE
            /*rbtree*/
                case static_cast<int>(Command::RSET):
                KV_LOG("RSET\n");
                if(count < 3){
                    KV_LOG("invalid set command\n");
                    // snprintf(response_buf, sizeof(response_buf), "FAILED");
                    response = "FAILED";
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
            case static_cast<int>(Command::RSYNC) : {
                KV_LOG("RSYNC\n");
                if (count < 3) {
                    KV_LOG("invalid sync command\n");
                    response = "FAILED";
                    return response;
                }
                std::string remote_ip = tokens[1];
                int remote_port = std::stoi(tokens[2]);
                KV_LOG("remote_ip: %s, remote_port: %d\n", remote_ip.c_str(), remote_port);
                SyncClient sync_client(remote_ip, remote_port);
                if (0 != sync_client.connect()) {
                    KV_LOG("Error: failed to connect remote server\n");
                    response = "FAILED";
                    return response;
                }
                KV_LOG("connect remote server success\n");

                if (0 != sync_client.send_command("GETRBTREE\n")) {
                    KV_LOG("Error: failed to send command GETRBTREE to remote server\n");
                    response = "FAILED";
                    return response;
                }

                std::string remote_data_str = sync_client.receive_response();
                if (remote_data_str.empty()) {
                    KV_LOG("Error: failed to receive response from remote server\n");
                    response = "FAILED";
                    return response;
                }

                KV_LOG("remote_data_str: %s\n", remote_data_str.c_str());
                std::vector<SyncData> remote_data = sync_client.parse_remote_data_str(remote_data_str);
                if (remote_data.empty()) {
                    KV_LOG("Error: failed to parse remote data\n");
                    response = "FAILED";
                    return response;
                }

                if (0 != sync_client.sync_data_to_local_rbtree_engine(remote_data)) {
                    KV_LOG("Error: failed to sync data to local rbtree engine\n");
                    response = "FAILED";
                }
                else {
                    KV_LOG("sync data to local rbtree engine success\n");
                    response = "SUCCESS";
                }
                return response;
                break;
            }
            case static_cast<int>(Command::GETRBTREE) : {
                KV_LOG("GETRBTREE\n");
                std::vector<SyncData> local_data = get_local_rbtree_data();
                if (local_data.empty()) {
                    KV_LOG("local_data is empty\n");
                    response = "EMPTY";
                    return response;
                }
                KV_LOG("local_data size: %lu\n", local_data.size());
                for (auto& data : local_data) {
                    response += data.key;
                    response += " ";
                    response += data.value;
                    response += " ";
#if USE_TIMESTAMP
                    response += std::to_string(data.timestamp);
#endif
                    response += "\n";
                    KV_LOG("response: %s", response.c_str());
                }
                return response;
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
                    response = "FAILED";
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
            case static_cast<int>(Command::HSYNC) : {
                KV_LOG("HSYNC\n");
                if (count < 3) {
                    KV_LOG("invalid sync command\n");
                    response = "FAILED";
                    return response;
                }
                std::string remote_ip = tokens[1];
                int remote_port = std::stoi(tokens[2]);
                KV_LOG("remote_ip: %s, remote_port: %d\n", remote_ip.c_str(), remote_port);
                SyncClient sync_client(remote_ip, remote_port);
                if (0 != sync_client.connect()) {
                    KV_LOG("Error: failed to connect remote server\n");
                    response = "FAILED";
                    return response;
                }
                KV_LOG("connect remote server success\n");

                if (0 != sync_client.send_command("GETHASH\n")) {
                    KV_LOG("Error: failed to send command GETHASH to remote server\n");
                    response = "FAILED";
                    return response;
                }

                std::string remote_data_str = sync_client.receive_response();
                if (remote_data_str.empty()) {
                    KV_LOG("Error: failed to receive response from remote server\n");
                    response = "FAILED";
                    return response;
                }

                KV_LOG("remote_data_str: %s\n", remote_data_str.c_str());
                std::vector<SyncData> remote_data = sync_client.parse_remote_data_str(remote_data_str);
                if (remote_data.empty()) {
                    KV_LOG("Error: failed to parse remote data\n");
                    response = "FAILED";
                    return response;
                }

                if (0 != sync_client.sync_data_to_local_hash_engine(remote_data)) {
                    KV_LOG("Error: failed to sync data to local hash engine\n");
                    response = "FAILED";
                }
                else {
                    KV_LOG("sync data to local hash engine success\n");
                    response = "SUCCESS";
                }
                return response;
                break;
            }
            case static_cast<int>(Command::GETHASH) : {
                KV_LOG("GETHASH\n");
                std::vector<SyncData> local_data = get_local_hash_data();
                if (local_data.empty()) {
                    KV_LOG("local_data is empty\n");
                    response = "EMPTY";
                    return response;
                }
                KV_LOG("local_data size: %lu\n", local_data.size());
                for (auto& data : local_data) {
                    response += data.key;
                    response += " ";
                    response += data.value;
                    response += " ";
#if USE_TIMESTAMP
                    response += std::to_string(data.timestamp);
#endif
                    response += "\n";
                    KV_LOG("response: %s", response.c_str());
                }
                return response;
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
                    response = "FAILED";
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
            case static_cast<int>(Command::SSYNC) : {
                KV_LOG("SSYNC\n");
                if (count < 3) {
                    KV_LOG("invalid sync command\n");
                    // snprintf(response_buf, sizeof(response_buf), "FAILED");
                    response = "FAILED";
                    return response;
                }
                std::string remote_ip = tokens[1];
                int remote_port = std::stoi(tokens[2]);
                KV_LOG("remote_ip: %s, remote_port: %d\n", remote_ip.c_str(), remote_port);
                SyncClient sync_client(remote_ip, remote_port);
                if (0 != sync_client.connect()) {
                    KV_LOG("Error: failed to connect remote server\n");
                    response = "FAILED";
                    return response;
                }
                KV_LOG("connect remote server success\n");

                if (0 != sync_client.send_command("GETSKIPTABLE\n")) {
                    KV_LOG("Error: failed to send command GETSKIPTABLE to remote server\n");
                    response = "FAILED";
                    return response;
                }

                std::string remote_data_str = sync_client.receive_response();
                if (remote_data_str.empty()) {
                    KV_LOG("Error: failed to receive response from remote server\n");
                    response = "FAILED";
                    return response;
                }

                KV_LOG("remote_data_str: %s\n", remote_data_str.c_str());
                std::vector<SyncData> remote_data = sync_client.parse_remote_data_str(remote_data_str);
                if (remote_data.empty()) {
                    KV_LOG("Error: failed to parse remote data\n");
                    response = "FAILED";
                    return response;
                }

                if (0 != sync_client.sync_data_to_local_skiptable_engine(remote_data)) {
                    KV_LOG("Error: failed to sync data to local skiptable engine\n");
                    response = "FAILED";
                }
                else {
                    KV_LOG("sync data to local skiptable engine success\n");
                    response = "SUCCESS";
                }
                return response;
                break;
            }
            case static_cast<int>(Command::GETSKIPTABLE) : {
                KV_LOG("GETSKIPTABLE\n");
                std::vector<SyncData> local_data = get_local_skiptable_data();
                if (local_data.empty()) {
                    KV_LOG("local_data is empty\n");
                    response = "EMPTY";
                    return response;
                }
                KV_LOG("local_data size: %lu\n", local_data.size());
                for (auto& data : local_data) {
                    response += data.key;
                    response += " ";
                    response += data.value;
                    response += " ";
#if USE_TIMESTAMP
                    response += std::to_string(data.timestamp);
#endif
                    response += "\n";
                    KV_LOG("response: %s", response.c_str());
                }
                return response;
                break;
            }
            #endif
            default:
                KV_LOG("unknow command, echo...\n");
                response = r_buf.data();
                break;
        }
        response = response_buf;
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
            KV_LOG("command: %s, response: %s", command.c_str(), response.c_str());
            send_queue_.push(response + '\n');
            // send_queue_.push(response);
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
                //KV_LOG("send queue empty\n");
                Reactor::get_instance().modify_handler(fd_, 
                                                        EPOLLIN | EPOLLET, 
                                                        shared_from_this());
                break;
            }
            response += send_queue_.front();
            send_queue_.pop();
        }
        int n = send(fd_, response.data(), response.size(), 0);
        //KV_LOG("send: %s, bytes: %ld, ret = %d\n", response.data(), response.size(), n);
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
        SYNC,
        GETARRAY,
        RSET,
        RGET,
        RDEL,
        RMOD,
        REXIST,
        RSAVE,
        RLOAD,
        RRANGE,
        RSYNC,
        GETRBTREE,
        HSET,
        HGET,
        HDEL,
        HMOD,
        HEXIST,
        HSAVE,
        HLOAD,
        HRANGE,
        HSYNC,
        GETHASH,
        SSET,
        SGET,
        SDEL,
        SMOD,
        SEXIST,
        SSAVE,
        SLOAD,
        SRANGE,
        SSYNC,
        GETSKIPTABLE,
        COUNT,
    };

    State state_ = State::READ_HEADER;

    char *tokens[MAX_TOKENS];
    std::vector<char> r_buf;
    std::vector<char> w_buf;
    std::queue<std::string> send_queue_;
    size_t current_send_offset_ = 0;

    const char* commands[static_cast<int>(Command::COUNT)] = {
        "SET", "GET", "DEL", "MOD","EXIST","SAVE", "LOAD", "RANGE","SYNC","GETARRAY",
        "RSET", "RGET", "RDEL", "RMOD","REXIST","RSAVE","RLOAD", "RRANGE", "RSYNC", "GETRBTREE",
        "HSET", "HGET", "HDEL", "HMOD","HEXIST","HSAVE","HLOAD", "HRANGE", "HSYNC", "GETHASH",
        "SSET", "SGET", "SDEL", "SMOD","SEXIST","SSAVE","SLOAD", "SRANGE", "SSYNC", "GETSKIPTABLE",
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
        printf("new connection: %d\n", new_fd);
        Reactor::get_instance().register_handler(new_fd, EPOLLIN | EPOLLET, client);
    }
};

#endif

#endif
