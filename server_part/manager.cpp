#include "manager.h"

ConnManager ConnManager::inst_;
WorkManager WorkManager::inst_;


int ConnManager::Login(const string &loginInfo, int fd)
{
    // 找到没有被占用的端口, 将内网的ip+port映射到该port上
    int find = -1;
    for (auto &port_addr : portPool)
    {
        if (port_addr.second == -1)
        {
            find = port_addr.first;
            port_addr.second = fd;
            Log(DEBUG) << "fd: " << fd << " 已将服务在" << port_addr.first << "上开放" << endl;
            break;
        }
    }

    // 若端口被全部占用，返回-1
    if (find == -1)
    {
        return -1;
    }

    client_fd_[loginInfo] = fd;
    fd_client_[fd] = loginInfo;
    // Log(DEBUG) << "映射关系" << loginInfo << " : " << fd << endl;
    UpdateTime(fd);
    return find;
}

bool ConnManager::Logout(const string &logoutInfo, int fd)
{
    // 检查logoutInfo合法性
    if (client_fd_.find(logoutInfo) == client_fd_.end())
        return false;
    if (client_fd_[logoutInfo] != fd)
        return false;

    // 找到该fd对应的端口,并删除映射
    Logout(fd);
}

bool ConnManager::Logout(int fd)
{
    string loginInfo = fd_client_[fd];

    // 找到该fd对应的端口,并删除映射
    bool find = false;
    for (auto &port_fd : portPool)
    {
        if (port_fd.second == fd)
        {
            find = true;
            port_fd.second = -1;
            Log(DEBUG) << "fd: " << fd << " 已关闭" << port_fd.first << "上的服务" << endl;
            break;
        }
    }

    // 若端口被全部占用，返回false
    if (!find)
    {
        return false;
    }

    client_fd_.erase(loginInfo);
    fd_client_.erase(fd);
    Log(DEBUG) << "已删除映射关系 " << loginInfo << " : " << fd << endl;
    fd_lastPacketTime_.erase(fd);
    return true;
}

void ConnManager::HeartBeatThreadRun(ConnManager* pCM)
{
    Log(DEBUG) << "已创建timer_thread[" << std::this_thread::get_id() << "]" << endl;
    // 使用boost库中的定时器
    static HeartBeatChecker checker(pCM->R_, kTimerInterval, [pCM](const boost::system::error_code &ec){
        if (ec) return;

        // Log(DEBUG) << "DoCheckHeartBeat" << endl;

        {
            // 加锁
            std::unique_lock<std::mutex> lock(ConnManager::Get().Mutex());
            // 遍历每个客户端
            auto iter = pCM->fd_lastPacketTime_.begin();
            while (iter != pCM->fd_lastPacketTime_.end())
            {
                // 提前记录以防迭代器失效
                auto cur = iter++;
                Log(DEBUG) << "check fd: " << cur->first << endl;
                // 执行心跳检测
                checker.DoCheckHeartBeat(cur->first, cur->second);
            }
        }
        
        // 继续定时
        checker.Timer().expires_from_now(boost::posix_time::seconds(kTimerInterval));
        // timer.expires_at(timer.expires_at() + boost::posix_time::seconds(kTimerInterval));
        checker.Timer().async_wait(checker.Handle());
    });
    
    checker.Timer().async_wait(checker.Handle());

    checker.Run();
}

void ConnManager::StartCheckHeartBeat()
{
    std::thread timer_thread(ConnManager::HeartBeatThreadRun, this);
    timer_thread.detach();
}

// 待修改
void WorkManager::GetUserInfo(int user_fd, string& user_ip, int& user_port)
{
    Sock::PeerInfo(user_fd, user_ip, user_port);
}

bool WorkManager::MakeTunnel(Event* user_ev, int client_fd)
{
    // 分配guid
    string guid = MakeGuid(user_ev->fd);
    // 加入等待队列
    waitQueue_[guid] = user_ev;

    // 制作请求信息
    ServerReqMsg sendMsg;
    sendMsg.req = kSwap;
    sendMsg.content = guid;

    // 序列化
    string buffer = SerializeMsg(sendMsg);
    buffer += kSep;

    // 向client发送请求信息
    int sendRet = FileUtil::SendInfo(buffer, client_fd);
    // 待修改 sendRet == -1?
    if (sendRet < 0)
    {
        // UserFdError(R_, user_ev.fd);
        return false;
    }
    return true;
}

bool WorkManager::MakeTunnel_Rsp(Event* client_ev, const string& guid)
{
    // 根据guid查找对端
    auto iter = waitQueue_.find(guid);
    if (iter == waitQueue_.end())
    {
        return false;
    }

    Event* user_ev = iter->second;

    // 建立映射
    int user_fd = user_ev->fd;
    user_ev->peer_fd_ = client_ev->fd;
    client_ev->peer_fd_ = user_fd;
    req_fdMap_[user_fd] = client_ev->fd;
    rsp_fdMap_[client_ev->fd] = user_fd;

    // 将userEvent从等待队列取出
    waitQueue_.erase(iter);

    // 向user发送成功建立连接的信息
    // ...
    // string successMsg = "success\n";
    // FileUtil::SendInfo(successMsg, user_fd);

    // 删除有关网络传输的guid ?(待定)
    // ...
    
    Log(DEBUG) << "user_fd " << user_fd << " 和" << "client_fd " << client_ev->fd << "成功建立隧道" << endl;
    return true;
}

bool WorkManager::DeleteTunnelFromUser(int user_fd)
{
    int client_fd = Peer(user_fd);
    if (client_fd >= 0)
    {
        // 删除隧道两端的映射关系
        req_fdMap_.erase(user_fd);
        rsp_fdMap_.erase(client_fd);
    }
    else
    {
        // 找不到对端说明该fd是出于等待队列的userfd
        waitQueue_.erase(fd_guid_[user_fd]);
    }

    // 删除guid
    fd_guid_.erase(user_fd);
    return true;
}

bool WorkManager::DeleteTunnelFromClient(int client_fd)
{
    int user_fd = Peer(client_fd);
    if (client_fd >= 0)
    {
        // 删除隧道两端的映射关系
        req_fdMap_.erase(user_fd);
        rsp_fdMap_.erase(client_fd);
    }
    else
    {
        Log(FATAL) << "???" << endl;
    }

    // 删除guid
    fd_guid_.erase(user_fd);
    return true;
}

string WorkManager::SerializeMsg(const ServerReqMsg& msg) 
{
    Json::FastWriter writer;
    Json::Value value;

    value["req"] = msg.req;
    value["content"] = msg.content;

    return writer.write(value);
}

int WorkManager::Peer(int fd)
{
    auto iter = req_fdMap_.find(fd);
    if (iter != req_fdMap_.end())
        return iter->second;
    
    iter = rsp_fdMap_.find(fd);
    if (iter != rsp_fdMap_.end())
        return iter->second;

    // Log(FATAL) << "Peer: 未知fd" << endl;
    return -1;
}