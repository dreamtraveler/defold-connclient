#include "conn_client.h"

#include "base_macro.h"
#include "common_def.h"

#ifndef OS_WIN32
#include <sys/select.h>

#include "unistd.h"
#else
#include <WinSock2.h>
#endif

#include <cstdint>
#include <functional>
#include <thread>

#include "concurrentqueue.h"
#include "conn_protocol.h"
#include "kcp_session.h"
#include "socket_api.h"
#include "stream.h"
#include "sys_api.h"
#include "time_api.h"
#include "time_expire.h"


const int recv_one_time_size = 1024 * 16;
const int cs_conn_head_size = sizeof(CsConnHead);
const int cs_udp_conn_head_size = sizeof(CsUdpConnHead);
const int max_udp_pkg_len = 2048;
const int max_pkg_size = 3 * 1024 * 1024;

#define LOG_DEBUG(p)                                                                              \
    if (debug_log_mode_) {                                                                        \
        std::ostringstream oss;                                                                   \
        oss << "[DEBUG] [" << TimeAPI::GetCurTimeStr() << "][" << __FILENAME__ << ":" << __LINE__ \
            << "][" << __FUNCTION__ << "] " << p;                                                 \
        LogDebug(oss.str().c_str());                                                              \
    }

#define LOG_INFO(p)                                                                              \
    if (debug_log_mode_) {                                                                       \
        std::ostringstream oss;                                                                  \
        oss << "[INFO] [" << TimeAPI::GetCurTimeStr() << "][" << __FILENAME__ << ":" << __LINE__ \
            << "][" << __FUNCTION__ << "] " << p;                                                \
        LogInfo(oss.str().c_str());                                                              \
    }

#define LOG_ERROR(p)                                                                              \
    do {                                                                                          \
        std::ostringstream oss;                                                                   \
        oss << "[ERROR] [" << TimeAPI::GetCurTimeStr() << "][" << __FILENAME__ << ":" << __LINE__ \
            << "][" << __FUNCTION__ << "] " << p;                                                 \
        LogError(oss.str().c_str());                                                              \
    } while (0)

enum ConnState {
    CS_INIT,
    CS_CONNECTING,
    CS_CONNECTED,
    CS_LOGIC_CONNECTED,
};

class ConnClientPrivate
{
public:
    ConnClientPrivate();
    ~ConnClientPrivate();

public:
    void StartNetThreadLoop();
    void Update();

    int Connect(const char* ip, uint32_t port, int timeout_ms);
    int ConnectBlock(const char* ip, uint32_t port, int timeout_ms);
    void Close();
    int SendMsg(const char* msg_buf, int msg_len);
    int CreateConnect(int ai_socktype, int ai_family, int ai_protocol);
    bool IsConnected() const { return conn_state_ == CS_LOGIC_CONNECTED; }
    void SetConnState(int state);

    void SetUserData(void* user);
    void SetDebugLogMode();
    void SetErrorLogMode();
    void SetLogDebugCB(LuaCallback cb);
    void SetLogInfoCB(LuaCallback cb);
    void SetLogErrorCB(LuaCallback cb);
    void SetOutputCB(LuaCallback cb);
    void SetDisconnectCB(LuaCallback cb);
    void SetConnectSuccessCB(LuaCallback cb);
    void SetRelinkSuccessCB(LuaCallback cb);
    void SetRelinkCB(LuaCallback cb);
    void SetMagicNum(int magic) { magic_ = magic; }
    void AddRelinkInterval(int msec);
    void ClearRelinkInterval();
    void EnableKcpLog() { enable_kcp_log_ = true; }
    void SwitchNetwork();

    static void StaticKcpLogFun(const char* log, struct IKCPCB* kcp, void* user);
    void KcpLogFun(const char* log, struct IKCPCB* kcp);

private:
    int InnerConnect(const std::string& ip, uint32_t port, int timeout_ms);
    int SendTCPBuf(uint8_t cmd, const char* msg_buf = nullptr, int msg_len = 0);
    int SendKCPBuf(const char* msg_buf, int msg_len);
    int SendUDPBuf(uint8_t cmd, const char* msg_buf = nullptr, int msg_len = 0);
    void InnerClose(int reason);

private:
    void NetThreadLoop();
    void NotifyWorker();
    bool IsErrorable(int fd);
    bool IsReadable(int fd);
    bool IsWritable(int fd);
    void OnTcpRead(int64_t cur_time);
    void OnTcpWrite();
    void OnUdpRead(int64_t cur_time);
    void ReadStream(int64_t cur_time);
    void SendTcpPing(int64_t now_ms, bool immediate);
    void SendUdpPing(int64_t now_ms);
    void AddSocketToSelect(int fd, bool is_read, bool is_write);
    int HandleUDPRoutePing(int64_t cur_time, char* pkg);
    int UdpWrite(const char* pkg_buf, int len);
    int InputToKcp(const char* msg_buf, int msg_len, int64_t cur_time);
    void CreateKCP(const ControlKCPInfo* kcp_info);
    static int KCPOutput(const char* data, int len, ikcpcb* kcp, void* user);
    void CheckTimeout(int64_t now_ms);
    void CheckRelink(int64_t now_ms);

public:
    int flow() { return flow_; }

private:
    volatile bool running_ = {false};
    moodycamel::ConcurrentQueue<std::function<void()>> in_queue_;
    moodycamel::ConcurrentQueue<std::function<void()>> out_queue_;
    std::thread thread_;
    int thread_priority_ = {0};

