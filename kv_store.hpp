#ifndef __KV_STORE_H__
#define __KV_STORE_H__

#include "reactor.hpp"

#define ENABLE_ARRAY_KV_ENGINE 1

#if ENABLE_ARRAY_KV_ENGINE
#include "kv_store_array.h"
#endif


class KVStoreConnection : public IConnection, public std::enable_shared_from_this<KVStoreConnection> 
{
public:
    KVStoreConnection(int fd) : IConnection(fd){}

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
                printf("SET\n");
                if(count < 3){
                    printf("invalid set command\n");
                    return -1;
                }
                res = kvs_array_set(tokens[1], tokens[2]);
                if(res == 0){
                    snprintf(w_buf, sizeof(w_buf), "SET %s %s OK\n", tokens[1], tokens[2]);
                }
                break;
            case static_cast<int>(Command::GET):
                printf("GET\n");
                value = kvs_array_get(tokens[1]);
                if(value){
                    snprintf(w_buf, sizeof(w_buf), "GET %s %s\n", tokens[1], value);
                }else{
                    snprintf(w_buf, sizeof(w_buf), "GET %s NULL\n", tokens[1]);
                }
                break;
            case static_cast<int>(Command::DEL):
                printf("DEL\n");
                break;
            case static_cast<int>(Command::MOD):
                printf("MOD\n");
                break;
            default:
                printf("unknow command\n");
                return -1;
        }
        // if(strlen(w_buf) > 0){

        //     Reactor::get_instance().modify_handler(fd_, EPOLLIN | EPOLLOUT | EPOLLET, shared_from_this());
        // }
        return 0;
    }

    void handle_read(){
        while(true){
            memset(r_buf, 0, sizeof(r_buf));
            int n = read(fd_, r_buf, sizeof(r_buf));
            if(n > 0){
                std::cout << "read: " << r_buf << " bytes: " << n << std::endl;
                
                int count = split_token();
                std::cout << "tokens count: " << count << std::endl;
                for(int i = 0; i < count; ++i){
                    std::cout << "token: " << tokens[i] << std::endl;
                }
                parse_protocol(count);
                // send(fd_, r_buf, n, 0);
            }else if(n == 0){
                printf("client close\n");
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
        if(strlen(w_buf) == 0){
            Reactor::get_instance().modify_handler(fd_, EPOLLIN | EPOLLET, shared_from_this());
            return;
        }
        
        while (true) {
            int n = send(fd_, w_buf, strlen(w_buf), 0);
            if(n > 0){
                printf("send: %s bytes: %d\n", w_buf, n);
                memset(w_buf, 0, sizeof(w_buf));
                Reactor::get_instance().modify_handler(fd_, EPOLLIN | EPOLLET, shared_from_this());
            } else if (n == 0) {
                Reactor::get_instance().unregister_handler(fd_);
                break;
            } else {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    /*缓冲区满，等待下次写事件*/
                    // Reactor::get_instance().modify_handler(fd_, EPOLLIN | EPOLLET, shared_from_this());
                    break;
                }
                perror("send");
                Reactor::get_instance().unregister_handler(fd_);
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

class KVStoreAcceptor : public IAcceptor, public std::enable_shared_from_this<KVStoreAcceptor> 
{
public:
    KVStoreAcceptor(int port) : IAcceptor(port){}
    KVStoreAcceptor(const std::string &ip, int port) : IAcceptor(ip, port){}

    void init(){
        Reactor::get_instance().register_handler(server_fd_, EPOLLIN, shared_from_this());
    }
private:
    void on_new_connection(int new_fd) override{
        auto client = std::make_shared<KVStoreConnection>(new_fd);
        // Reactor::get_instance().register_handler(new_fd, EPOLLIN | EPOLLET | EPOLLONESHOT, client);
        printf("new connection: %d\n", new_fd);
        Reactor::get_instance().register_handler(new_fd, EPOLLIN | EPOLLET, client);
    }
};

#endif
