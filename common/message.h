#pragma once
#include <string>

enum ClientReq {
    kLogin = 0,     // 客户端注册内网服务
    kWorkConn = 1,   // 流量转发
    kHeartBeat = 2,  // 心跳
    kLogout = 3     // 客户端注销内网服务
};

enum ServerReq {
    kSwap = 0
};

// server向client发送的请求信息
struct ServerReqMsg
{
    ServerReq req;
    std::string content;
};

// server向client发送的请求信息
struct ClientReqMsg
{
    ClientReq req;
    std::string content;
};