#include "proxy.h"
#include "helper.h"
#include <vector>
#include <sys/socket.h>
#include <string.h>
#include <stdio.h>

using namespace std;
#define URL_LIMIT 65536

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void ProxyServer::run(){
    int listen_socket_fd = setUpServer(this->port_num);
    const char * ip;
    int client_id;
    while (true) {
        int accept_socket_fd = acceptClients(listen_socket_fd, ip);
        pthread_t thread;
        pthread_mutex_lock(&mutex);
        Client * client = new Client(accept_socket_fd, client_id, ip);
        //Client client(accept_socket_fd, client_id, ip);

        // thread
        client_id++;
        //processRequest(&client); 
        pthread_mutex_unlock(&mutex);
        pthread_create(&thread, NULL, processRequest, client);
        //thread(processRequest, ref(input_client)).detach();
        //thread t(&ProxyServer::processRequest, this, client);
        //t.join();
    }
}

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
    
    // cout << "method name: " << request.method_name << endl;
    // cout << "host name: " <<request.host_name << endl;
    // cout << "port: "<< request.port.c_str() << "happy" << endl;
    // step1: open client socket with web server
    int server_fd = setUpClient(request.host_name.c_str(), request.port.c_str());
    cout << "connect successfully with server " << server_fd << endl;
    client->server_fd = server_fd;


    if(request.method_name == "CONNECT") {
        processCONNECT(client);
    }
    if(request.method_name == "GET") {
        cout<<"\n\n get test start \n\n";
        processGET(*client,request,buf,byte_count);
        cout<<"\n\n get test successfully \n\n";
    }

    return NULL;////////////////////
}


void * ProxyServer::processCONNECT(Client * client){
    // step2: send an http response of "200 ok" back to the browser
    int byte_count = send(client->socket_fd, "HTTP/1.1 200 OK\r\n\r\n", 19, 0);
    if (byte_count <= 0) {
        return NULL;
    }
    // step3: use IO multiplexing (select())
    fd_set readfds;
    int maxfd = 0;
    int rv = 0;
    int client_socket = client->socket_fd;
    int server_socket = client->server_fd;

    char buf[URL_LIMIT] = {0};
    while (true) {
        int byte_count, byte_count_send;
        //tv.tv_sec = 2; 
        //tv.tv_usec = 0; 
        // clear sets and add our descriptors
        FD_ZERO(&readfds);
        FD_SET(client_socket, &readfds); // socket one
        FD_SET(server_socket, &readfds); // socket two
        maxfd = client_socket > server_socket ? client_socket : server_socket;
        rv = select(maxfd + 1, &readfds, NULL, NULL, NULL);
        if (rv == -1) {
            perror("select"); // error occurred in select()
        } else if (rv == 0) {
            printf("Timeout occurred! No data after 10.5 seconds.\n");
        } else {
            // one or both of the descriptors have data
            if (FD_ISSET(client_socket, &readfds)) {
                byte_count = recv(client_socket, buf, sizeof(buf), MSG_NOSIGNAL);
                cout << "connect recv1 " << byte_count << endl;
                if(byte_count <= 0){
                    perror("recv error");
                    return NULL;
                }
                byte_count_send = send(server_socket, buf, byte_count, MSG_NOSIGNAL);
                cout << "connect send1 " << byte_count << endl;
                if (byte_count_send <= 0){
                    perror("send error");
                    return NULL;
                }

            }
            if (FD_ISSET(server_socket, &readfds)) {
                byte_count = recv(server_socket, buf, sizeof(buf), MSG_NOSIGNAL);
                cout << "connect recv2 " << byte_count << endl;
                if(byte_count <= 0){
                    perror("recv error");
                    return NULL;
                }
                byte_count_send = send(client_socket, buf, byte_count, MSG_NOSIGNAL);
                cout << "connect sned2 " << byte_count << endl;
                if (byte_count_send <= 0){
                    perror("send error");
                    return NULL;
                }
            }
        }
    }

};




