#include "kv_store.hpp"
#include "reactor.hpp"

static int 
init_kv_engine(void) 
{
    int ret = -1;
#if ENABLE_ARRAY_KV_ENGINE
    ret = kvs_array_create(&global_array);
#endif

#if ENABLE_RBTREE_KV_ENGINE
    ret = kvs_rbtree_create(&global_rbtree);
#endif

    return ret;
}

static void
destroy_kv_engine(void)
{
#if ENABLE_ARRAY_KV_ENGINE
    kvs_array_destroy(&global_array);
#endif

#if ENABLE_RBTREE_KV_ENGINE
    kvs_rbtree_destroy(&global_rbtree);
#endif

}

int main (int argc, char *argv[]) {
    std::cout << "Usage: " << argv[0] << " <ip> <port> " << std::endl;
    if (argc != 3) {
        std::cerr << "Invalid arguments" << std::endl;
        return 1;
    }

    std::string ip = argv[1];
    int port = std::stoi(argv[2]);

    init_kv_engine();

#if ( ENABLE_NETWORK_SELECT == NETWORK_EPOLL )
    auto acceptor = std::make_shared<KVStoreEpollAcceptor>(ip, port);
    acceptor->init();
    Reactor::get_instance().run();
    goto out;
#elif ( ENABLE_NETWORK_SELECT == NETWORK_NTYCO )
    auto acceptor = std::make_shared<KVStoreNtycoAcceptor>(ip, port);
#elif ( ENABLE_NETWORK_SELECT == NETWORK_IOURING )
    auto acceptor = std::make_shared<KVStoreIouringAcceptor>(ip, port);
#endif
    acceptor->init();

    // std::cout << "Usage: " << argv[0] << " <ip> <port> " << std::endl;
    // if (argc != 3) {
    //     std::cerr << "Invalid arguments" << std::endl;
    //     return 1;
    // }

    // std::string ip = argv[1];
    // int port = std::stoi(argv[2]);

    // auto acceptor = std::make_shared<HttpAcceptor>(ip, port);
    // acceptor->init();
    // Reactor::get_instance().run();
out:
    destroy_kv_engine();
    return 0;
}