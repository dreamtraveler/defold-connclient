// myextension.cpp
// Extension lib defines
#define LIB_NAME "connclient"
#define MODULE_NAME "connclient"

// include the Defold SDK
#include <dmsdk/sdk.h>

#include <vector>

#include "conn_client.h"

std::vector<ConnClient*> g_connclients;

static bool IsConnectionValid(ConnClient* conn)
{
    if (conn) {
        for (int i = 0; i < g_connclients.size(); ++i) {
            if (g_connclients[i] == conn) {
                return true;
            }
        }
    }
    return false;
}

static void RemoveConnection(ConnClient* conn)
{
    if (conn) {
        for (auto it = g_connclients.begin(); it != g_connclients.end(); ++it) {
            if (*it == conn) {
                g_connclients.erase(it);
                break;
            }
        }
    }
}

static int lua_connclient_connect(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);

    const char* ip = luaL_checkstring(L, 1);
    int port = luaL_checkinteger(L, 2);
    int timeout_ms = luaL_checkinteger(L, 3);

    ConnClient* conn = new ConnClient();
    int ret = conn->Connect(ip, port, timeout_ms);
    if (ret != 0) {
        delete conn;
        return luaL_error(L, "client->Connect failed");
    }

    g_connclients.push_back(conn);
    lua_pushlightuserdata(L, conn);
    return 1;
}

static ConnClient* pop_conn_client(lua_State* L)
{
    if (!lua_islightuserdata(L, 1)) {
        luaL_error(L, "The first argument must be a valid connection!");
        return nullptr;
    }
    ConnClient* conn = (ConnClient*)lua_touserdata(L, 1);
    if (IsConnectionValid(conn)) {
        return conn;
    } else {
        luaL_error(L, "Invalid connection");
        return nullptr;
    }
}

static int lua_connclient_close(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);

    ConnClient* conn = pop_conn_client(L);
    if (conn) {
        conn->Close();
        RemoveConnection(conn);
    }
    return 0;
}

static int lua_connclient_send(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);

    ConnClient* conn = pop_conn_client(L);
    if (conn) {
        size_t msg_len = 0;
        const char* msg_buf = luaL_checklstring(L, 2, &msg_len);
        conn->SendMsg(msg_buf, (int)msg_len);
    }
    return 0;
}

static int lua_connclient_add_relink_interval(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);

    ConnClient* conn = pop_conn_client(L);
    if (conn) {
        int interval = luaL_checkinteger(L, 2);
        conn->AddRelinkInterval(interval);
    }
    return 0;
}

static int lua_connclient_set_magic_num(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);

    ConnClient* conn = pop_conn_client(L);
    if (conn) {
        int interval = luaL_checkinteger(L, 2);
        conn->SetMagicNum(interval);
    }
    return 0;
}

static int lua_connclient_set_logdebug_cb(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);

    ConnClient* conn = pop_conn_client(L);
    if (conn) {
        conn->SetLogDebugCB(dmScript::CreateCallback(L, 2));
    }
    return 0;
}

static int lua_connclient_set_loginfo_cb(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);

    ConnClient* conn = pop_conn_client(L);
    if (conn) {
        conn->SetLogInfoCB(dmScript::CreateCallback(L, 2));
    }
    return 0;
}

static int lua_connclient_set_logerror_cb(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);

    ConnClient* conn = pop_conn_client(L);
    if (conn) {
        conn->SetLogErrorCB(dmScript::CreateCallback(L, 2));
    }
    return 0;
}

static int lua_connclient_set_output_cb(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);

    ConnClient* conn = pop_conn_client(L);
    if (conn) {
        conn->SetOutputCB(dmScript::CreateCallback(L, 2));
    }
    return 0;
}

static int lua_connclient_set_disconnect_cb(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);

    ConnClient* conn = pop_conn_client(L);
    if (conn) {
        conn->SetDisconnectCB(dmScript::CreateCallback(L, 2));
    }
    return 0;
}

static int lua_connclient_set_connectsuccess_cb(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);

    ConnClient* conn = pop_conn_client(L);
    if (conn) {
        conn->SetConnectSuccessCB(dmScript::CreateCallback(L, 2));
    }
    return 0;
}