    std::string ip_;
    uint16_t port_ = {0};
    int tcp_sock_ = {-1};
    int udp_sock_ = {-1};
    int tcp_writable_ = {false};
#ifndef OS_WIN32
    int pipe_sock_[2] = {-1, -1};
#endif
    int maxfd_ = {0};
    fd_set rset_;
    fd_set wset_;
    fd_set eset_;

    int conn_state_ = {CS_INIT};
    int64_t conn_state_ts_ = {0};
    Stream write_stream_;
    Stream read_stream_;
    int flow_ = {0};
    int magic_ = {0};

    bool debug_log_mode_ = {true};
    void* user_data_ = {nullptr};
    TimeExpire tcp_ping_expire_ = {2000};
    TimeExpire udp_ping_expire_ = {2000};
    int64_t ping_seq_ = {0};

    KcpSession kcp_session_;
    bool enable_udp_ = {false};
    bool enable_kcp_log_ = {false};

    bool is_first_connect_ = {true};
    std::vector<int> relink_interval_ms_vec_;
    int relink_count_ = {0};
    int connect_timeout_ms_ = {3000};

private:
    void LogDebug(const char* text);
    void LogInfo(const char* text);
    void LogError(const char* text);
    void Output(const char* data, int len, int64_t cur_time);
    void Disconnect();
    void ConnectSuccess();
    void ReConnectSuccess();
    void CallLuaCallback(void* user, LuaCallback callback, const char* data, int data_len,
                         const char* text, int text_len);

    LuaCallback log_debug_cb_ = {nullptr};
    LuaCallback log_info_cb_ = {nullptr};
    LuaCallback log_error_cb_ = {nullptr};
    LuaCallback output_cb_ = {nullptr};
    LuaCallback disconnect_cb_ = {nullptr};
    LuaCallback connect_success_cb_ = {nullptr};
    LuaCallback reconnect_success_cb_ = {nullptr};
    LuaCallback relink_cb_ = {nullptr};
};

ConnClientPrivate::ConnClientPrivate()
{
#ifndef OS_WIN32
    pipe(pipe_sock_);
    SocketAPI::setsocketnonblocking_ex(pipe_sock_[0], true);
    SocketAPI::setsocketnonblocking_ex(pipe_sock_[1], true);
#endif
    thread_priority_ = SysAPI::GetPriority();
    SetConnState(CS_INIT);

    SocketAPI::init_sock_env();
}

ConnClientPrivate::~ConnClientPrivate()
{
    running_ = false;
    NotifyWorker();
    if (thread_.joinable()) thread_.join();
    InnerClose(-1);
#ifndef OS_WIN32
    if (pipe_sock_[0] != -1) SocketAPI::closesocket_ex(pipe_sock_[0]);
    if (pipe_sock_[1] != -1) SocketAPI::closesocket_ex(pipe_sock_[1]);
    pipe_sock_[0] = -1;
    pipe_sock_[1] = -1;
#endif

    SocketAPI::free_sock_env();
}

void ConnClientPrivate::StartNetThreadLoop()
{
    if (!thread_.joinable()) {
        LOG_DEBUG("thread_.joinable=" << thread_.joinable());
        thread_ = std::thread(&ConnClientPrivate::NetThreadLoop, this);
    }
}

bool ConnClientPrivate::IsErrorable(int fd)
{
    return (fd != -1 && FD_ISSET(fd, &eset_));
}

bool ConnClientPrivate::IsReadable(int fd)
{
    return (fd != -1 && FD_ISSET(fd, &rset_));
}

bool ConnClientPrivate::IsWritable(int fd)
{
    return (fd != -1 && FD_ISSET(fd, &wset_));
}


void ConnClientPrivate::AddSocketToSelect(int fd, bool is_read, bool is_write)
{
    if (fd != INVALID_SOCKET) {
        if (is_read) FD_SET(fd, &rset_);
        if (is_write) FD_SET(fd, &wset_);
        if (fd > maxfd_) maxfd_ = fd;
    }
}

