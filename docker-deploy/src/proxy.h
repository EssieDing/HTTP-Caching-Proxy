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
    //private:
    const char * port_num;

    public:
        class Client{
            public:
            int socket_fd; // socket created from proxy end as server to accept request from browser
            int server_fd; // socket created from proxy end as client to connect to web server
            int id;
            const char * ip;
            Client(int fd, int id, const char *ip): socket_fd(fd), server_fd(-1), id(id), ip(ip){};
        };
    ProxyServer (const char * port_num): port_num(port_num) {};

    void run();
    void * processRequest(void * client);
    void * processCONNECT(Client * client);
};
