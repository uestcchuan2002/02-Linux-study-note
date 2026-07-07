# Linux Socket 编程基础笔记

---

## 一、Socket 概述

Socket（套接字）是 Linux 下网络通信的编程接口，封装了 TCP/IP 协议栈，提供了一组系统调用供用户态程序进行网络通信。

### 通信模型

```
         Server                              Client
           |                                    |
      socket()                              socket()
           |                                    |
         bind()                                 |
           |                                    |
        listen()                                |
           |                                    |
        accept()  <------- 三次握手 -------  connect()
           |                                    |
        recv()    <------- 数据传输 -------  send()
        send()    ------- 数据传输 ------>  recv()
           |                                    |
        close()                              close()
```

---

## 二、头文件

```c
#include <sys/types.h>
#include <sys/socket.h>   // socket / bind / listen / accept / connect / send / recv
#include <arpa/inet.h>    // htonl / htons / ntohl / ntohs / inet_pton / inet_ntop
#include <netinet/in.h>   // sockaddr_in 结构体 / INADDR_ANY
#include <unistd.h>       // close
#include <string.h>       // memset
#include <stdio.h>
#include <stdlib.h>
```

---

## 三、关键数据结构

### sockaddr_in（IPv4 地址结构体）

```c
struct sockaddr_in {
    sa_family_t    sin_family;  // 地址族，IPv4 固定填 AF_INET
    in_port_t      sin_port;    // 端口号（16 位）   **网络字节序**
    struct in_addr sin_addr;    // IP 地址（32 位）  **网络字节序**
};

struct in_addr {
    in_addr_t s_addr;           // IP 地址，类型为 uint32_t
};
```

### 通用 socket 地址结构体（用于类型转换）

```c
struct sockaddr {
    sa_family_t sa_family;      // 地址族
    char        sa_data[14];    // 地址数据
};
```

> **注意：** `bind / accept / connect` 等函数的参数类型为 `struct sockaddr*`，实际使用时需要将 `struct sockaddr_in*` 强制转换为 `struct sockaddr*`。

---

## 四、字节序转换函数

网络协议规定使用**大端字节序（网络字节序）**，而主机可能为小端（如 x86），因此需要转换。

| 函数       | 含义                        |
| ---------- | --------------------------- |
| `htons(s)` | **H**ost **to** **N**etwork **S**hort (16bit) |
| `htonl(l)` | **H**ost **to** **N**etwork **L**ong  (32bit) |
| `ntohs(s)` | **N**etwork **to** **H**ost **S**hort (16bit) |
| `ntohl(l)` | **N**etwork **to** **H**ost **L**ong  (32bit) |

> 记忆：`h`=host, `n`=network, `s`=short(端口号), `l`=long(IP地址)

---

## 五、IP 地址转换函数

### inet_pton（字符串 IP -> 网络字节序二进制）

```c
int inet_pton(int af, const char *src, void *dst);
// af:   地址族 AF_INET
// src:  点分十进制 IP 字符串，如 "192.168.1.1"
// dst:  输出缓冲区，指向 struct in_addr
// 返回值: 1=成功, 0=格式无效, -1=错误
```

**用法示例（客户端绑定服务器 IP）：**
```c
inet_pton(AF_INET, "192.168.2.150", &server_addr.sin_addr);
```

### inet_ntop（网络字节序二进制 -> 字符串 IP）

```c
const char *inet_ntop(int af, const void *src, char *dst, socklen_t size);
// af:   地址族 AF_INET
// src:  指向 struct in_addr
// dst:  输出字符串缓冲区
// size: dst 缓冲区大小
// 返回值: 成功返回 dst 指针, 失败返回 NULL
```

**用法示例（服务端打印客户端 IP）：**
```c
char ip_str[20];
inet_ntop(AF_INET, &client_addr.sin_addr.s_addr, ip_str, sizeof(ip_str));
printf("客户端IP: %s\n", ip_str);
```

---

## 六、核心函数详解

### 6.1 socket() — 创建套接字

```c
int socket(int domain, int type, int protocol);
// domain:   地址族，IPv4 用 AF_INET，本地通信用 AF_UNIX
// type:     套接字类型，SOCK_STREAM(TCP) / SOCK_DGRAM(UDP)
// protocol: 协议，通常填 0（自动选择）
// 返回值:   成功返回套接字文件描述符，失败返回 -1
```

### 6.2 bind() — 绑定地址与端口

```c
int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
// 将套接字与指定的 IP + 端口绑定
// 服务端必须调用，客户端通常不调用（由系统自动分配）
// 返回值: 成功 0，失败 -1
```

### 6.3 listen() — 进入监听状态

```c
int listen(int sockfd, int backlog);
// 将主动套接字转为被动套接字，开始接受连接请求
// backlog: 未完成连接队列的最大长度（典型值 50~128）
// 返回值: 成功 0，失败 -1
```

### 6.4 accept() — 接受客户端连接