void ConnClientPrivate::NetThreadLoop()
{
    SysAPI::SetPriority(thread_priority_);
    running_ = true;
    relink_count_ = 0;
    const int timeout_ms = 10;
    while (running_) {
        struct timeval tv = {timeout_ms / 1000, (timeout_ms % 1000) * 1000};

        maxfd_ = 0;
        FD_ZERO(&rset_);
        FD_ZERO(&wset_);
        FD_ZERO(&eset_);

        AddSocketToSelect(tcp_sock_, true, tcp_writable_);
        AddSocketToSelect(udp_sock_, true, false);
#ifndef OS_WIN32
        const int pipe_read_sock = pipe_sock_[0];
        AddSocketToSelect(pipe_read_sock, true, false);
#endif

        if (maxfd_ > 0) {
            const int retval = select(maxfd_ + 1, &rset_, &wset_, &eset_, &tv);
            if (retval > 0) {
                const int64_t cur_time = TimeAPI::GetTimeMs();
                if (IsReadable(tcp_sock_) || IsErrorable(tcp_sock_)) {
                    OnTcpRead(cur_time);
                } else if (IsWritable(tcp_sock_)) {
                    LOG_DEBUG("Writable tcp_sock_=" << tcp_sock_);
                    tcp_writable_ = false;
                    if (conn_state_ == CS_CONNECTING) {
                        SetConnState(CS_CONNECTED);
                        if (!SocketAPI::set_tcp_no_delay(tcp_sock_)) {
                            const int err = SocketAPI::get_last_error();
                            LOG_ERROR("SetTcpNoDelay failed errno["<<err<<"] errstr[" << strerror(err) << "]");
                        }
                        LOG_INFO("tcp_sock connect Ok");
                    } else {
                        OnTcpWrite();
                    }
                }
                if (IsReadable(udp_sock_) || IsErrorable(udp_sock_)) {
                    OnUdpRead(cur_time);
                }

#ifndef OS_WIN32
                if (char c[8]; IsReadable(pipe_read_sock)) {
                    read(pipe_sock_[0], &c, 8);
                    LOG_DEBUG("Readable pipe");
                }
#endif
            } else if (retval == -1 && errno != EINTR) {
                LOG_ERROR("panic select: " << strerror(errno));
            }
        } else {
            TimeAPI::SleepMs(timeout_ms);
        }

        std::function<void()> fun;
        while (in_queue_.try_dequeue(fun)) {
            fun();
        }
        const int64_t now_ms = TimeAPI::GetTimeMs();
        kcp_session_.Tick((uint32_t)now_ms);
        SendTcpPing(now_ms, false);
        SendUdpPing(now_ms);
        CheckTimeout(now_ms);
        CheckRelink(now_ms);
    }
}

void ConnClientPrivate::Update()
{
    std::function<void()> fun;
    while (out_queue_.try_dequeue(fun)) {
        fun();
    }
}

int ConnClientPrivate::Connect(const char* ip, uint32_t port, int timeout_ms)
{
    LOG_DEBUG("Connect[" << ip << ":" << port << "] " << std::this_thread::get_id());
    ASSERT(std::this_thread::get_id() != thread_.get_id());
    std::string ip_str(ip);
    in_queue_.enqueue([this, ip_str = std::move(ip_str), port, timeout_ms]() {
        InnerConnect(ip_str, port, timeout_ms);
    });
    StartNetThreadLoop();
    NotifyWorker();
    return 0;
}
int ConnClientPrivate::InnerConnect(const std::string& ip, uint32_t port, int timeout_ms)
{
    LOG_DEBUG("InnerConnect[" << ip << ":" << port << "]" << std::this_thread::get_id());
    ip_ = ip;
    port_ = port;
    if (timeout_ms > 10000) timeout_ms = 10000;
    if (timeout_ms > 0) {
        connect_timeout_ms_ = timeout_ms;
    }

    tcp_sock_ = CreateConnect(SOCK_STREAM, AF_INET, IPPROTO_TCP);
    if (tcp_sock_ == -1) {
        LOG_ERROR("Create tcp_sock failed:" << strerror(errno));
        InnerClose(CLIENT_CONNECT_ERROR);
        return -1;
    }
    udp_sock_ = CreateConnect(SOCK_DGRAM, AF_INET, IPPROTO_UDP);
    if (udp_sock_ == -1) {
        LOG_ERROR("Create udp_sock failed:" << strerror(errno));
        InnerClose(CLIENT_CONNECT_ERROR);
        return -1;
    }
    LOG_DEBUG("tcp_sock=" << tcp_sock_ << ", upd_sock=" << udp_sock_);
    SetConnState(CS_CONNECTING);
    return 0;
}

int ConnClientPrivate::CreateConnect(int ai_socktype, int ai_family, int ai_protocol)
{
    struct sockaddr addr;
    memset(&addr, 0, sizeof(addr));
    if (SocketAPI::GetHostAddressV4(ip_, port_, &addr, ai_socktype) != 0) {
        LOG_ERROR("GetHostAddressV4 falied:" << strerror(errno));
        return -1;
    }
    const int sock = SocketAPI::socket_ex(ai_family, ai_socktype, ai_protocol);
    if (sock == -1) {
        LOG_ERROR("create socket_ex failed");
        return -1;
    }
    if (!SocketAPI::setsocketnonblocking_ex(sock, true)) {
        SocketAPI::closesocket_ex(sock);
        LOG_ERROR("setsocketnonblocking_ex failed");
        return -1;
    }

    const int ret = SocketAPI::connect_ex(sock, (struct sockaddr*)&addr, sizeof(addr));
    if (ret == -1) {
        if (errno == EINPROGRESS) {
            LOG_DEBUG("It is Ok in EINPROGRESS.");
        } else {
            SocketAPI::closesocket_ex(sock);
            LOG_ERROR("connect failed err: " << strerror(errno));
            return -1;
        }
    }


#ifndef OS_WIN32
    // TOS
    if (ai_socktype == SOCK_DGRAM) {
        uint32_t qos_ef = 0xB8;
        SocketAPI::setsockopt_ex(sock, IPPROTO_IP, IP_TOS, (void*)&qos_ef, sizeof(qos_ef));
    }
#endif
    return sock;
}

