#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <string>
#include <thread>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <string.h>

class ProxyServer {
    private:
    const char * port_num;
    struct client_t{
        int socket_fd;
        int id;
        const char * ip;
    };
    typedef struct client_t client;

    public:
    explicit ProxyServer (const char * port_num): port_num(port_num) {};

    void run(){};
    void * transferData(client * client){};
};