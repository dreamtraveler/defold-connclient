#pragma once

#include <cstdint>
#include <string>

namespace net
{
int SetNonBlock(int fd);
uint32_t IpToInt(const char* ip);
void SetSocketOpt(int fd);
int GetHostAddressV4(const std::string& ip, uint16_t port, struct sockaddr* addr, int socktype);
int SetTcpNoDelay(int fd, bool enable);
int SetTcpKeepAlive(int fd, bool enable);
int SetSendBuffer(int fd, int size);
int SetRecvBuffer(int fd, int size);
int SetReuseAddr(int fd);
int SetReusePort(int fd);
void ToIpV4(uint32_t ip, char ip_str[20]);
};  // namespace net