#include "socket_api.h"

#include <cstdio>
#include <cstring>
#include <sstream>

#include "base_macro.h"
#include "common_def.h"
#include "file_api.h"


int SocketAPI::get_last_error()
{
#ifndef OS_WIN32
    return errno;
#else
    int err = WSAGetLastError();
    switch (err) {
        case WSAEINTR:
            return EINTR;
        case WSAECONNABORTED:
            return ECONNABORTED;
        case WSAEWOULDBLOCK:
            return EWOULDBLOCK;
        default:
            return err;
    }
#endif
}

int SocketAPI::init_sock_env()
{
#ifdef OS_WIN32
    WSADATA Ws;
    if (WSAStartup(MAKEWORD(2, 2), &Ws) != 0) {
        return -1;
    }
#endif
    return 0;
}

void SocketAPI::free_sock_env()
{
#ifdef OS_WIN32
    WSACleanup();
#endif
}

SOCKET SocketAPI::socket_ex(int domain, int type, int protocol)
{
    SOCKET s = ::socket(domain, type, protocol);
    return s;
}

bool SocketAPI::bind_ex(SOCKET s, const struct sockaddr* addr, uint32_t addr_len)
{
    if (::bind(s, addr, addr_len) == SOCKET_ERROR) {
        return false;
    }
    return true;
}


bool SocketAPI::connect_ex(SOCKET s, const struct sockaddr* addr, uint32_t addr_len)
{
    if (connect(s, addr, addr_len) == SOCKET_ERROR) {
        return false;
    }

    return true;
}

bool SocketAPI::connect_ex(SOCKET s, const struct sockaddr* addr, uint32_t addr_len, int timeout_ms)
{
    if (!setsocketnonblocking_ex(s, true)) return false;

    struct timeval tm;
    tm.tv_sec = timeout_ms / 1000;
    tm.tv_usec = (timeout_ms % 1000) * 1000;

    if (connect(s, addr, addr_len) != SOCKET_ERROR) {
        return true;
    }

    fd_set w, e;

    FD_ZERO(&w);
    FD_SET(s, &w);
    FD_ZERO(&e);
    FD_SET(s, &e);

    int ret = select_ex(s + 1, nullptr, &w, &e, &tm);
    if (ret <= 0) {
        // 有错误或者超时
        closesocket_ex(s);
        return false;
    }

    if (FD_ISSET(s, &e)) {
        return false;
    }
    setsocketnonblocking_ex(s, false);

    return true;
}

bool SocketAPI::listen_ex(SOCKET s, int backlog)
{
    if (listen(s, backlog) == SOCKET_ERROR) {
        return false;
    }

    return true;
}

SOCKET SocketAPI::accept_ex(SOCKET s, struct sockaddr* addr, uint32_t* addr_len)
{
#ifndef OS_WIN32
    SOCKET client = accept(s, addr, (socklen_t*)addr_len);
#else
    SOCKET client = accept(s, addr, (int*)addr_len);
#endif
    return client;
}

bool SocketAPI::getsockopt_ex(SOCKET s, int level, int opt_name, void* opt_val, uint32_t* opt_len)
{
#ifndef OS_WIN32
    if (getsockopt(s, level, opt_name, opt_val, (socklen_t*)opt_len) == SOCKET_ERROR) {
        return false;
    }
#else
    if (getsockopt(s, level, opt_name, (char*)opt_val, (int*)opt_len) == SOCKET_ERROR) {
        return false;
    }
#endif

    return true;
}

bool SocketAPI::setsockopt_ex(SOCKET s, int level, int opt_name, const void* opt_val,
                              uint32_t opt_len)
{
#ifndef OS_WIN32
    if (setsockopt(s, level, opt_name, opt_val, opt_len) == SOCKET_ERROR) {
        return false;
    }
#else
    if (setsockopt(s, level, opt_name, (char*)opt_val, opt_len) == SOCKET_ERROR) {
        return false;
    }
#endif

    return true;
}

int SocketAPI::send_ex(SOCKET s, const void* buf, uint32_t len, int flags)
{
    int send_ret;

#ifndef OS_WIN32
    send_ret = (int)send(s, buf, len, flags);
#else
    send_ret = send(s, (const char*)buf, len, flags);
#endif
    return send_ret;
}


int SocketAPI::sendto_ex(SOCKET s, const void* buf, int len, int flags, const struct sockaddr* to,
                         int to_len)
{
#ifndef OS_WIN32
    int send_ret = (int)sendto(s, buf, len, flags, to, to_len);
#else
    int send_ret = sendto(s, (const char*)buf, len, flags, to, to_len);
#endif
    return send_ret;
}

int SocketAPI::recv_ex(SOCKET s, void* buf, uint32_t len, int flags)
{
#ifndef OS_WIN32
    int recv_ret = (int)recv(s, buf, len, flags);
#else
    int recv_ret = recv(s, (char*)buf, len, flags);
#endif
    return recv_ret;
}

int SocketAPI::recvfrom_ex(SOCKET s, void* buf, int len, int flags, struct sockaddr* from,
                           uint32_t* from_len)
{
#ifndef OS_WIN32
    int recv_ret = (int)recvfrom(s, buf, len, flags, from, (socklen_t*)from_len);

#else
    int recv_ret = recvfrom(s, (char*)buf, len, flags, from, (int*)from_len);
#endif
    return recv_ret;
}