int ConnClientPrivate::ConnectBlock(const char* ip, uint32_t port, int timeout_ms)
{
    ASSERT(std::this_thread::get_id() != thread_.get_id());
    std::string ip_str(ip);
    in_queue_.enqueue([this, ip_str = std::move(ip_str), port, timeout_ms]() {
        InnerConnect(ip_str, port, timeout_ms);
    });
    StartNetThreadLoop();
    NotifyWorker();

    int loop_count = timeout_ms / 10;
    while (loop_count-- > 0) {
        Update();
        if (conn_state_ == CS_LOGIC_CONNECTED) return 0;
        TimeAPI::SleepMs(10);
    }
    return -1;
}

void ConnClientPrivate::Close()
{
    ASSERT(std::this_thread::get_id() != thread_.get_id());
    in_queue_.enqueue([this]() { InnerClose(-1); });
}
void ConnClientPrivate::InnerClose(int reason)
{
    if (reason < CLIENT_CONNECT_ERROR) {
        LOG_DEBUG("Release Kcp");
        kcp_session_.Flush();
        kcp_session_.Release();
        running_ = false;
    }
    if (tcp_sock_ != -1) {
        SocketAPI::closesocket_ex(tcp_sock_);
        tcp_sock_ = -1;
    }
    if (udp_sock_ != -1) {
        SocketAPI::closesocket_ex(udp_sock_);
        udp_sock_ = -1;
    }
    SetConnState(CS_INIT);
    write_stream_.Reset();
    read_stream_.Reset();

    if (reason >= 0 && disconnect_cb_ != nullptr) {
        if (disconnect_cb_) {
            out_queue_.enqueue([this, reason]() {
                CallLuaCallback(user_data_, disconnect_cb_, nullptr, 0, nullptr, reason);
            });
        }
    }
}

int ConnClientPrivate::SendMsg(const char* msg_buf, int msg_len)
{
    if (conn_state_ < CS_LOGIC_CONNECTED) return -1;
    std::string str(msg_buf, msg_len);
    in_queue_.enqueue([this, str = std::move(str)]() { SendKCPBuf(str.c_str(), (int)str.size()); });
    return 0;
}

int ConnClientPrivate::SendKCPBuf(const char* msg_buf, int msg_len)
{
    if (msg_len <= 0) return 0;
    if (conn_state_ < CS_LOGIC_CONNECTED) return -1;
    const int ret = kcp_session_.Send(msg_buf, msg_len, TimeAPI::GetTimeMs());
    if (ret != 0) {
        LOG_ERROR("kcp_session_.Send ret[" << ret << "]");
        return -1;
    }
    return 0;
}

int ConnClientPrivate::SendTCPBuf(uint8_t cmd, const char* msg_buf, int msg_len)
{
    if (msg_len < 0) return -1;
    const int total_len = cs_conn_head_size + msg_len;
    if (write_stream_.EnsureWritable(total_len) != 0) {
        LOG_ERROR("flow[" << flow_ << "] EnsureWritable failed total_len=" << total_len);
        InnerClose(CLIENT_CONNECT_ERROR);
        return -1;
    }
    auto* head = (CsConnHead*)write_stream_.End();
    head->sec_pkg_len = htonl(total_len);
    head->flow = flow_;
    head->magic = magic_;
    head->cmd = cmd;
    if (msg_buf != nullptr && msg_len > 0) {
        memcpy(write_stream_.End() + cs_conn_head_size, msg_buf, msg_len);
    }
    write_stream_.AddSize(total_len);
    OnTcpWrite();
    return 0;
}

int ConnClientPrivate::SendUDPBuf(uint8_t cmd, const char* msg_buf, int msg_len)
{
    if (udp_sock_ == INVALID_SOCKET) return -1;

    if (msg_len < 0 || msg_len > max_udp_pkg_len - cs_udp_conn_head_size) {
        LOG_ERROR("SendUDPBuf msg_len[" << msg_len << "] illegal");
        return -1;
    }
    const int len = cs_udp_conn_head_size + msg_len;
    static char pkg_buf[max_udp_pkg_len];
    auto* head = (CsUdpConnHead*)pkg_buf;
    head->flow = flow_;
    head->magic = magic_;
    head->cmd = cmd;
    if (msg_buf != nullptr && msg_len > 0) {
        memcpy((char*)pkg_buf + cs_udp_conn_head_size, msg_buf, msg_len);
    }
    return UdpWrite(pkg_buf, len);
}

int ConnClientPrivate::UdpWrite(const char* pkg_buf, int len)
{
    const int send_len = SocketAPI::send_ex(udp_sock_, pkg_buf, len, 0);
    if (send_len <= 0) {
        const int err = SocketAPI::get_last_error();
        if (err != EWOULDBLOCK && err != EINTR) {
            LOG_ERROR("udp send errno[" << err << "] errstr[" << strerror(err) << "] send_len["
                                        << send_len << "] pkg_len[" << len << "]");
            InnerClose(CLIENT_CONNECT_ERROR);
            return -1;
        }
    }
    if (send_len != len) {
        LOG_ERROR("udp send busy, disconnect! send_len = " << send_len << ", pkg_len = " << len);
        InnerClose(CLIENT_CONNECT_ERROR);
        return -1;
    }
    return send_len;
}

void ConnClientPrivate::NotifyWorker()
{
#ifndef OS_WIN32
    const int wfd = pipe_sock_[1];
    if (wfd > 0) {
        write(wfd, "z", 1);
    }
#endif
}

