#include "kcp_session.h"

#define KCP_MTU 500

KcpSession::~KcpSession()
{
    Release();
}

int KcpSession::CreateKCP(const ControlKCPInfo* kcp_info, kcp_output output, void* user,
                          kcp_write_log log_fun, bool enable_kcp_log)
{
    kcp_conv_ = kcp_info->kcp_conv;
    kcp_ = pvp_ikcp_create(kcp_conv_, user);
    if (kcp_ == nullptr) return -1;

    kcp_->output = output;
    pvp_ikcp_nodelay(kcp_, kcp_info->nodelay, kcp_info->interval, kcp_info->resend, kcp_info->nc);
    pvp_ikcp_wndsize(kcp_, kcp_info->snd_wnd, kcp_info->rcv_wnd);
    if (kcp_info->dup_send_count > 0) {
        pvp_ikcp_setdupsend(kcp_, kcp_info->dup_send_count, kcp_info->dup_ack,
                            kcp_info->dup_dynamic, kcp_info->dup_wait, kcp_info->dup_wnd_on);
    }
    int mtu = (int)kcp_info->mtu;
    if (mtu > KCP_MTU) mtu = KCP_MTU;
    pvp_ikcp_setmtu(kcp_, mtu);

    kcp_->writelog = log_fun;
    if (kcp_->writelog && enable_kcp_log) {
        kcp_->logmask = 0x7FFFFFFF;
    }
    kcp_->rx_minrto = kcp_info->rx_minrto;
    kcp_->fastresend = kcp_info->fastresend;
    return 0;
}

int KcpSession::Input(const char* buf, int len, int64_t cur_time)
{
    if (kcp_ == nullptr) return 0;
    update_now_ = true;
    return pvp_ikcp_input(kcp_, buf, len, (uint32_t)cur_time);
}

int KcpSession::Send(const char* buf, int len, int64_t cur_time)
{
    if (kcp_ == nullptr) return 0;
    return pvp_ikcp_send_ex(kcp_, buf, len, (uint32_t)cur_time);
}

int KcpSession::Recv(char* buf, int len)
{
    if (kcp_ == nullptr) return 0;
    return pvp_ikcp_recv(kcp_, buf, len);
}

uint32_t KcpSession::Check(uint32_t current_ms)
{
    if (kcp_ == nullptr) return 0;
    return pvp_ikcp_check(kcp_, current_ms);
}

void KcpSession::Update(uint32_t current_ms)
{
    if (kcp_ == nullptr) return;
    pvp_ikcp_update(kcp_, current_ms);
    update_now_ = false;
}

void KcpSession::Release()
{
    if (kcp_ != nullptr) {
        pvp_ikcp_release(kcp_);
        kcp_ = nullptr;
    }
    kcp_conv_ = 0;
}

void KcpSession::Flush()
{
    if (kcp_ == nullptr) return;
    pvp_ikcp_flush(kcp_);
}

int KcpSession::PeekSize() const
{
    if (kcp_ == nullptr) return 0;
    return pvp_ikcp_peeksize(kcp_);
}

int KcpSession::WaitSnd() const
{
    if (kcp_ == nullptr) return 0;
    return pvp_ikcp_waitsnd(kcp_);
}

uint32_t KcpSession::NSndQue() const
{
    if (kcp_ == nullptr) return 0;
    return kcp_->nsnd_que;
}

uint32_t KcpSession::SndWnd() const
{
    if (kcp_ == nullptr) return 0;
    return kcp_->snd_wnd;
}

uint32_t KcpSession::SndUna() const
{
    if (kcp_ == nullptr) return 0;
    return kcp_->snd_una;
}

uint32_t KcpSession::SndNxt() const
{
    if (kcp_ == nullptr) return 0;
    return kcp_->snd_nxt;
}

uint32_t KcpSession::NSndBuf() const
{
    if (kcp_ == nullptr) return 0;
    return kcp_->nsnd_buf;
}

uint32_t KcpSession::NRcvQue() const
{
    if (kcp_ == nullptr) return 0;
    return kcp_->nrcv_que;
}

uint32_t KcpSession::RcvWnd() const
{
    if (kcp_ == nullptr) return 0;
    return kcp_->rcv_wnd;
}

uint32_t KcpSession::NRcvBuf() const
{
    if (kcp_ == nullptr) return 0;
    return kcp_->nrcv_buf;
}

uint32_t KcpSession::RcvNxt() const
{
    if (kcp_ == nullptr) return 0;
    return kcp_->rcv_nxt;
}

int KcpSession::RxSrtt() const
{
    if (kcp_ == nullptr) return 0;
    return kcp_->rx_srtt;
}

int32_t KcpSession::State() const
{
    if (kcp_ == nullptr) return 0;
    return (int)kcp_->state;
}

uint32_t KcpSession::Xmit() const
{
    if (kcp_ == nullptr) return 0;
    return kcp_->xmit;
}

uint32_t KcpSession::GetConv() const
{
    return kcp_conv_;
}

void KcpSession::Tick(uint32_t current_ms)
{
    if (kcp_ == nullptr) return;
    if (next_time_ms_ <= current_ms || update_now_) {
        Update(current_ms);
        next_time_ms_ = Check(current_ms);
    }
}
