#ifndef __KV_STORE_EPOLL_HPP__
#define __KV_STORE_EPOLL_HPP__

#include "kv_store.hpp"
#include "kv_log.h"

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
            case static_cast<int>(Command::SET):
                KV_LOG("SET\n");
                if(count < 3){
                    KV_LOG("invalid set command\n");
                    snprintf(w_buf, sizeof(w_buf), "FAILED");
                    return -1;
                }
                res = kvs_array_set(tokens[1], tokens[2]);
                if(res == 0){
                    snprintf(w_buf, sizeof(w_buf), "SUCCESS");
                }
                break;
            case static_cast<int>(Command::GET):
                KV_LOG("GET\n");
                value = kvs_array_get(tokens[1]);
                if(value){
                    KV_LOG("GET success : %s\n", value);
                    snprintf(w_buf, sizeof(w_buf), "%s", value);
                }else{
                    snprintf(w_buf, sizeof(w_buf), "NO EXIST");
                }
                break;
            case static_cast<int>(Command::DEL):
                KV_LOG("DEL\n");
                res = kvs_array_delete(tokens[1]);
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
                res = kvs_array_modify(tokens[1], tokens[2]);
                if (res < 0) {
                    snprintf(w_buf, sizeof(w_buf), "ERROR");
                } else if (res == 0) {
                    snprintf(w_buf, sizeof(w_buf), "SUCCESS");
                } else {
                    snprintf(w_buf, sizeof(w_buf), "NO EXIST");
                }
                break;
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
        COUNT,
    };

    State state_ = State::READ_HEADER;

    char *tokens[MAX_TOKENS];
    char r_buf[1024] = {0};
    char w_buf[1024] = {0};
    const char* commands[static_cast<int>(Command::COUNT)] = {
        "SET", "GET", "DEL", "MOD",
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