```c
int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
// 从已完成连接队列中取出一个连接（阻塞等待）
// addr:     输出参数，保存客户端的地址信息
// addrlen:  输入输出参数，传入 addr 的大小，传出实际大小
// 返回值:   成功返回新的连接套接字描述符(connfd)，失败返回 -1
```

> **重要：** accept 返回的是**新**的套接字描述符 `connfd`，后续与这个客户端的数据收发都使用 `connfd`，而非原来的监听套接字 `sockfd`。

### 6.5 connect() — 连接服务器

```c
int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
// 客户端调用，向服务器发起连接请求（TCP 三次握手）
// 返回值: 成功 0，失败 -1
```

### 6.6 send() — 发送数据

```c
ssize_t send(int sockfd, const void *buf, size_t len, int flags);
// 向已连接的套接字发送数据
// flags: 通常填 0
// 返回值: 成功返回实际发送字节数，失败返回 -1
```

### 6.7 recv() — 接收数据

```c
ssize_t recv(int sockfd, void *buf, size_t len, int flags);
// 从已连接的套接字接收数据
// flags: 通常填 0，阻塞模式下会阻塞等待数据到来
// 返回值: >0=接收字节数, 0=对端已关闭连接, -1=出错
```

### 6.8 close() — 关闭套接字

```c
int close(int fd);
// 关闭套接字，释放资源
```

---

## 七、服务端完整流程（TCP）

```
socket()  ──►  bind()  ──►  listen()  ──►  accept()  ──►  recv()/send()  ──►  close()
```

### 代码骨架

```c
int sockfd, connfd;
struct sockaddr_in server_addr = {0};
struct sockaddr_in client_addr = {0};
socklen_t addrlen = sizeof(client_addr);

// 1. 创建 socket
sockfd = socket(AF_INET, SOCK_STREAM, 0);

// 2. 绑定地址和端口
server_addr.sin_family = AF_INET;
server_addr.sin_addr.s_addr = htonl(INADDR_ANY);  // 监听所有网卡
server_addr.sin_port = htons(8888);
bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));

// 3. 监听
listen(sockfd, 50);

// 4. 接收连接（阻塞）
connfd = accept(sockfd, (struct sockaddr*)&client_addr, &addrlen);

// 5. 收发数据
recv(connfd, buf, sizeof(buf), 0);
send(connfd, buf, strlen(buf), 0);

// 6. 关闭
close(connfd);
close(sockfd);
```

### 关键细节说明

| 要点 | 说明 |
|------|------|
| `INADDR_ANY` | 表示绑定本机所有 IP 地址，需用 `htonl()` 转为网络字节序 |
| `htons(8888)` | 端口号必须转为网络字节序 |
| `memset(buf, 0, sizeof(buf))` | 每次接收前清空缓冲区，避免残留数据干扰 |
| `strncmp("exit", buf, 4)` | 用 `strncmp` 而非 `strcmp`，因为 `fgets` 会读入 `\n` |

---

## 八、客户端完整流程（TCP）

```
socket()  ──►  connect()  ──►  send()/recv()  ──►  close()
```

### 代码骨架

```c
int sockfd;
struct sockaddr_in server_addr = {0};

// 1. 创建 socket
sockfd = socket(AF_INET, SOCK_STREAM, 0);

// 2. 连接服务器
server_addr.sin_family = AF_INET;
server_addr.sin_port = htons(8888);
inet_pton(AF_INET, "192.168.2.150", &server_addr.sin_addr);
connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));

// 3. 收发数据
send(sockfd, buf, strlen(buf), 0);

// 4. 关闭
close(sockfd);
```

### 关键细节说明

| 要点 | 说明 |
|------|------|
| `inet_pton` | 将字符串 IP 转为 `in_addr` 结构体（推荐代替已废弃的 `inet_addr`） |
| `fgets(buf, sizeof(buf), stdin)` | 从标准输入读取一行（含 `\n`），适合交互式发送 |

---

## 九、常用常量速查

| 常量 | 含义 |
|------|------|
| `AF_INET` | IPv4 地址族 |
| `AF_INET6` | IPv6 地址族 |
| `SOCK_STREAM` | TCP 流式套接字（可靠、面向连接） |
| `SOCK_DGRAM` | UDP 数据报套接字（不可靠、无连接） |
| `INADDR_ANY` | 绑定所有网卡 IP（值为 0） |

---

## 十、编译与运行

```bash
# 编译
gcc -o server 02_socket_server.c
gcc -o client 02_socket_client.c

# 先启动服务端
./server

# 新终端启动客户端
./client
```

---

## 十一、常见错误排查

| 错误 | 原因 |
|------|------|
| `bind error: Address already in use` | 端口被占用，等一会儿或换端口 |
| `connect error: Connection refused` | 服务器未启动或端口/ip 不对 |
| `send error: Broken pipe` | 对端已关闭连接，本端仍在发送 |
| `recv` 返回 0 | 对端正常关闭了连接 |

---

*生成日期: 2026-07-07*  
*基于 02_socket_server.c 和 02_socket_client.c 内容整理*