void ConnClientPrivate::CallLuaCallback(void* user, LuaCallback callback, const char* data,
                                        int data_len, const char* text, int text_len)
{
    if (!dmScript::IsCallbackValid(callback)) return;

    lua_State* L = dmScript::GetCallbackLuaContext(callback);
    DM_LUA_STACK_CHECK(L, 0)

    if (!dmScript::SetupCallback(callback)) {
        dmLogError("Failed to setup callback");
        return;
    }

    lua_pushlightuserdata(L, user);
    int nargs = 1;
    if (data != nullptr && data_len > 0) {
        lua_pushlstring(L, data, data_len);
        nargs++;
    } else if (text != nullptr && text_len > 0) {
        lua_pushlstring(L, text, text_len);
        nargs++;
    } else if (text_len > 0) {
        lua_pushnumber(L, text_len);
        nargs++;
    }

    dmScript::PCall(L, nargs, 0);
    dmScript::TeardownCallback(callback);
}

void ConnClientPrivate::ConnectSuccess()
{
    SetConnState(CS_LOGIC_CONNECTED);
    const int64_t now_ms = TimeAPI::GetTimeMs();
    tcp_ping_expire_.Reset(now_ms);
    SendTcpPing(now_ms, true);
    if (connect_success_cb_ != nullptr) {
        out_queue_.enqueue([this]() {
            if (connect_success_cb_ != nullptr) {
                CallLuaCallback(user_data_, connect_success_cb_, nullptr, 0, nullptr, 0);
            }
        });
    }
}

void ConnClientPrivate::ReConnectSuccess()
{
    SetConnState(CS_LOGIC_CONNECTED);
    const int64_t now_ms = TimeAPI::GetTimeMs();
    tcp_ping_expire_.Reset(now_ms);
    SendTcpPing(now_ms, true);
    if (reconnect_success_cb_ != nullptr) {
        out_queue_.enqueue([this]() {
            if (reconnect_success_cb_ != nullptr) {
                CallLuaCallback(user_data_, reconnect_success_cb_, nullptr, 0, nullptr, 0);
            }
        });
    }
}


void ConnClientPrivate::Output(const char* data, int len, int64_t cur_time)
{
    if (output_cb_ != nullptr) {
        std::string data_str(data, len);
        out_queue_.enqueue([this, data_str = std::move(data_str)]() {
            if (output_cb_ != nullptr) {
                CallLuaCallback(user_data_, output_cb_, data_str.c_str(), data_str.size(), nullptr,
                                0);
            }
        });
    }
}

void ConnClientPrivate::OnTcpRead(int64_t cur_time)
{
    if (read_stream_.EnsureWritable(recv_one_time_size) != 0) {
        InnerClose(CLIENT_CONNECT_ERROR);
    }
    int nread = SocketAPI::recv_ex(tcp_sock_, read_stream_.End(), recv_one_time_size, 0);
    if (nread > 0) {
        LOG_DEBUG("recv nread=" << nread);
        read_stream_.AddSize(nread);
        ReadStream(cur_time);
    } else {
        LOG_ERROR("close tcp_sock nread["<<nread<<"] " << tcp_sock_ << " :" << strerror(errno));
        InnerClose(CLIENT_CONNECT_ERROR);
    }
}

void ConnClientPrivate::OnUdpRead(int64_t cur_time)
{
    if (udp_sock_ == INVALID_SOCKET) return;

    static char pkg_buf[max_udp_pkg_len];
    struct sockaddr server_addr;
    uint32_t server_addr_len = sizeof(server_addr);
    int pkg_len = SocketAPI::recvfrom_ex(udp_sock_, pkg_buf, max_udp_pkg_len, 0, &server_addr,
                                         &server_addr_len);
    if (pkg_len >= cs_udp_conn_head_size) {
        auto* head = (CsUdpConnHead*)pkg_buf;
        const int flow = head->flow;
        if (pkg_len == sizeof(int) + sizeof(int64_t) && flow == 0) {
            HandleUDPRoutePing(cur_time, pkg_buf);
        } else {
            if (flow != flow_) {
                LOG_ERROR("proto flow[" << flow << "] != flow[" << flow_ << "]");
                return;
            }
            char* msg_buf = (char*)pkg_buf + cs_udp_conn_head_size;
            const int msg_len = pkg_len - cs_udp_conn_head_size;

            if (head->cmd == CONTROL_UNRELIABLE_MSG) {
                Output(msg_buf, msg_len, cur_time);
            } else if (head->cmd == CONTROL_RELIABLE_MSG) {
                InputToKcp(msg_buf, msg_len, cur_time);
            } else if (head->cmd == CONTROL_DISCONNECT) {
                InnerClose(CONTROL_SERVER_CLOSE);
            }
        }
    } else if (pkg_len == -1) {
        const int err = SocketAPI::get_last_error();
        if (err != EWOULDBLOCK && err != EINTR) {
            LOG_ERROR("udp sock[" << udp_sock_ << "] errno[" << err << "] errstr[" << strerror(err)
                                  << "]");
            InnerClose(CLIENT_CONNECT_ERROR);
            return;
        }
    }
}

int ConnClientPrivate::HandleUDPRoutePing(int64_t cur_time, char* pkg)
{
    // ping时延处理
    char* p = pkg + sizeof(int);
    const int64_t ack_time_ms = *(int64_t*)p;
    const int64_t rtt = cur_time - ack_time_ms;
    LOG_DEBUG("CONTROL_PING:recv udp internet ping rtt[" << rtt << "]");
    return 0;
}