void ProxyServer::processGET(ProxyServer::Client & client, Request & request, const char * message, int message_bytes){

    //ssize_t send(int sockfd, const char *buf, size_t len, int flags);
    //ssize_t recv(int socket, void *buffer, size_t length, int flags);

    // send request: proxy->server
    ssize_t num=send(client.server_fd,message,message_bytes, 0);
    if(num==-1){
        cout<<"send unsuccessfully.\n";//////
        return;
    }

    // recieve response: server->proxy
    char server_rsp[65536];
    int server_rsp_bytes = recv(client.server_fd,server_rsp, sizeof(server_rsp),0);
    
    if(server_rsp_bytes==-1){
        cout<<"recv unsuccessfully.\n";//////
        return;
    }
    if(server_rsp_bytes==0){}//////

    // prase the response, determine chunked or not
    string temp_response(server_rsp);
    string response=temp_response.substr(0,server_rsp_bytes);//////
    cout<<response<<endl;//////////////////////test
    // bool chunked=determineChunked(response);
    bool chunked=determineChunked(server_rsp);

    if(chunked){
        // for chunked
        cout<<"enter chunked:\n";

        // test:
        const char * test1=strstr(server_rsp,"chunked");
        if(test1!=nullptr){cout<<"right1\n";}else{cout<<"wrong1\n";}
        // // test:
        // const char * test2=strstr(server_rsp,"Content-Length: ");
        // if(test2==nullptr){cout<<"right2\n";}else{cout<<"wrong2\n";}
        // test end

        getChunked(client,server_rsp,server_rsp_bytes);
        cout<<"Chunked finished.\n";     
    }else{

        const char * test2=strstr(server_rsp,"Content-Length: ");
        if(test2==nullptr){
            // no Transfer-Encoding && no Content-Length: error
            cout<<"error response in GET.";
            return;
            }
        // for non-chunked (length)
        cout<<"enter content length:\n";
        getNoChunked(client,server_rsp,server_rsp_bytes);
        cout<<"Content length finished.\n"; 
    }
}

bool ProxyServer::determineChunked(char * rsp){// string rsp
    // size_t ans;
    // if((ans=rsp.find_first_of("chunked"))!=string::npos){
    //     return true;
    // }
    // else{
    //     return false;
    // }
    const char * test1=strstr(rsp,"chunked");
    if(test1!=nullptr){return true;}else{return false;}
}

void ProxyServer::getChunked(Client & client, const char * server_rsp, int server_rsp_bytes){
    // send the first response recieved from server to client: proxy->client
    int num = send(client.socket_fd,server_rsp,server_rsp_bytes, 0);
    if (num == -1) {
        cout<<"send the response recieved from server to client unsuccessfully.\n";
        return;
    }
    char message[65536];
    while (1) {
        cout<<"loop once\n";
        // recv from server
        int recv_length=recv(client.server_fd,message, sizeof(message), 0);
        if(recv_length==0){
            // no messages are available to be recieved
            return;
        }
        if(recv_length==-1){
            cout<<"chunked: recv unsuccessfully.\n";
            return;
        }
        // send to client
        int send_length=send(client.socket_fd,message,recv_length, 0); 
        if(send_length==-1){
            cout<<"chunked: send unsuccessfully.\n";
            return;
        }
    }
}

void ProxyServer::getNoChunked(Client & client,char * server_rsp, int server_rsp_bytes){
    string full_message(server_rsp, server_rsp_bytes);
    // get Content-Length: 
    // help to determine whether we have received the full message. 
    // < Content-Length: don't have the full message yet, wait in a while loop 
    // until you have received >= Content-Length

    cout<<"enter getNonChunked:\n";
    int content_length=getContentLength(server_rsp,server_rsp_bytes);
    cout<<"get remaining content length: "<<content_length<<endl;

    // get full message according to the remaining content_length 
    int current_recieved_length = 0;
    while (current_recieved_length < content_length) {
        char new_server_rsp[65536];
        int rsp_len = recv(client.server_fd, new_server_rsp, sizeof(new_server_rsp), 0);

        if(rsp_len==-1){
            cout<<"Content Length: recv unsuccessfully.\n";
            return;
        }
        if (rsp_len == 0) {
            break; // supposed to get full message
        }
        else{
            // msg_len > 0: continue to recv from server
            string rsp_now(new_server_rsp, rsp_len);
            // update the message
            full_message = full_message + rsp_now;
            current_recieved_length = current_recieved_length + rsp_len;
        }
    }

    // use vector.data() to get the message to be sent
    vector<char> message;
    for (size_t i = 0; i < full_message.length(); i++) {
        message.push_back(full_message[i]);
    }
    const char * msg_sent = message.data();

    // send to client
    int rsp_to_client;
    if ((rsp_to_client = send(client.socket_fd, msg_sent, full_message.size(),0))==-1){
        cout<<"Content Length: send unsuccessfully.\n";
        return;
    }


    // if not get: ??
}


int ProxyServer::getContentLength(char * server_rsp,int server_rsp_bytes){
    // get the pointer pointing to start of the "Content-Length"
    cout<<"enter get content length:\n";
    char * found_length=strstr(server_rsp,"Content-Length: ");
    size_t size=strlen("Content-Length: ");
    found_length=found_length+size;

    cout<<"run here 1\n";
    int content_length=0;
    while(isdigit(*found_length)){
        // cout<<"run here 2\n";
        content_length *= 10;
        content_length += *found_length - '0';
        found_length++;
    }
    // remaining_content_length = content_length - (recv_size - header_size)
    cout<<"run here 3\n";
    std::string rsp(server_rsp, server_rsp_bytes);
    size_t header_end=rsp.find_first_of("\r\n\r\n");
    content_length = content_length - (server_rsp_bytes - header_end - strlen("\r\n\r\n"));

    cout<<" get content length finished.\n";
    return content_length;
}





