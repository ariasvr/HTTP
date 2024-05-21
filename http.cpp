#include <iostream>
#include <stdio.h> // handle errors
#include <string.h> // bzero
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>
#include <fstream>
using namespace std;

int connect_server(string url="/"){
    struct hostent* h_ip = gethostbyname(url.c_str());
    if (h_ip == NULL){
        perror("Error getting IP address of server");
        exit(1);
    }

    sockaddr_in serverAddr;
    bzero(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(80);
    bcopy(h_ip->h_addr, &serverAddr.sin_addr, h_ip->h_length);

    sockaddr &serverAddrCast = (sockaddr&)serverAddr;

    int sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd < 0){
        perror("Error creating socket");
        exit(1);
    }

    int on = 1;
    setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (const char*)&on, sizeof(int));

    if (connect(sockfd, &serverAddrCast, sizeof(serverAddr)) < 0){
        return -1;
    }

    return sockfd;
}

int readHeader(int sockfd){
    string header;
    char response_header[3073];
    int read_bytes = 480;
    while (strstr(response_header, "<!") == NULL && strstr(response_header, "\r\n\r\n") == NULL){
        bzero(response_header, 3073);
        recv(sockfd, response_header, read_bytes, MSG_PEEK);
        header = response_header;
        read_bytes += 480;
    }

    if (header.find("Content-Length"))
        return header.find("\r\n\r\n");

    else{
        return header.find("<!");}

    // int end_of_header = header.find("\r\n\r\n");
    // return end_of_header;
}

int find_content_length(char *response_header){

    string header(response_header);
    if (header.find("Content-Length: ") != string::npos){
        int content_length = stoi(header.substr(header.find("Content-Length: ") + 16, header.find("\r\n", header.find("Content-Length: "))));
        return content_length;
    }

    return -1;
}

string contentLength_Body(int sockfd, int content_length){
    //cout << content_length << endl;
    //return "";
    char content[content_length + 1];
    //string result;
    char chunk[101];
    int buff_size = 100;
    int count = 0;
    while(true){
        //cout << "hihihi" << endl;
        //cout << count << endl;
        bzero(chunk, 100);
        int save = recv(sockfd, chunk, 100, 0);
        string check = chunk;
        count = count + save;
        cout << content_length << endl;
        cout << chunk << endl;
        strncat(content, chunk, 100);
        //cout << result.size() << endl;
        if (count >= content_length){
            break;
        }
        // if (save.size() < buff_size){
        //     break;
        // }
    }
    memmove(content, content+4, strlen(content));
    string result = content;
    bzero(content, content_length + 1);
    //cout << result << endl;
    //cout << content << endl;
    return result;
}

string readEachChunk(int socketfd, int size_buffer, int pos){
    char content[size_buffer + 1];
    //string result;
    char chunk[101];
    int buff_size = 100;
    int count = 0;
    int size_s = 0;
    bool flag = true;
    bzero(content, size_buffer + 1);
    while(true){
        //cout << "asdffsad" << endl;
        //bzero(content, size_buffer + 1);
        //cout << "asdf" << endl;
        //cout << count << endl;
        if (size_buffer - count >= 0 && size_buffer - count <= buff_size){
            flag = false;
            size_s = size_buffer - count;
        }
        else{
            size_s = 100;
        }
        
        
        bzero(chunk, 101);
        if (recv(socketfd, chunk, size_s, MSG_PEEK) != size_s)continue;
        else{
            recv(socketfd, chunk, size_s, 0);
        }
        cout << chunk << endl;
        count = count + size_s;
        //cout << count << endl;
        strncat(content, chunk, size_s);
        //cout << "asdf: " << flag << endl;
        //cout << result.size() << endl;
        if (!flag){
            break;
        }
    }
    memmove(content, content + pos + 2, strlen(content));
    string result = content;
    bzero(content, size_buffer + 1);
    return result;
}

string transferEncodingChunked_Body(int socketfd){
    char content[1024];
    string body = "";
    recv(socketfd, content, 4, 0);
    char size[30];
    while(true){
        bzero(content, 1024);
        bzero(size, 30);
        recv(socketfd, size, 30, MSG_PEEK);
        string temp = size;
        //cout << temp << endl; 
        int pos = temp.find("\r\n");
        temp = temp.substr(0, pos);
        int size_buffer = (int)strtol(temp.c_str(), NULL, 16);
        //cout << size_buffer << endl;
        if (size_buffer <= 0)break;
        string cont = readEachChunk(socketfd, size_buffer + pos + 4, pos);
        //memmove(content, content+ pos + 2, strlen(content));
        body = body + cont;
        //break;
    }
    // memset(size, 0, 50);
    //char size_s[50];
    //recv(socketfd, size, 50, MSG_PEEK);
    return body;
}

string get_url(string url, string &filename){
    string url_ = url.substr(url.find("://") + 3);
    if (url_.find("/") != string::npos){
        url_ = url_.substr(0, url_.find("/"));
        filename = url.substr(url.find(url_) + url_.length() + 1);
    }
    return url_;
}

void write_to_file(string savefile, string content){
    ofstream file(savefile.c_str(), ios::binary | ios::out);
    file << content;
    file.close();
}

int main(int argc, char **argv){
    //string temp = "hihihihithihihi";
    //cout << temp.find("t")<< endl;
    // int n;
    // unsigned int m = sizeof(n);
    // int fdsocket;
    // fdsocket = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP); // example
    // getsockopt(fdsocket,SOL_SOCKET,SO_RCVBUF,(void *)&n, &m);
    // cout << "asdf" << endl;
    // cout << n << endl;
    string url = argv[1];
    string filename = "";
    string url_ = get_url(url, filename);
    
    cout << "Filename: " << filename << endl;
    cout << url_ << endl; 
    int sockfd = connect_server(url_);
    //cout << sockfd << "asdfasdf" << endl;
    if (sockfd < 0){
        perror("Error connecting to server");
        exit(1);
    }

// Send GET request
    string header = "GET /" + filename + " HTTP/1.1\r\n";
    header += "Host: " + url_ + "\r\n\r\n";
    
    int ret = write(sockfd, header.c_str(), header.length());

// Read header
    //cout << "asdfasdfffffffffffffff" << endl;
    int end_of_header = readHeader(sockfd);
    // Create the response_header buffer
    char response_header[end_of_header + 5];
    bzero(response_header, end_of_header + 5);
    recv(sockfd, response_header, end_of_header, 0); // MSG_PEEK to read without removing from buffer (read from beginning in the next time)
    //cout << response_header << endl;
    //cout << "tttttt" << endl;
    // Check content length/transfer encoding
    int content_length = find_content_length(response_header);
    //cout << "asdfasdfffffffffffffff" << endl;
    //cout << content_length << endl;
    string body;
    // recv()
    if (content_length != -1){
        cout << "Content length: " << content_length << endl;
        body = contentLength_Body(sockfd, content_length);
    }
    else{
        body = transferEncodingChunked_Body(sockfd);
        //cout << "Bodyyyyyyyyyyyyyyyyyyy" << endl;
        //cout << body;
    }
    //cout << body << endl;
    write_to_file(argv[2], body);

// Close connection    
    shutdown(sockfd, SHUT_RDWR);
    close(sockfd);
    return 0;
}