#include "proxy.h"
#include "helper.h"
#include <unordered_map>
#include <map>
#include <vector>
#include <sys/socket.h>
#include <string.h>
#include <stdio.h>

using namespace std;
#define URL_LIMIT 65536

pthread_mutex_t p_mutex = PTHREAD_MUTEX_INITIALIZER;

void ProxyServer::run(){

    int listen_socket_fd = setUpServer(this->port_num);
    string ip;
    int client_id;
    while (true) {
        int accept_socket_fd = acceptClients(listen_socket_fd, ip);
        //pthread_t thread;
        //pthread_mutex_lock(&p_mutex);
        Client * client = new Client(accept_socket_fd, client_id, ip);
        //Client client(accept_socket_fd, client_id, ip);

        cout<<"\nip: "<<ip<<endl;
        // thread
        client_id++;
        client->id = client_id;

        //thread t(&ProxyServer::processRequest, this, client);
        //t.join();
        //processRequest(&client); 
        //pthread_mutex_unlock(&p_mutex);
        //pthread_create(&thread, NULL, processRequest, client);
        //std::thread(processRequest, std::ref(client)).detach();
        thread t(&ProxyServer::processRequest, this, std::ref(client));
        t.detach();
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
    } else if(request.method_name == "GET") {
        cout<<"\n\n get test start \n\n";
        //processGET(*client,buf,byte_count);
        cacheGet(*client, request, buf, byte_count);
        cout<<"\n\n get test successfully \n\n";
    } else if(request.method_name == "POST") {
        processPOST(*client,buf,byte_count);
    }

    return NULL;////////////////////
}


void ProxyServer::processCONNECT(Client * client){
    // step2: send an http response of "200 ok" back to the browser
    int byte_count = send(client->socket_fd, "HTTP/1.1 200 OK\r\n\r\n", 19, 0);
    if (byte_count < 0) {
        return;
    }
    // step3: use IO multiplexing (select())
    fd_set readfds;
    int rv = 0;
    int client_socket = client->socket_fd;
    int server_socket = client->server_fd;
    int maxfd = client_socket > server_socket ? client_socket : server_socket;
    char buf[URL_LIMIT] = {0};
    while (true) {
        int byte_count, byte_count_send;
        //tv.tv_sec = 2; 
        //tv.tv_usec = 0; 
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
                byte_count = recv(client_socket, buf, sizeof(buf), MSG_NOSIGNAL);
                cout << "connect recv1 " << byte_count << endl;
                if(byte_count <= 0){
                    perror("recv error");
                    return;
                }
                byte_count_send = send(server_socket, buf, byte_count, MSG_NOSIGNAL);
                cout << "connect send1 " << byte_count << endl;
                if (byte_count_send <= 0){
                    perror("send error");
                    return;
                }

            }
            if (FD_ISSET(server_socket, &readfds)) {
                byte_count = recv(server_socket, buf, sizeof(buf), MSG_NOSIGNAL);
                cout << "connect recv2 " << byte_count << endl;
                if(byte_count <= 0){
                    perror("recv error");
                    return;
                }
                byte_count_send = send(client_socket, buf, byte_count, MSG_NOSIGNAL);
                cout << "connect send2 " << byte_count << endl;
                if (byte_count_send <= 0){
                    perror("send error");
                    return;
                }
            }
        }
    }

};



    