void ConnClientPrivate::ReadStream(int64_t cur_time)
{
    while (true) {
        const int stream_len = read_stream_.Len();
        if (stream_len < cs_conn_head_size) return;
        auto* head = (CsConnHead*)read_stream_.Buf();
        flow_ = head->flow;
        magic_ = head->magic;
        const int pkg_len = ntohl(head->sec_pkg_len);
        if (stream_len < pkg_len) return;

        const char* data = read_stream_.Buf() + cs_conn_head_size;
        const int data_len = pkg_len - cs_conn_head_size;

        if (head->cmd == CONTROL_PING) {
            if (data_len != (int)sizeof(CsPing)) {
                LOG_ERROR("ping cmd error");
                InnerClose(CLIENT_CONNECT_ERROR);
                return;
            }
            auto* ping = (CsPing*)(data);
            if (ping->time != ping_seq_) {
                LOG_ERROR("ping lost seq=" << ping_seq_);
            }
            const int64_t ping_rtt = cur_time - ping->time;
            LOG_DEBUG("PING seq=" << ping_seq_ << " ping_rtt=" << ping_rtt);

            read_stream_.Skip(pkg_len);
            continue;
        } else if (head->cmd == CONTROL_RELIABLE_MSG) {
            InputToKcp(data, data_len, cur_time);
        } else if (head->cmd == CONTROL_UNRELIABLE_MSG) {
            Output(data, data_len, cur_time);
        } else if (head->cmd == CONTROL_KCP_INFO) {
            if (data_len >= (int)sizeof(ControlKCPInfo) && kcp_session_.IsNull()) {
                const ControlKCPInfo* kcp_info = (ControlKCPInfo*)data;
                LOG_DEBUG("CONTROL_KCP_INFO");
                CreateKCP(kcp_info);
            } else {
                LOG_ERROR("KCP_INFO length error");
                InnerClose(CLIENT_CONNECT_ERROR);
                return;
            }
        } else if (head->cmd == CONTROL_DISCONNECT) {
            if (data_len >= (int)sizeof(int)) {
                const int control_disconnect_reason = *(int*)data;
                if (control_disconnect_reason == CONTROL_SERVER_CLOSE) {
                    LOG_DEBUG("FINI:SERVER_CLOSE");
                    InnerClose(CONTROL_SERVER_CLOSE);
                    return;
                }
                if (control_disconnect_reason == CONTROL_SERVER_FULL) {
                    LOG_ERROR("FINI:SERVER_FULL");
                    InnerClose(CONTROL_SERVER_FULL);
                    return;
                }
                if (control_disconnect_reason == CONTROL_FLOW_NOT_EXIST) {
                    LOG_DEBUG("FINI:FLOW_NOT_EXIST");
                    InnerClose(CONTROL_FLOW_NOT_EXIST);
                    return;
                }
            } else {
                LOG_ERROR("FINI:UNKNOW_REASON, data_len=" << data_len);
                InnerClose(CONTROL_SERVER_CLOSE);
            }
        } else if (head->cmd == CONTROL_SYNC_LABEL) {
            if (is_first_connect_) {
                is_first_connect_ = false;
                ConnectSuccess();
            } else {
                ReConnectSuccess();
            }
        }

        read_stream_.Skip(pkg_len);
        LOG_DEBUG("CsConnHead pkg_len=" << pkg_len << ", flow=" << head->flow << ", magic="
                                        << (int)head->magic << ", cmd=" << (int)head->cmd);
    }
}

void ConnClientPrivate::OnTcpWrite()
{
    if (write_stream_.Len() <= 0) return;
    const int need_send_len = write_stream_.Len();
    const int nwritten = SocketAPI::send_ex(tcp_sock_, write_stream_.Buf(), write_stream_.Len(), 0);
    if (nwritten > 0) {
        write_stream_.Skip(nwritten);
    } else {
        const int eno = errno;
        if (eno == EWOULDBLOCK || eno == EAGAIN) {
            LOG_DEBUG("Try again later fd[" << tcp_sock_ << "]");
        } else if (eno == EINTR) {
            LOG_DEBUG("send encounter EINTR fd[" << tcp_sock_ << "]");
        } else {
            LOG_ERROR("send errno[" << eno << "]:" << strerror(eno));
            InnerClose(CLIENT_CONNECT_ERROR);
            return;
        }
    }
    if (nwritten < need_send_len) {
        tcp_writable_ = true;
    }
}

void ConnClientPrivate::SendTcpPing(int64_t now_ms, bool immediate)
{
    if (conn_state_ >= CS_CONNECTED && (immediate || tcp_ping_expire_.TryExpire(now_ms))) {
        CsPing ping;
        ping.time = now_ms;
        ping_seq_ = ping.time;
        SendTCPBuf(CONTROL_PING, (char*)&ping, (int)sizeof(ping));
    }
}

void ConnClientPrivate::SendUdpPing(int64_t now_ms)
{
    if (udp_sock_ == INVALID_SOCKET) return;
    if (conn_state_ >= CS_CONNECTED && udp_ping_expire_.TryExpire(now_ms)) {
        const size_t len = sizeof(int) + sizeof(int64_t);
        char buf[len];
        *(int*)buf = 0;
        *(int64_t*)(buf + sizeof(int)) = now_ms;
        UdpWrite(buf, len);
    }
}

