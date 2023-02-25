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
            const char * ip;
            int server_fd;/////
            Client(int fd, int id, const char *ip): socket_fd(fd), server_fd(-1), id(id), ip(ip){};
    };
    // typedef struct client_t client;

    public:
    explicit ProxyServer (const char * port_num): port_num(port_num){};

   void run();
    static void * processRequest(void * input_client);

    // Connect method
    static void * processCONNECT(Client * client);

    // GET method
    static void processGET(ProxyServer::Client & client, Request & request, const char * message, int message_bytes);
    static void getChunked(Client & client, const char * server_rsp, int server_rsp_bytes); // Transfer-Encoding: chunked
    static bool determineChunked(char * rsp);//string
    
    static void getNoChunked(Client & client, char * server_rsp, int server_rsp_bytes); // Content-Length: <length>
    static int getContentLength(char * server_rsp, int server_rsp_bytes);


    // POST method
    // void processPost();
};

