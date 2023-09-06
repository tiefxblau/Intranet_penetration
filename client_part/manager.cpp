#include "manager.h"

ConnManager ConnManager::inst_;

int ConnManager::LinkToServer()
{
    int keep_fd = Sock::Socket();
    Sock::Connect(keep_fd, LocalServer::ServerIP(), LocalServer::ServerPort());
    FileUtil::SetNoBlock(keep_fd);
    Log(DEBUG) << "连接服务器成功" << endl;

    ClientReqMsg login;
    login.req = kLogin;
    login.content = LocalServer::LocalIP() + ":" + std::to_string(LocalServer::LocalPort());
    string login_json = SerializeMsg(login);
    login_json += kSep;
    
    FileUtil::SendInfo(login_json, keep_fd);
    
    keep_fd_ = keep_fd;
    return keep_fd;
}

void ConnManager::BindFd(int toServer, int toLocal)
{
    local_server_[toLocal] = toServer;
    server_local_[toServer] = toLocal;
    Log(DEBUG) << "fd" << LogFormat(toServer) << " 与 " << LogFormat(toLocal) << "绑定" << endl;
    UpdateTime(toLocal);
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

            checker.DoCheckHeartBeat(pCM->keep_fd_, pCM->lastSendTime_);
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

void ConnManager::UnbindFd(int toServer, int toLocal)
{
    local_server_.erase(toLocal);
    server_local_.erase(toServer);
}