void ConnClientPrivate::SetConnState(int state)
{
    conn_state_ = state;
    conn_state_ts_ = TimeAPI::GetTimeMs();
    if (conn_state_ == CS_INIT) {
        tcp_writable_ = true;
    } else if (conn_state_ == CS_LOGIC_CONNECTED) {
        relink_count_ = 0;
    }
}

void ConnClientPrivate::CheckTimeout(int64_t now_ms)
{
    if (conn_state_ > CS_INIT && conn_state_ < CS_LOGIC_CONNECTED &&
        conn_state_ts_ + connect_timeout_ms_ < now_ms) {
        if (relink_count_ < (int)relink_interval_ms_vec_.size()) {
            InnerClose(CLIENT_CONNECT_TIMEOUT_CONTINUE);
        } else {
            InnerClose(CLIENT_CONNECT_TIMEOUT);
        }
    }
}

void ConnClientPrivate::CheckRelink(int64_t now_ms)
{
    if (!running_) return;
    if (relink_count_ < 0) relink_count_ = 0;
    if (relink_count_ >= relink_interval_ms_vec_.size()) return;
    const int64_t interval = relink_interval_ms_vec_[relink_count_];

    if (conn_state_ == CS_INIT && conn_state_ts_ + interval < now_ms) {
        relink_count_++;
        InnerConnect(ip_, port_, 0);
        if (relink_cb_ != nullptr) {
            out_queue_.enqueue([this]() {
                if (relink_cb_ != nullptr) {
                    CallLuaCallback(user_data_, relink_cb_, nullptr, 0, nullptr, relink_count_);
                }
            });
        }
    }
}

int ConnClientPrivate::InputToKcp(const char* msg_buf, int msg_len, int64_t cur_time)
{
    if (!kcp_session_.IsNull()) {
        const int ret = kcp_session_.Input(msg_buf, msg_len, cur_time);
        if (ret != 0) {
            LOG_ERROR("pvp_ikcp_input ERROR ret = " << ret << ", flow = " << flow_);
            InnerClose(CLIENT_CONNECT_ERROR);
            return -1;
        }
        static char pkg[max_pkg_size];
        int recv_len = 0;
        while_s(true)
        {
            recv_len = kcp_session_.Recv(pkg, max_pkg_size);
            if (recv_len <= 0) break;
            // 第一个byte为控制byte
            const char cmd = pkg[0];
            if (cmd == CONTROL_DISCONNECT) {
                LOG_DEBUG("FINI:SERVER_CLOSE");
                InnerClose(CONTROL_SERVER_CLOSE);
            } else if (recv_len > 1) {
                Output(pkg + 1, recv_len - 1, cur_time);
            }
        }
        if (recv_len == -3) {
            LOG_ERROR("flow[" << flow_ << "] kcp_session Recv failed pkg_len > MAX_PKG_SIZE");
            InnerClose(CLIENT_CONNECT_ERROR);
        }
    } else {
        InnerClose(CLIENT_CONNECT_ERROR);
        return -1;
    }
    return 0;
}

void ConnClientPrivate::StaticKcpLogFun(const char* log, struct IKCPCB* kcp, void* user)
{
    auto* client = (ConnClientPrivate*)user;
    if (client == nullptr) return;
    client->KcpLogFun(log, kcp);
}
void ConnClientPrivate::KcpLogFun(const char* log, struct IKCPCB* kcp)
{
    LOG_DEBUG("[KCP]:" << log);
}

void ConnClientPrivate::CreateKCP(const ControlKCPInfo* kcp_info)
{
    LOG_DEBUG("CreateKCP");
    if (kcp_session_.CreateKCP(kcp_info, ConnClientPrivate::KCPOutput, (void*)this, StaticKcpLogFun,
                               enable_kcp_log_) != 0) {
        LOG_ERROR("CreateKCP Failed");
        return;
    }
    enable_udp_ = kcp_info->enable_udp > 0;

    kcp_session_.Update((uint32_t)TimeAPI::GetTimeMs());
    LOG_DEBUG("CreateKCP success! conv = " << kcp_info->kcp_conv);
}

int ConnClientPrivate::KCPOutput(const char* data, int len, ikcpcb* kcp, void* user)
{
    auto* client = (ConnClientPrivate*)user;
    if (client == nullptr) return -1;

    if (client->enable_udp_) {
        return client->SendUDPBuf(CONTROL_RELIABLE_MSG, data, len);
    } else {
        return client->SendTCPBuf(CONTROL_RELIABLE_MSG, data, len);
    }
}

void ConnClientPrivate::LogDebug(const char* text)
{
    if (log_debug_cb_ != nullptr) {
        std::string str(text);
        out_queue_.enqueue([this, str = std::move(str)]() {
            if (log_debug_cb_ != nullptr) {
                CallLuaCallback(user_data_, log_debug_cb_, nullptr, 0, str.c_str(), str.size());
            }
        });
    } else {
        std::cout << text << std::endl;
    }
}
void ConnClientPrivate::LogInfo(const char* text)
{
    if (log_info_cb_ != nullptr) {
        std::string str(text);
        out_queue_.enqueue([this, str = std::move(str)]() {
            if (log_info_cb_ != nullptr) {
                CallLuaCallback(user_data_, log_info_cb_, nullptr, 0, str.c_str(), str.size());
            }
        });
    } else {
        std::cerr << text << std::endl;
    }
}
void ConnClientPrivate::LogError(const char* text)
{
    if (log_error_cb_ != nullptr) {
        std::string str(text);
        out_queue_.enqueue([this, str = std::move(str)]() {
            if (log_error_cb_ != nullptr) {
                CallLuaCallback(user_data_, log_error_cb_, nullptr, 0, str.c_str(), str.size());
            }
        });
    } else {
        std::cerr << text << std::endl;
    }
}

