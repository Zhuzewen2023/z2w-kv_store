#include "kv_store.hpp"
#include "reactor.hpp"




int main (int argc, char *argv[]) {
    std::cout << "Usage: " << argv[0] << " <ip> <port> " << std::endl;
    if (argc != 3) {
        std::cerr << "Invalid arguments" << std::endl;
        return 1;
    }

    std::string ip = argv[1];
    int port = std::stoi(argv[2]);
    
#if ( ENABLE_NETWORK_SELECT == NETWORK_EPOLL )
    auto acceptor = std::make_shared<KVStoreEpollAcceptor>(ip, port);
    acceptor->init();
    Reactor::get_instance().run();
    return 0;
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
    return 0;
}