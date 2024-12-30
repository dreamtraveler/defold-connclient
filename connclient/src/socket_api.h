#pragma once
#include "base_macro.h"

#ifndef OS_WIN32
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <cstddef>
#include <cstdlib>
#include <functional>
#else
#include <WS2tcpip.h>
#include <WinSock2.h>
#endif

using SOCKADDR = struct sockaddr;
using SOCKADDR_IN = struct sockaddr_in;
static const uint32_t SIZE_SOCKADDR_IN = sizeof(SOCKADDR_IN);

#define ERROR_STR_SIZE 256

namespace SocketAPI
{
int init_sock_env();
void free_sock_env();
int get_last_error();
SOCKET socket_ex(int domain, int type, int protocol);
bool bind_ex(SOCKET s, const struct sockaddr* addr, uint32_t addr_len);
bool connect_ex(SOCKET s, const struct sockaddr* addr, uint32_t addr_len);
bool connect_ex(SOCKET s, const struct sockaddr* addr, uint32_t addr_len, int timeout_ms);
bool listen_ex(SOCKET s, int backlog);
SOCKET accept_ex(SOCKET s, struct sockaddr* addr, uint32_t* addr_len);
bool getsockopt_ex(SOCKET s, int level, int opt_name, void* opt_val, uint32_t* opt_len);
bool setsockopt_ex(SOCKET s, int level, int opt_name, const void* opt_val, uint32_t opt_len);
int send_ex(SOCKET s, const void* buf, uint32_t len, int flags);
int sendto_ex(SOCKET s, const void* buf, int len, int flags, const struct sockaddr* to, int tolen);
int recv_ex(SOCKET s, void* buf, uint32_t len, int flags);
int recvfrom_ex(SOCKET s, void* buf, int len, int flags, struct sockaddr* from, uint32_t* fromlen);
bool closesocket_ex(SOCKET s);
bool ioctlsocket_ex(SOCKET s, int64_t cmd, uint64_t* argp);
bool getsocketnonblocking_ex(SOCKET s);
bool setsocketnonblocking_ex(SOCKET s, bool on);
uint32_t availablesocket_ex(SOCKET s);
bool shutdown_ex(SOCKET s, int how);
int select_ex(int maxfdp1, fd_set* readset, fd_set* writeset, fd_set* exceptset,
              struct timeval* timeout);
void set_send_buf(SOCKET s, int len);
void set_recv_buf(SOCKET s, int len);
bool set_tcp_no_delay(int fd);

SOCKET InitUDPListenSocket(const char* ip, uint32_t port, bool nonblock = true);
std::string GetSockAddrStr(const struct sockaddr_in& addr);
bool IsIPV4Addr(const char* s);

int GetHostAddressV4(const std::string& ip, uint16_t port, struct sockaddr* addr, int socketype);
};  // namespace SocketAPI
