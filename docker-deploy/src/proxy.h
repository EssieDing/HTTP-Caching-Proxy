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

#include "request.h"

class ProxyServer {
    public: //
    const char * port_num;
    class Client{
        public:
            int socket_fd;
            int id;
            string ip;
            int server_fd;/////
            Client(int fd, int id, string ip): socket_fd(fd), server_fd(-1), id(id), ip(ip){};
    };
    // typedef struct client_t client;

    public:
    explicit ProxyServer (const char * port_num): port_num(port_num){};

   void run();
    void * processRequest(void * input_client);

    // Connect method
    void * processCONNECT(Client * client);

    // GET method
    void processGET(ProxyServer::Client & client, const char * message, int message_bytes);
    void getChunked(Client & client, const char * server_rsp, int server_rsp_bytes); // Transfer-Encoding: chunked
    bool determineChunked(char * rsp);//string
    
    void getNoChunked(Client & client, char * server_rsp, int server_rsp_bytes); // Content-Length: <length>
    int getContentLength(char * server_rsp, int server_rsp_bytes);


    // POST method
    void processPOST(ProxyServer::Client & client, char * message, int message_bytes);
};

