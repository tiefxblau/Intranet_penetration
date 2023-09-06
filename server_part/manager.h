#pragma once
#include <atomic>
#include <jsoncpp/json/json.h>
#include <boost/asio.hpp>

#include "../common/common.h"
#include "../common/sock.h"
#include "../common/reactor.h"
#include "../common/message.h"

namespace asio = boost::asio;

// 使用该字符作为分割字符
constexpr char kSep = 3;

// 对应fd发生错误的处理方法
static void FdError(Reactor* pR, int err_fd);

static void UserFdError(Reactor* pR, int err_fd);


// 连接管理类
class ConnManager {
private:
    ConnManager() = default;
    ConnManager(const ConnManager&) = delete;
public:
    static void Init(Reactor* R) {inst_.R_ = R;}
    static ConnManager& Get() {return inst_;}

    // 注册客户端信息
    // 返回值 成功：该客户端映射的服务器端口 / 失败：-1
    int Login(const string& loginInfo, int fd);

    // 注销客户端信息
    bool Logout(const string& logoutInfo, int fd);
    bool Logout(int fd);

    // 开启心跳检测
    void StartCheckHeartBeat();

    // 更新某个fd的最后收到包的时间
    void UpdateTime(int fd) {
        fd_lastPacketTime_[fd] = TimeUtil::Now();
    }

    string LoginInfo(int fd) {
        auto iter = fd_client_.find(fd);
        if (iter == fd_client_.end())
        {
            Log(FATAL) << "未注册的fd" << endl;
            return {};
        }
        return fd_client_[fd];
    }

    std::mutex& Mutex() {return mtx_;}

private:
    static void HeartBeatThreadRun(ConnManager* pCM);

    // 内网ip+port 与 sockfd 的映射
    std::unordered_map<string, int> client_fd_;

    // sockfd 与 内网ip+port 的映射
    std::unordered_map<int, string> fd_client_;

    // sockfd 与 最后收到包的时间 的映射
    std::unordered_map<int, int64_t> fd_lastPacketTime_;

    // 公网端口 与 sockfd
    // 初始化时将可用port全部注册
    std::unordered_map<int, int> portPool = {{8082, -1}, {9001, -1}, {9002, -1}};

    std::mutex mtx_;

    static ConnManager inst_;

    Reactor* R_ = nullptr;
};


// 定时器周期时间(s)
constexpr int64_t kTimerInterval = 5;

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
        if (now - lastTime > kMaxClientInterval)
        {
            // 超时处理
            HeartBeatTimeOut(fd);
        }
    }

    // 心跳检测超时的处理方法
    void HeartBeatTimeOut(int fd)
    {
        // 将该客户端的注册信息及有关映射删除
        string logoutInfo = ConnManager::Get().LoginInfo(fd);
        ConnManager::Get().Logout(logoutInfo, fd);

        // 删除reactor中对应的事件并关闭fd
        FdError(R_, R_->GetEvent(fd)->peer_fd_);
        Log(DEBUG) << R_->GetEvent(fd)->peer_fd_ << ": " << "已关闭(超时)" << endl;
        FdError(R_, fd);
        Log(DEBUG) << fd << ": " << "已关闭(超时)" << endl;
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

// 通信管理类
class WorkManager {
    friend void UserFdError(Reactor* pR, int err_fd);
private:
    WorkManager() = default;
    WorkManager(const WorkManager&) = delete;

public:
    static WorkManager& Get() {return inst_;}
    static void Init(Reactor* R) {inst_.R_ = R;}

    // 获取user的信息
    void GetUserInfo(int user_fd, string& user_ip, int& user_port);

    // 尝试建立user与client通信的隧道
    bool MakeTunnel(Event* user_ev, int client_fd);

    // client发来回应，建立user与client通信的隧道；
    bool MakeTunnel_Rsp(Event* client_fd, const string& guid);

    // 删除与fd有关的隧道
    bool DeleteTunnelFromUser(int user_fd);

    bool DeleteTunnelFromClient(int client_fd);

    // 找到对端fd
    int Peer(int fd);

    string MakeGuid(int fd) {
        string guid = std::to_string(TimeUtil::TimeStamp()) + "_" + std::to_string(ascNum_++);
        fd_guid_[fd] = guid;
        return guid;
    }

private:
    // 使用json序列化
    string SerializeMsg(const ServerReqMsg& msg);

private:
    static WorkManager inst_;

    Reactor* R_ = nullptr;

    // 一个递增的数，guid的一部分
    std::atomic<uint> ascNum_;

    
    // 用于查询 user->client 和 client->user 的fd的一对一对应关系
    std::unordered_map<int, int> req_fdMap_;

    std::unordered_map<int, int> rsp_fdMap_;

    // 为每个user的socket分配一个guid
    std::unordered_map<int, string> fd_guid_;

    // 等待建立隧道的userEvent
    std::unordered_map<string, Event*> waitQueue_;
};


static void FdError(Reactor* pR, int err_fd)
{
    pR->DeleteEvent(err_fd);
    // Log(DEBUG) << "DeleteEvent success" << endl;
    close(err_fd);
    // Log(DEBUG) << "close success" << endl;
}

static void UserFdError(Reactor* pR, int err_fd)
{
    WorkManager& wm = WorkManager::Get();
    auto iter = wm.fd_guid_.find(err_fd);
    if (iter != wm.fd_guid_.end())
    {
        wm.waitQueue_.erase(iter->second);
        wm.fd_guid_.erase(iter);
    }
    FdError(pR, err_fd);
}