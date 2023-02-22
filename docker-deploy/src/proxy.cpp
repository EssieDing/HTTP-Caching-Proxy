#include "proxy.h"
#include "helper.h"

using namespace std;

void ProxyServer::run(){
    int listen_socket_fd = setUpServer(this->port_num);
    const char * ip;
    int client_id;
    while (true) {
        int accept_socket_fd = acceptClients(listen_socket_fd, ip);
        client client;
        client.socket_fd = accept_socket_fd;
        client.id = client_id;
        client.ip = ip;

        // thread
        client_id++;
        transferData(&client);
    }
};

void * ProxyServer::transferData(client * client){
    char request_msg[65535];
    int len = recv(client->socket_fd, request_msg, 65535, 0);
    if (len <= 0) {
        return NULL;
    }
}