static int lua_connclient_set_relinksuccess_cb(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);

    ConnClient* conn = pop_conn_client(L);
    if (conn) {
        conn->SetRelinkSuccessCB(dmScript::CreateCallback(L, 2));
    }
    return 0;
}

static int lua_connclient_set_relink_cb(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);

    ConnClient* conn = pop_conn_client(L);
    if (conn) {
        conn->SetRelinkCB(dmScript::CreateCallback(L, 2));
    }
    return 0;
}

static const luaL_reg connclient_module_methods[] = {
    {"connect", lua_connclient_connect},
    {"close", lua_connclient_close},
    {"send", lua_connclient_send},
    {"add_relink_interval", lua_connclient_add_relink_interval},
    {"set_magic_num", lua_connclient_set_magic_num},
    {"set_logdebug_cb", lua_connclient_set_logdebug_cb},
    {"set_loginfo_cb", lua_connclient_set_loginfo_cb},
    {"set_logerror_cb", lua_connclient_set_logerror_cb},
    {"set_output_cb", lua_connclient_set_output_cb},
    {"set_disconnect_cb", lua_connclient_set_disconnect_cb},
    {"set_connectsuccess_cb", lua_connclient_set_connectsuccess_cb},
    {"set_relinksuccess_cb", lua_connclient_set_relinksuccess_cb},
    {"set_relink_cb", lua_connclient_set_relink_cb},
    {0, 0}};

static void LuaInit(lua_State* L)
{
    int top = lua_gettop(L);

    // Register lua names
    luaL_register(L, MODULE_NAME, connclient_module_methods);

    lua_pop(L, 1);
    assert(top == lua_gettop(L));
}

static dmExtension::Result AppInitializeMyExtension(dmExtension::AppParams* params)
{
    dmLogInfo("AppInitializeMyExtension");
    return dmExtension::RESULT_OK;
}

static dmExtension::Result InitializeMyExtension(dmExtension::Params* params)
{
    // Init Lua
    LuaInit(params->m_L);
    dmLogInfo("Registered %s Extension", MODULE_NAME);
    return dmExtension::RESULT_OK;
}

static dmExtension::Result AppFinalizeMyExtension(dmExtension::AppParams* params)
{
    dmLogInfo("AppFinalizeMyExtension");
    return dmExtension::RESULT_OK;
}

static dmExtension::Result FinalizeMyExtension(dmExtension::Params* params)
{
    dmLogInfo("FinalizeMyExtension");
    return dmExtension::RESULT_OK;
}

static dmExtension::Result OnUpdateMyExtension(dmExtension::Params* params)
{
    for (int i = 0; i < g_connclients.size(); ++i) {
        g_connclients[i]->Update();
    }
    return dmExtension::RESULT_OK;
}

static void OnEventMyExtension(dmExtension::Params* params, const dmExtension::Event* event)
{
    switch (event->m_Event) {
        case dmExtension::EVENT_ID_ACTIVATEAPP:
            dmLogInfo("OnEventMyExtension - EVENT_ID_ACTIVATEAPP");
            break;
        case dmExtension::EVENT_ID_DEACTIVATEAPP:
            dmLogInfo("OnEventMyExtension - EVENT_ID_DEACTIVATEAPP");
            break;
        case dmExtension::EVENT_ID_ICONIFYAPP:
            dmLogInfo("OnEventMyExtension - EVENT_ID_ICONIFYAPP");
            break;
        case dmExtension::EVENT_ID_DEICONIFYAPP:
            dmLogInfo("OnEventMyExtension - EVENT_ID_DEICONIFYAPP");
            break;
        default:
            dmLogWarning("OnEventMyExtension - Unknown event id");
            break;
    }
}

// Defold SDK uses a macro for setting up extension entry points:
//
// DM_DECLARE_EXTENSION(symbol, name, app_init, app_final, init, update,
// on_event, final)

// MyExtension is the C++ symbol that holds all relevant extension data.
// It must match the name field in the `ext.manifest`
DM_DECLARE_EXTENSION(connclient, LIB_NAME, AppInitializeMyExtension, AppFinalizeMyExtension,
                     InitializeMyExtension, OnUpdateMyExtension, OnEventMyExtension,
                     FinalizeMyExtension)