void ConnClientPrivate::SetUserData(void* user)
{
    user_data_ = user;
}
void ConnClientPrivate::SetDebugLogMode()
{
    debug_log_mode_ = true;
}
void ConnClientPrivate::SetErrorLogMode()
{
    debug_log_mode_ = false;
}
void ConnClientPrivate::SetLogDebugCB(LuaCallback cb)
{
    log_debug_cb_ = cb;
}
void ConnClientPrivate::SetLogInfoCB(LuaCallback cb)
{
    log_info_cb_ = cb;
}
void ConnClientPrivate::SetLogErrorCB(LuaCallback cb)
{
    log_error_cb_ = cb;
}
void ConnClientPrivate::SetOutputCB(LuaCallback cb)
{
    output_cb_ = cb;
}
void ConnClientPrivate::SetDisconnectCB(LuaCallback cb)
{
    disconnect_cb_ = cb;
}
void ConnClientPrivate::SetConnectSuccessCB(LuaCallback cb)
{
    connect_success_cb_ = cb;
}
void ConnClientPrivate::SetRelinkSuccessCB(LuaCallback cb)
{
    reconnect_success_cb_ = cb;
}
void ConnClientPrivate::SetRelinkCB(LuaCallback cb)
{
    relink_cb_ = cb;
}
void ConnClientPrivate::AddRelinkInterval(int msec)
{
    if (relink_interval_ms_vec_.empty()) {
        relink_interval_ms_vec_.push_back(0);
    }
    if (relink_interval_ms_vec_.size() < 5) {
        relink_interval_ms_vec_.push_back(msec);
        std::sort(relink_interval_ms_vec_.begin(), relink_interval_ms_vec_.end());
    }
}
void ConnClientPrivate::ClearRelinkInterval()
{
    relink_interval_ms_vec_.clear();
}
void ConnClientPrivate::SwitchNetwork()
{
    if (conn_state_ == CS_LOGIC_CONNECTED && relink_count_ == 0) {
        // 避免网络库在感知断网的时候，外部也同时感知断网传入进来
        LOG_DEBUG("SwitchNetwork");
        InnerClose(CLIENT_CONNECT_SWITCH);
    }
}

ConnClient::ConnClient() : m(new ConnClientPrivate())
{
    m->SetUserData((void*)this);
}
ConnClient::~ConnClient()
{
    if (m != nullptr) {
        delete m;
        m = nullptr;
    }
}

void ConnClient::Update()
{
    m->Update();
}
int ConnClient::Connect(const char* ip, uint32_t port, int timeout_ms)
{
    return m->Connect(ip, port, timeout_ms);
}
int ConnClient::ConnectBlock(const char* ip, uint32_t port, int timeout_ms)
{
    return m->ConnectBlock(ip, port, timeout_ms);
}
void ConnClient::Close()
{
    m->Close();
}
int ConnClient::SendMsg(const char* msg_buf, int msg_len)
{
    return m->SendMsg(msg_buf, msg_len);
}
bool ConnClient::IsConnected() const
{
    return m->IsConnected();
}

void ConnClient::SetUserData(void* user)
{
    m->SetUserData(user);
}
void ConnClient::SetDebugLogMode()
{
    m->SetDebugLogMode();
}
void ConnClient::SetErrorLogMode()
{
    m->SetErrorLogMode();
}
void ConnClient::SetLogDebugCB(LuaCallback cb)
{
    m->SetLogDebugCB(cb);
}
void ConnClient::SetLogInfoCB(LuaCallback cb)
{
    m->SetLogInfoCB(cb);
}
void ConnClient::SetLogErrorCB(LuaCallback cb)
{
    m->SetLogErrorCB(cb);
}
void ConnClient::SetOutputCB(LuaCallback cb)
{
    m->SetOutputCB(cb);
}
void ConnClient::SetDisconnectCB(LuaCallback cb)
{
    m->SetDisconnectCB(cb);
}
void ConnClient::SetConnectSuccessCB(LuaCallback cb)
{
    m->SetConnectSuccessCB(cb);
}
void ConnClient::SetRelinkSuccessCB(LuaCallback cb)
{
    m->SetRelinkSuccessCB(cb);
}
void ConnClient::SetRelinkCB(LuaCallback cb)
{
    m->SetRelinkCB(cb);
};
void ConnClient::SetMagicNum(int magic)
{
    m->SetMagicNum(magic);
}
void ConnClient::AddRelinkInterval(int msec)
{
    m->AddRelinkInterval(msec);
}
void ConnClient::ClearRelinkInterval()
{
    m->ClearRelinkInterval();
}
void ConnClient::EnableKcpLog()
{
    m->EnableKcpLog();
}
void ConnClient::SwitchNetwork()
{
    m->SwitchNetwork();
}
