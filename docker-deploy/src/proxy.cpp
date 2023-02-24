#include "proxy.h"
#include "helper.h"
#include "request.h"

using namespace std;

#define URL_LIMIT 65536

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void ProxyServer::run(){
    int listen_socket_fd = setUpServer(this->port_num);
    const char * ip;
    int client_id;
    while (true) {
        int accept_socket_fd = acceptClients(listen_socket_fd, ip);
        //pthread_t thread;
        //pthread_mutex_lock(&mutex);
        Client * client = new Client(accept_socket_fd, client_id, ip);
        //Client input_client(accept_socket_fd, client_id, ip);

        // thread
        client_id++;
        //processRequest(&input_client); 
        //pthread_mutex_unlock(&mutex);
        //pthread_create(&thread, NULL, processCONNECT, client);
        //thread(processRequest, ref(input_client)).detach();
        thread t(&ProxyServer::processRequest, this, client);
        t.join();
    }
};

void * ProxyServer::processRequest(void * input_client){
    // receive request buffer from browser
    Client * client = (Client *) input_client;
    char buf[URL_LIMIT];
    int byte_count = recv(client->socket_fd, buf, sizeof(buf), 0);
    if (byte_count <= 0) {
        return NULL;
    }
    Request request ((string) buf);
    cout << "Received request " <<request.method_name<< " from host " <<request.host_name<< " port " <<request.port<< endl;
    
    cout << "method name: " << request.method_name << endl;
    cout << "host name: " <<request.host_name << endl;
    cout << "port: "<< request.port.c_str() << "happy" << endl;
    // step1: open client socket with web server
    int server_fd = setUpClient(request.host_name.c_str(), request.port.c_str());
    cout << "connect successfully with server " << server_fd << endl;
    client->server_fd = server_fd;

    if(request.method_name == "CONNECT") {
        processCONNECT(client);
    }
};


void * ProxyServer::processCONNECT(Client * client){
    cout << "process CONNECT" << endl;
    // step2: send an http response of "200 ok" back to the browser
    int byte_count = send(client->socket_fd, "HTTP/1.1 200 OK\r\n\r\n", 19, 0);
    if (byte_count <= 0) {
        return NULL;
    }
    cout << byte_count << endl;
    // step3: use IO multiplexing (select())
    struct timeval tv;
    fd_set readfds;
    int rv = 0;
    int client_socket = client->socket_fd;
    int server_socket = client->server_fd;
    int maxfd = client_socket > server_socket ? client_socket : server_socket;

    char buf[URL_LIMIT] = {0};
    while (true) {
        int byte_count;
        // tv.tv_sec = 10; 
        // tv.tv_usec = 500000; 
        // clear sets and add our descriptors
        FD_ZERO(&readfds);
        FD_SET(client_socket, &readfds); // socket one
        FD_SET(server_socket, &readfds); // socket two
        rv = select(maxfd + 1, &readfds, NULL, NULL, NULL);
        if (rv == -1) {
            perror("select"); // error occurred in select()
        } else if (rv == 0) {
            printf("Timeout occurred! No data after 10.5 seconds.\n");
        } else {
            // one or both of the descriptors have data
            if (FD_ISSET(client_socket, &readfds)) {
                byte_count = recv(client_socket, buf, sizeof(buf), 0);
                cout << "connect recv1 " << byte_count << endl;
                if(byte_count <= 0){
                    perror("recv error");
                    return NULL;
                }
                byte_count = send(server_socket, buf, byte_count, 0);
                cout << "connect send1 " << byte_count << endl;
                if (byte_count <= 0){
                    perror("send error");
                    return NULL;
                }

            }
            if (FD_ISSET(server_socket, &readfds)) {
                byte_count = recv(server_socket, buf, sizeof(buf), 0);
                cout << "connect recv2 " << byte_count << endl;
                if(byte_count <= 0){
                    perror("recv error");
                    return NULL;
                }
                byte_count = send(client_socket, buf, byte_count, 0);
                cout << "connect send2 " << byte_count << endl;
                if (byte_count <= 0){
                    perror("send error");
                    return NULL;
                }
            }
        }
    }
};


