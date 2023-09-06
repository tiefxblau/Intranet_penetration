#pragma once
#include <atomic>
#include <jsoncpp/json/json.h>
#include <boost/asio.hpp>

#include "../common/common.h"
#include "../common/sock.h"
#include "../common/reactor.h"
#include "../common/message.h"

#include "localServer.h"

namespace asio = boost::asio;

// 对应fd发生错误的处理方法
static void FdError(Reactor* pR, int err_fd);

static string SerializeMsg(const ClientReqMsg& msg) 
{
    Json::FastWriter writer;
    Json::Value value;

    value["req"] = msg.req;
    value["content"] = msg.content;

    return writer.write(value);
}

static ServerReqMsg DeserializeMsg(const string& msg)
{
    Json::Reader reader;
    Json::Value server_json;
    reader.parse(msg, server_json);

    ServerReqMsg res;
    res.content = server_json["content"].asString();
    res.req = static_cast<ServerReq>(server_json["req"].asInt());
    return res;
}

// 使用该字符作为分割字符
constexpr char kSep = 3;

// 连接管理类
class ConnManager {
private:
    ConnManager() = default;
    ConnManager(const ConnManager&) = delete;
public:
    static void Init(Reactor* R) {inst_.R_ = R;}
    static ConnManager& Get() {return inst_;}

    int LinkToServer();

    void BindFd(int toServer, int toLocal);

    void UnbindFd(int toServer, int toLocal);

    // 开启心跳检测
    void StartCheckHeartBeat();

    // 更新某个fd的最后收到包的时间
    void UpdateTime(int fd) {
        lastSendTime_ = TimeUtil::Now();
    }

    std::mutex& Mutex() {return mtx_;}

private:
    static void HeartBeatThreadRun(ConnManager* pCM);

    // localfd 与 serverfd 的映射
    std::unordered_map<int, int> local_server_;

    // serverfd 与 localfd 的映射
    std::unordered_map<int, int> server_local_;

    int keep_fd_;

    // 最后发出包的时间
    int64_t lastSendTime_;

    std::mutex mtx_;

    static ConnManager inst_;

    Reactor* R_ = nullptr;
};


// 定时器周期时间(s)
constexpr int64_t kTimerInterval = 1 * 20;

// 最大允许客服端消息间隔时间(ms)
// 若客服端未与服务器通信时间大于kMaxClientInterval，则服务器认为客户端已离线
constexpr int64_t kMaxClientInterval = 1 * 60 * 1000;

// 作为ConnManager的心跳检测器
class HeartBeatChecker {
public:
    HeartBeatChecker() = default;
    HeartBeatChecker(Reactor* pR, int interval, std::function<void(const boost::system::error_code&)> handle)
        : timer_(io_, boost::posix_time::seconds(kTimerInterval))
        , handle_(handle)
        , R_(pR)
    {}

    // 启动HeartBeatChecker
    void Run() {io_.run();}

    // 对某个fd进行心跳检测
    void DoCheckHeartBeat(int fd, int64_t lastTime)
    {
        int64_t now = TimeUtil::Now();
        if (now - lastTime > kMaxClientInterval / 2)
        {
            // 超时处理
            HeartBeatTimeOut(fd);
        }
    }

    // 心跳检测超时的处理方法
    void HeartBeatTimeOut(int fd)
    {
        // 发送心跳包
        ClientReqMsg msg;
        msg.req = kHeartBeat;
        string sendMsg = SerializeMsg(msg);
        sendMsg += kSep;
        if (FileUtil::SendInfo(sendMsg, fd) < 0)
        {
            Log(DEBUG) << "send error" << endl;
        }
    }

    asio::deadline_timer& Timer() {return timer_;}
    std::function<void(const boost::system::error_code&)>& Handle() {return handle_;}
private:
    // 使用boost库中的定时器
    asio::io_service io_;
    asio::deadline_timer timer_;
    std::function<void(const boost::system::error_code&)> handle_;
    Reactor* R_;
};


static void FdError(Reactor* pR, int err_fd)
{
    pR->DeleteEvent(err_fd);
    // Log(DEBUG) << "DeleteEvent success" << endl;
    close(err_fd);
    // Log(DEBUG) << "close success" << endl;
}
