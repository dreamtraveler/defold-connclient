#pragma once
#include <cstdint>

// TCP pkg
// | pkg_len | flow    | magic |   cmd  | buf     |
// |---------|---------|-------|--------|---------|
// | 4 Bytes | 4 Bytes |1 Bytes| 1 Bytes| N Bytes |
struct CsConnHead {
    int sec_pkg_len;
    int flow;
    uint8_t magic;
    uint8_t cmd;
} __attribute__((__packed__));
static_assert(sizeof(CsConnHead) == 10, "unexpected layout");

struct CsUdpConnHead {
    int flow;
    uint8_t magic;
    uint8_t cmd;
} __attribute__((__packed__));
static_assert(sizeof(CsUdpConnHead) == 6, "unexpected layout");

struct SsConnHead {
    int flow = {0};
    uint8_t trans_type = {0};
    uint8_t conn_cmd = {0};
    uint64_t custom_id = {0};
    uint32_t client_ip = {0};
} __attribute__((__packed__));
static_assert(sizeof(SsConnHead) == 18, "unexpected layout");

struct CsPing {
    int64_t time;
    /* data */
} __attribute__((__packed__));
static_assert(sizeof(CsPing) == 8, "unexpected layout");

enum CsConnCmd {
    CONTROL_RELIABLE_MSG = 0,    // 可靠消息
    CONTROL_UNRELIABLE_MSG = 1,  // 不可靠消息
    CONTROL_PING = 2,            // ping
    CONTROL_KCP_INFO = 3,        // ControlKCPInfo
    CONTROL_DISCONNECT = 4,      // 服务器断开
    CONTROL_BI = 5,              // 记录BI事件
    CONTROL_LOG = 6,             // 客户端日志
    CONTROL_SYNC_LABEL = 7,      // 传输标签给客户端
    CONTROL_QUEUE = 8,           // 排队信息
};

enum ControlDisconnectReason {
    CONTROL_SERVER_CLOSE = 0,                // 服务器断开
    CONTROL_SERVER_FULL = 1,                 // 服务器断开原因，服务器已满
    CONTROL_FLOW_NOT_EXIST = 2,              // 服务器断开原因，FLOW已经不存在了
    CONTROL_OVERPASS_LIMIT_PKG_PER_SEC = 3,  // 服务器断开原因，超过包量限制
    CLIENT_CONNECT_TIMEOUT = 4,              // 连接超时
    CLIENT_CONNECT_ERROR = 5,                // 客户端错误
    CLIENT_CONNECT_TIMEOUT_CONTINUE = 6,     // 连接超时且会继续尝试
    CLIENT_CONNECT_SWITCH = 7,               // 网络切换
};

enum SsConnCmd {
    CONN_START = 21,
    CONN_PROC = 22,
    CONN_STOP = 23,
    CONN_RECONN = 24,
};

// ConnSvr
enum PkgTransType {
    PKG_TRANS_KCP = 1,
    PKG_TRANS_TCP = 2,
    PKG_TRANS_UDP = 3,
};

struct ControlKCPInfo {
    int dup_send_count;
    uint32_t kcp_conv;
    int nodelay;
    int interval;
    int resend;
    int nc;
    uint32_t mtu;
    int rx_minrto;
    int fastresend;
    uint32_t snd_wnd;
    uint32_t rcv_wnd;
    uint32_t rmt_wnd;
    int enable_udp;
    int dup_ack;
    int dup_dynamic;
    uint32_t dup_wait;
    uint32_t dup_wnd_on;
    bool enable_simulator;
    int rtt_min;
    int rtt_max;
    int lost_rate_low;
    int lost_rate_high;
    int low_lost_period;
    int high_lost_period;
} __attribute__((__packed__));
static_assert(sizeof(ControlKCPInfo) == 93, "unexpected layout");
