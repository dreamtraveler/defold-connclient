#pragma once

#include <cstdint>
#include <dmsdk/script.h>


using LuaCallback = dmScript::LuaCallbackInfo*;

class ConnClientPrivate;
class ConnClient
{
public:
    ConnClient();
    ~ConnClient();

public:
    void Update();
    int Connect(const char* ip, uint32_t port, int timeout_ms);
    int ConnectBlock(const char* ip, uint32_t port, int timeout_ms);
    void Close();
    int SendMsg(const char* msg_buf, int msg_len);
    bool IsConnected() const;

public:
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
    void SetRelinkCB(LuaCallback);
    void SetMagicNum(int magic);
    void AddRelinkInterval(int msec);
    void ClearRelinkInterval();
    void EnableKcpLog();
    void SwitchNetwork();

private:
    ConnClientPrivate* m = {nullptr};
};