bool SocketAPI::closesocket_ex(SOCKET s)
{
#ifndef OS_WIN32
    close(s);
#else
    if (closesocket(s) == SOCKET_ERROR) {
        return false;
    }
#endif

    return true;
}

bool SocketAPI::ioctlsocket_ex(SOCKET s, int64_t cmd, uint64_t* argp)
{
#ifndef OS_WIN32
#else
    if (ioctlsocket(s, cmd, (unsigned long*)argp) == SOCKET_ERROR) {
        return false;
    }
#endif

    return true;
}


bool SocketAPI::getsocketnonblocking_ex(SOCKET s)
{
#ifndef OS_WIN32
    int flags = FileAPI::fcntl_ex(s, F_GETFL, 0);

    return flags | O_NONBLOCK;
#else
    return false;
#endif

    return false;
}


bool SocketAPI::setsocketnonblocking_ex(SOCKET s, bool on)
{
#ifndef OS_WIN32
    int flags = FileAPI::fcntl_ex(s, F_GETFL, 0);

    if (on)
        // make nonblocking fd
        flags |= O_NONBLOCK;
    else
        // make blocking fd
        flags &= ~O_NONBLOCK;

    FileAPI::fcntl_ex(s, F_SETFL, flags);

    return true;
#else
    uint64_t argp = (on == true) ? 1 : 0;
    return ioctlsocket_ex(s, FIONBIO, &argp);
#endif
    return false;
}

uint32_t SocketAPI::availablesocket_ex(SOCKET s)
{
#ifndef OS_WIN32
    return FileAPI::availablefile_ex(s);
#else
    uint64_t argp = 0;
    ioctlsocket_ex(s, FIONREAD, &argp);
    return argp;
#endif
    return 0;
}

bool SocketAPI::set_tcp_no_delay(int fd)
{
    uint32_t tcp_nodelay_enable = 1;
    return setsockopt_ex(fd, IPPROTO_TCP, TCP_NODELAY, (void*)&tcp_nodelay_enable, sizeof(tcp_nodelay_enable));
}

bool SocketAPI::shutdown_ex(SOCKET s, int how)
{
    if (shutdown(s, how) < 0) {
        return false;
    }

    return true;
}

int SocketAPI::select_ex(int maxfdp1, fd_set* readset, fd_set* writeset, fd_set* exceptset,
                         struct timeval* timeout)
{
    int result = select(maxfdp1, readset, writeset, exceptset, timeout);
    return result;
}

void SocketAPI::set_send_buf(SOCKET s, int len)
{
    setsockopt_ex(s, SOL_SOCKET, SO_SNDBUF, &len, sizeof(len));
}

void SocketAPI::set_recv_buf(SOCKET s, int len)
{
    setsockopt_ex(s, SOL_SOCKET, SO_RCVBUF, &len, sizeof(len));
}

SOCKET SocketAPI::InitUDPListenSocket(const char* ip, uint32_t port, bool nonblock)
{
    SOCKET listen_sock = SocketAPI::socket_ex(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (listen_sock == INVALID_SOCKET) {
        return INVALID_SOCKET;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip);
    if (addr.sin_addr.s_addr == INADDR_NONE) {
        closesocket_ex(listen_sock);
        return INVALID_SOCKET;
    }
    addr.sin_port = htons(port);
    bool ret = SocketAPI::bind_ex(listen_sock, (struct sockaddr*)&addr, sizeof(addr));
    if (!ret) {
        SocketAPI::closesocket_ex(listen_sock);
        return INVALID_SOCKET;
    }

    int reuse_addr_ok = 1;
    SocketAPI::setsockopt_ex(listen_sock, SOL_SOCKET, SO_REUSEADDR, &reuse_addr_ok,
                             sizeof(reuse_addr_ok));
    SocketAPI::setsocketnonblocking_ex(listen_sock, nonblock);
    return listen_sock;
}

std::string SocketAPI::GetSockAddrStr(const struct sockaddr_in& addr)
{
    std::ostringstream oss;
    oss << "ip[" << int(addr.sin_addr.s_addr & 0xFF) << "."
        << int((addr.sin_addr.s_addr & 0xFF00) >> 8) << "."
        << int((addr.sin_addr.s_addr & 0xFF0000) >> 16) << "."
        << int((addr.sin_addr.s_addr & 0xFF000000) >> 24) << "] port[" << ntohs(addr.sin_port)
        << "]";
    return oss.str();
}

bool SocketAPI::IsIPV4Addr(const char* s)
{
    const char* p = nullptr;
    int tmp1, tmp2, tmp3, tmp4, i;

    i = sscanf(s, "%d.%d.%d.%d", &tmp1, &tmp2, &tmp3, &tmp4);

    if (i != 4) {
        return false;
    }

    if ((tmp1 > 255) || (tmp2 > 255) || (tmp3 > 255) || (tmp4 > 255)) {
        return false;
    }

    for (p = s; *p != 0; p++) {
        if ((*p != '.') && ((*p < '0') || (*p > '9'))) {
            return false;
        }
    }

    return true;
}

int SocketAPI::GetHostAddressV4(const std::string& ip, uint16_t port, struct sockaddr* addr,
                                int socketype)
{
    addrinfo hints;
    addrinfo* servinfo = nullptr;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = socketype;

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
