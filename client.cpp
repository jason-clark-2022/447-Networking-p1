// client.cpp
// Jason Clark
// 800617442

#include <iostream>
#include <fstream>
#include <string>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define MAXBUFLEN 1000

using namespace std;

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) 
    {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
void read_config(char* port, char* server_ip)
{
    int count;
    ifstream config;
    string line;
    string delimiter = "=";
    string args[2];
    size_t position;
    
    config.open("client.conf");

    // erase the content from beginning of string to delim, including delim
    while(getline(config, line))    
    {
        position = line.find(delimiter); 
        line.erase(0, position+delimiter.length());
        args[count] = line;
        count++;
        if(count == 2)
            break;
    }
    strcpy(server_ip, args[0].c_str());
    strcpy(port, args[1].c_str());
}

int main(int argc, char *argv[])
{
    int sockfd, numbytes, addr_info_status;   
    long long_port;

    char out_buf[MAXBUFLEN];
    char in_buf[MAXBUFLEN];
    char server_ip_str[INET6_ADDRSTRLEN];
    char port[50];
    char server_ip[INET_ADDRSTRLEN];
    char* e;

    struct addrinfo hints, *serv_info, *serv_info_ptr;
    struct sockaddr_in serv_addr;
    struct timeval tv;

    socklen_t serv_addr_len;

    read_config(port, server_ip);
    
    long_port = strtol(port, &e, 10);
    
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    if ((addr_info_status = getaddrinfo(server_ip, port, &hints, &serv_info)) != 0) {
        perror("error, getaddrinfo");
        exit(EXIT_FAILURE);
    }

    tv.tv_sec = 5;  // how many seconds
    tv.tv_usec = 0;

    for(serv_info_ptr = serv_info; serv_info_ptr != NULL; serv_info_ptr = serv_info_ptr->ai_next) 
    {
        if ((sockfd = socket(serv_info_ptr->ai_family, serv_info_ptr->ai_socktype, serv_info_ptr->ai_protocol)) == -1) 
        {
            perror("client: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv) == -1) 
        {
            perror("setsockopt");
            exit(1);
        }
        break;
    }
    freeaddrinfo(serv_info); 

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(long_port);
    inet_aton(server_ip, &serv_addr.sin_addr);
    
    printf("destination address: %s\n", server_ip);
    printf("port: %s\n", port);
    
    while(true)
    {
        cout << ">>";
        cin.getline(out_buf, MAXBUFLEN);
        
        // send message from out_buf to server
        if((numbytes = sendto(sockfd, out_buf, sizeof out_buf, 0, (struct sockaddr*)&serv_addr, sizeof serv_addr))==-1)
        {
            perror("sendto");
            exit(EXIT_FAILURE);
        }

        serv_addr_len = sizeof serv_addr;

        // recieve from server and store to in_buf
        if((numbytes = recvfrom(sockfd, in_buf, MAXBUFLEN+1, 0, (struct sockaddr *)&serv_addr, &serv_addr_len)) == -1)
        {
            printf("\n<<Request timed out, try again.\n");
            continue;
        }

        printf("<<%s", in_buf);
        
        if((out_buf[0] == 'B' && out_buf[1] == 'Y' && out_buf[2] == 'E')&&(in_buf[0] == '2' && in_buf[1] == '0' && in_buf[2] == '0'))
        {
            break;
        }
    }

    
    close(sockfd);
    return 0;
}