void ProxyServer::processGET(ProxyServer::Client & client, const char * message, int message_bytes){

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
    Response rsp(response);
    // cout<<response<<endl;//////////////////////test
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
        string request_str (server_rsp, server_rsp_bytes);
        Request request(request_str);
        getNoChunked(client,server_rsp,server_rsp_bytes, request, rsp);

        if (rsp.status_code == "200"){
            cout << "enterrrrrrrrrrrrrrr code    " << rsp.status_code <<endl;
            cacheCheck(rsp, request.request_line);
        }
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

void ProxyServer::getNoChunked(Client & client,char * server_rsp, int server_rsp_bytes, Request & request, Response & rsp){
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
            cout<<"GET-Content Length: recv unsuccessfully.\n";
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
    rsp.all_content = full_message;

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


void ProxyServer::processPOST(ProxyServer::Client & client, char * message, int message_bytes){
    cout<<"enter POST test:\n";
    // get content length
    int content_length=getContentLength(message,message_bytes);

    // get full request according to the remaining content_length
    // client->proxy
    cout<<"client->proxy\n";
    int current_length=0;
    string full_request(message, message_bytes);

    while (current_length < content_length) {
    char new_client_req[65536];
    int req_len = recv(client.socket_fd, new_client_req, sizeof(new_client_req), 0);

    if(req_len==-1){
        cout<<"POST-Content Length: recv unsuccessfully.\n";
        return;
    }
    if (req_len == 0) {
        break; // supposed to get full request
    }
    else{
        string req_now(new_client_req, req_len);
        full_request = full_request + req_now;
        current_length = current_length + req_len;
        }
    }

    // send request: proxy->server
    cout<<"proxy->server\n";
    vector<char> request;
    for (size_t i = 0; i < full_request.length(); i++) {
        request.push_back(full_request[i]);
    }
    const char * req_sent = request.data();

    int req_to_server;
    if ((req_to_server = send(client.server_fd, req_sent, full_request.size(),0))==-1){
        cout<<"POST: send to server unsuccessfully.\n";
        return;
    }

    // recv response: server->proxy
    cout<<"server->proxy\n";
    char server_rsp[65536];
    int server_rsp_len = recv(client.server_fd, server_rsp, sizeof(server_rsp), 0);
    if(server_rsp_len==-1){
        cout<<"POST: recv from server unsuccessfully.\n";
        return;
    }
    // send response: proxy->client
    cout<<"proxy->client\n";
    int rsp_to_client;
    if ((rsp_to_client = send(client.socket_fd, server_rsp,server_rsp_len,0))==-1){
        cout<<"POST: send to client unsuccessfully.\n";
        return;
    }

    cout<<"POST test Successfully\n";

}



void ProxyServer::cacheGet(ProxyServer::Client & client, Request request, const char * message, int message_bytes){
    cout << "cache get enter" << endl;
    string request_line = request.request_line;
    string requestStr (message, message_bytes);
    Response match = cache->get(request_line);
    cout << "max_age" << match.max_age << endl;
    // check if the request startline is in the cache map
    if (match.all_content == ""){ // has not found in cache map, cache miss
    // req and response whole process
        // printLog(": not in cache")
        processGET(client, message, message_bytes); // include send req to server and process response (check and cache)
        
    } else { // find in the cache map, cache hit, match is not empty
        // check no_cache
        if (match.no_cache){ // the response must be validated with the origin server before each reuse
            // printLog(": in cache, requires validation")
            // revalidate with origin server
            if (validCheck(client, match, requestStr) == false) { // false: modified, invalid, need revalidate
                processGET(client, message, message_bytes);
            } else {
                const char * return_msg = match.all_content.c_str();
                int return_msg_bytes = send(client.socket_fd, return_msg, sizeof(return_msg), 0);
                if (return_msg_bytes < 0){
                    return;
                }
            }
        } else {
            if (expireCheck(client, match) == true){ // true: is expired, must_revalidate and expires
                cache->remove(request_line); // expired, remove cache from mycache
                processGET(client, message, message_bytes);
            } else {
                const char * return_msg = match.all_content.c_str();
                int return_msg_bytes = send(client.socket_fd, return_msg, sizeof(return_msg), 0);
                if (return_msg_bytes < 0){
                    return;
                }
            }
        }
    }   
}

bool ProxyServer::validCheck(Client & client, Response & response, string request){
    if (response.etag == "" && response.last_modified == "") {
        return false;
    }
    if (response.etag != "") {
        string modified_etag = "If-None-Match: "+ response.etag +"\r\n";
        request.insert(request.length()-2, modified_etag);
    }
    if (response.last_modified != "") {
        string modified_lastModified = "If-Modified-Since: "+ response.last_modified +"\r\n";
        request.insert(request.length()-2, modified_lastModified);
    }
    const char * new_request = request.c_str();
    int new_request_bytes = send(client.server_fd, new_request, request.length() + 1, 0);
    if (new_request_bytes < 0) {
        // printLog
        return false;
    }

    char new_server_rsp[65536];
    int new_server_rsp_bytes = recv(client.server_fd, new_server_rsp, sizeof(new_server_rsp), 0);
    if (new_server_rsp_bytes < 0) {
        //printLog
        return false;
    }
    std::string new_server_rsp_str(new_server_rsp, new_server_rsp_bytes);
    Response new_response (new_server_rsp_str);
    // check status code
    if (new_response.status_code == "304") {
        // printLog("not modified")
        return true;
    } else {
        return false;
    }
};

bool expireCheck_Expires (string timeStr){
  string current = timeString(getUTCurrentime());
  struct tm t1 = {0};
  getTimeStruct(t1, current);

//   struct tm t2 = {0};
//   getTimeStruct(t2, timeStr);
  struct tm * t2 = getUTCtime(timeStr);

  int seconds = difftime(mktime(t2), mktime(&t1));//t1-t2 
  return seconds < 0;
}

bool expireCheck_maxAge(int max_age,string timeStr){
  string current = timeString(getUTCurrentime());
 
  struct tm t1 = {0};
  getTimeStruct(t1, current);

  struct tm * t2 = getUTCtime(timeStr);
  //getTimeStruct(t2, timeStr);

  int seconds = difftime(mktime(&t1), mktime(t2));//t1-t2
  return seconds > max_age;
}

bool ProxyServer::expireCheck(Client & client, Response & response){ // true: isExpired
    if (response.max_age != -100){
        if (expireCheck_maxAge(response.max_age, response.date)){ // true: is expired
            return true;
        }
    } else if (response.expires != "") {
        if (expireCheck_Expires(response.expires)){
            return true;
        }
    }
    return false;
};


// response status code is 200 can be cached
void ProxyServer::cacheCheck (Response & response, string request_line) { // chunked and unchunked both need?
    if (response.no_store) { // should not be cached
        // printlog ": not cacheable because NO STORE" << endl;
        return;
    } else {
        if (response.no_cache) { // should be cached
        //printlog ": cached, but requires re-validation"
        } else if (response.max_age != -100) {
            //printLog
        } else if (response.expires != "") {
            //printLog
        }
        cache->put(request_line, response);
    }
};

