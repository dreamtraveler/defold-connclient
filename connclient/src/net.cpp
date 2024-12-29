#include "net.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include <sys/un.h>
#include <unistd.h>

#include <cstring>

#include "base_macro.h"

namespace net
{

int SetNonBlock(int fd)
{
    int flags = fcntl(fd, F_GETFL);
    if (flags == -1) {
        return -1;
    }
    flags |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) == -1) {
        return -1;
    }
    return 0;
}

uint32_t IpToInt(const char* ip)
{
    struct in_addr s;
    int ret = inet_pton(AF_INET, ip, (void*)&s);
    if (ret <= 0) return 0;
    return ntohl(s.s_addr);
}

void SetSocketOpt(int fd)
{
    // 设置套接字重用
    int reuse_addr_ok = 1;
    CHECK_VOID(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr_ok, sizeof(reuse_addr_ok)) !=
               -1);

    // 设置端口重用
    int reuse_port_ok = 1;
    CHECK_VOID(setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &reuse_port_ok, sizeof(reuse_port_ok)) !=
               -1);

    // 心跳机制
    int flags = 1;
    CHECK_VOID(setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &flags, sizeof(flags)) != -1);

    // 将数据继续发送至对端, 优雅关闭连接
    struct linger ling = {0, 0};
    CHECK_VOID(setsockopt(fd, SOL_SOCKET, SO_LINGER, &ling, sizeof(ling)) != -1);

    // 禁用Nagle算法
    int enable = 1;
    CHECK_VOID(setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (void*)&enable, sizeof(enable)) != -1);
}

int GetHostAddressV4(const std::string& ip, uint16_t port, struct sockaddr* addr, int socktype)
{
    addrinfo hints;
    addrinfo* servinfo = nullptr;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = socktype;

    char _port[6] = {0};
    snprintf(_port, 6, "%d", port);

    int ret = getaddrinfo(ip.c_str(), _port, &hints, &servinfo);
    if (ret != 0) {
        return -1;
    }
    for (auto* p = servinfo; p != nullptr; p = p->ai_next) {
        if (p->ai_addr->sa_family == AF_INET) {
            memcpy(addr, p->ai_addr, p->ai_addrlen);
            freeaddrinfo(servinfo);
            return 0;
        }
    }

    if (servinfo != nullptr) {
        freeaddrinfo(servinfo);
    }
    return -1;
}

int SetTcpNoDelay(int fd, bool enable)
{
    int val = enable ? 1 : 0;
    if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(val)) == -1) {
        return -1;
    }
    return 0;
}

int SetTcpKeepAlive(int fd, bool enable)
{
    int val = enable ? 1 : 0;
    if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &val, sizeof(val)) == -1) {
        return -1;
    }
    return 0;
}

int SetSendBuffer(int fd, int size)
{
    if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &size, sizeof(size)) == -1) {
        return -1;
    }
    return 0;
}

int SetRecvBuffer(int fd, int size)
{
    if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size)) == -1) {
        return -1;
    }
    return 0;
}

int SetReuseAddr(int fd)
{
    int enable = 1;
    /* Make sure connection-intensive things
     * will be able to close/open sockets a zillion of times */
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) == -1) {
        return -1;
    }
    return 0;
}

int SetReusePort(int fd)
{
    int enable = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(enable)) == -1) {
        return -1;
    }
    return 0;
}

void ToIpV4(uint32_t ip, char ip_str[20])
{
    inet_ntop(AF_INET, (void*)&ip, ip_str, 16);
}

}  // namespace net