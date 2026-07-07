/*
 * @Author: uestcchuan2002 1992735052@qq.com
 * @Date: 2026-07-07 17:08:07
 * @LastEditors: uestcchuan2002 1992735052@qq.com
 * @LastEditTime: 2026-07-07 17:36:46
 * @FilePath: /01_应用/02_socket_server.c
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h>

#define SERVER_PORT 8888

int main(void) {

    struct sockaddr_in server_addr = {0};
    struct sockaddr_in client_addr = {0};
    char ip_str[20] = {0};
    int sockfd, connfd;
    int addrlen = sizeof(client_addr);
    char recv_buf[512];
    int ret;

    // 1. open socket, get socket file id
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (0 > sockfd) {
        perror("socket error");
        exit(EXIT_FAILURE);
    }

    // 2. Bind the socket to the specified port number
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(SERVER_PORT); 
    
    ret = bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (0 > ret) {
        perror("bind error");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // 3. Put the server into listening mode
    ret = listen(sockfd, 50);
    if (0 > ret) {
        perror("listen error");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // 4. Blocking and waiting for client connection
    connfd = accept(sockfd, (struct sockaddr*)&client_addr, &addrlen);
    if (0 > connfd) {
        perror("accept error");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("有客户端接入...\n");
    inet_ntop(AF_INET, &client_addr.sin_addr.s_addr, ip_str, sizeof(ip_str)); 
    printf("客户端主机的IP地址: %s\n", ip_str); 
    printf("客户端进程的端口号: %d\n", client_addr.sin_port);

    // 5. read receve data from client and print it
    for (;;) {

        // 5.1 clear receve buffer
        memset(recv_buf, 0x0, sizeof(recv_buf));

        // 5.2 read data
        ret = recv(connfd, recv_buf, sizeof(recv_buf), 0);
        if (0 > ret) {
            perror("recv error");
            close(connfd); 
            break;
        }

        // 5.3 print read data
        printf("from client: %s\n", recv_buf);

        // 5.4 if read data is "exit", close the socket and exit the program
        if (0 == strncmp("exit", recv_buf, 4)) {
            printf("server exit...\n"); 
            close(connfd); 
            break;
        }
    }

    close(sockfd);
    exit(EXIT_SUCCESS);

}














