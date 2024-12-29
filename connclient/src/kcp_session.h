#pragma once

#include <stdint.h>

#include "conn_protocol.h"
#include "ikcp.h"

using kcp_output = int (*)(const char*, int, ikcpcb*, void*);
using kcp_write_log = void (*)(const char* log, struct IKCPCB* kcp, void* user);

class KcpSession
{
public:
    KcpSession() = default;
    ~KcpSession();

public:
    // wrapper kcp
    int Input(const char* buf, int len, int64_t cur_time);
    int Send(const char* buf, int len, int64_t cur_time);
    int Recv(char* buf, int len);
    uint32_t Check(uint32_t current_ms);
    void Update(uint32_t current_ms);
    void Release();
    void Flush();
    int PeekSize() const;
    int WaitSnd() const;
    uint32_t NSndQue() const;
    uint32_t SndWnd() const;
    uint32_t SndUna() const;
    uint32_t SndNxt() const;
    uint32_t NSndBuf() const;
    uint32_t NRcvQue() const;
    uint32_t RcvWnd() const;
    uint32_t NRcvBuf() const;
    uint32_t RcvNxt() const;
    int RxSrtt() const;
    uint32_t Xmit() const;
    int32_t State() const;

public:
    int CreateKCP(const ControlKCPInfo* kcp_info, kcp_output output, void* user,
                  kcp_write_log log_fun, bool enable_kcp_log);
    uint32_t GetConv() const;
    bool IsNull() { return kcp_ == nullptr; }
    void Tick(uint32_t current_ms);

private:
    static void KcpWriteLog(const char* log, struct IKCPCB* kcp, void* user);

    IKCPCB* kcp_ = {nullptr};
    uint32_t kcp_conv_ = {0};
    uint32_t next_time_ms_ = {0};
    bool update_now_ = {false};
};
