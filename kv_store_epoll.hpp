#ifndef __KV_STORE_EPOLL_HPP__
#define __KV_STORE_EPOLL_HPP__

#include "kv_store.hpp"
#include "kv_log.h"
#include "kv_save.h"

#ifdef __cplusplus

class KVStoreEpollConnection : public IConnection, public std::enable_shared_from_this<KVStoreEpollConnection> 
{
public:
    KVStoreEpollConnection(int fd) : IConnection(fd){}

private:
    constexpr static int MAX_TOKENS = 128;
    int split_token() {
        int idx = 0;
        char *token = strtok(r_buf, " ");

        while (token != NULL) {
            tokens[idx++] = token;
            token = strtok(NULL, " ");
        }
        return idx;
    }
    
    int parse_protocol(int count) {
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

    void handle_read(){
        while(true){
            memset(r_buf, 0, sizeof(r_buf));
            int n = read(fd_, r_buf, sizeof(r_buf));
            if(n > 0){
                KV_LOG("read: %s, bytes: %d\n", r_buf, n);
                
                int count = split_token();
                KV_LOG("tokens count: %d\n", count);
                for(int i = 0; i < count; ++i){
                    KV_LOG("token: %s\n", tokens[i]);;
                }
                parse_protocol(count);
                // send(fd_, r_buf, n, 0);
            }else if(n == 0){
                KV_LOG("client close\n");
                Reactor::get_instance().unregister_handler(fd_);
                break;
            }else{
                if(errno == EAGAIN || errno == EWOULDBLOCK){
                    break;
                }
                perror("read");
                Reactor::get_instance().unregister_handler(fd_);
                break;
            }   
        }
    }
    void handle_write() {
        static size_t sent_bytes = 0;
        size_t total_len = strlen(w_buf);
        if(total_len == 0 || sent_bytes >= total_len){
            sent_bytes = 0;
            Reactor::get_instance().modify_handler(fd_, EPOLLIN | EPOLLET, shared_from_this());
            return;
        }
        
        while (true) {
            int n = send(fd_, w_buf + sent_bytes, total_len - sent_bytes, 0);
            if(n > 0){
                sent_bytes += n;
                if ( sent_bytes >= total_len ) {
                    /*全部发送完成*/
                    KV_LOG("all send\n");
                    memset(w_buf, 0, sizeof(w_buf));
                    sent_bytes = 0;
                    Reactor::get_instance().modify_handler(fd_, EPOLLIN | EPOLLET, shared_from_this());
                    break;
                }
                KV_LOG("send: %s bytes: %d\n", w_buf + sent_bytes, n);
            } else if (n == 0) {
                KV_LOG("n == 0\n");
                Reactor::get_instance().unregister_handler(fd_);
                close(fd_);
                break;
            } else {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    /*缓冲区满，等待下次写事件*/
                    KV_LOG("errno == EAGAIN || errno == EWOULDBLOCK\n");
                    Reactor::get_instance().modify_handler(fd_, EPOLLIN | EPOLLOUT | EPOLLET, shared_from_this());
                    // Reactor::get_instance().modify_handler(fd_, EPOLLIN | EPOLLET, shared_from_this());
                    break;
                }
                perror("send");
                Reactor::get_instance().unregister_handler(fd_);
                close(fd_);
                break;
            }
        }
    }
    void handle_error() {
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
        RSET,
        RGET,
        RDEL,
        RMOD,
        REXIST,
        RSAVE,
        RLOAD,
        HSET,
        HGET,
        HDEL,
        HMOD,
        HEXIST,
        HSAVE,
        HLOAD,
        SSET,
        SGET,
        SDEL,
        SMOD,
        SEXIST,
        SSAVE,
        SLOAD,
        COUNT,
    };

    State state_ = State::READ_HEADER;

    char *tokens[MAX_TOKENS];
    char r_buf[1024] = {0};
    char w_buf[1024] = {0};
    const char* commands[static_cast<int>(Command::COUNT)] = {
        "SET", "GET", "DEL", "MOD","EXIST","SAVE", "LOAD",
        "RSET", "RGET", "RDEL", "RMOD","REXIST","RSAVE","RLOAD",
        "HSET", "HGET", "HDEL", "HMOD","HEXIST","HSAVE","HLOAD",
        "SSET", "SGET", "SDEL", "SMOD","SEXIST","SSAVE","SLOAD"
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
