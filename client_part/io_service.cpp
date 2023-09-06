#include "io_service.h"

int Errorer::operator()(Event& ev)
{
    // Log(ERROR) << "call errorer" << std::endl;
    Log(INFO) << "已关闭fd: " << ev.fd << std::endl;
    FdError(ev.R_, ev.fd);
    exit(1);
    return 0; 
}

int Work_Errorer::operator()(Event& ev)
{
    int fd = ev.fd, peer = ev.peer_fd_;
    Log(DEBUG) << "已关闭fd: " << fd << std::endl;
    FdError(ev.R_, fd);
    if (peer >= 0)
    {
        Log(DEBUG) << "已关闭fd: " << peer << std::endl;
        FdError(ev.R_, peer);
    }
    return 0;
}

int Keep_Recver::operator()(Event& ev)
{
    // 更新时间
    ConnManager::Get().UpdateTime(ev.fd);

    // 读取对端发来的信息，放入输入缓冲区
    int readRet = FileUtil::ReadInfo(ev.inbuffer_, ev.fd);
    // Log(DEBUG) << LogFormat(ev.fd) << "{Keep_Recver}" << "read: " << ev.inbuffer_ << endl;
    if (readRet < 0)
    {
        CallBack& Err = *(ev.errorer_);
        Err(ev);
        return -1;
    }


    string json_info;
    
    // 循环读取用户发来的json串
    while (!ev.inbuffer_.empty())
    {
        if (StrUtil::SplitRead(ev.inbuffer_, json_info, kSep) == false)
        {
            CallBack& Err = *(ev.errorer_);
            Err(ev);
            // assert(false);
            Log(FATAL) << "???" << endl;
            return -1;
        }

        // Log(DEBUG) << json_info << endl;
        // 解析用户发来的json串
        ServerReqMsg msg = DeserializeMsg(json_info);

        // 对不同server请求类型作出不同响应
        ConnManager& connManager = ConnManager::Get();
        switch (msg.req)
        {
        case kSwap: 
            DoSwap(ev, msg.content);
            break;
        default:
            Log(FATAL) << msg.req << " ???" << endl;
            CallBack& Err = *(ev.errorer_);
            Err(ev);
            return -1;
            break;
        }
    }

    // Log(DEBUG) << "client_recver end success" << endl;
    return 0;
}

int Keep_Recver::DoSwap(Event& ev, const string& guid)
{
    // 向中转服务器和本地服务发起socket请求
    int toServer = Sock::Socket();
    Sock::Connect(toServer, LocalServer::ServerIP(), LocalServer::ServerPort());
    FileUtil::SetNoBlock(toServer);
    
    int toLocal = Sock::Socket();
    Sock::Connect(toLocal, "127.0.0.1", LocalServer::LocalPort());
    FileUtil::SetNoBlock(toLocal);

    // 加入reactor中
    Event* s_ev = new Event;
    s_ev->fd = toServer;
    s_ev->R_ = ev.R_;
    s_ev->peer_fd_ = toLocal;
    s_ev->RegisterCallBack(Work_Recver(), Work_Sender(), Work_Errorer());
    ev.R_->InsertEvent(s_ev, EPOLLET | EPOLLIN);

    Event* l_ev = new Event;
    l_ev->fd = toLocal;
    l_ev->R_ = ev.R_;
    l_ev->peer_fd_ = toServer;
    l_ev->RegisterCallBack(Work_Recver(), Work_Sender(), Work_Errorer());
    ev.R_->InsertEvent(l_ev, EPOLLET | EPOLLIN);

    // 向server发送请求信息
    ClientReqMsg clientMsg;
    clientMsg.content = guid;
    clientMsg.req = kWorkConn;
    string json_str = SerializeMsg(clientMsg);
    json_str += kSep;
    if (FileUtil::SendInfo(json_str, toServer) < 0)
    {
        CallBack& Err = *s_ev->errorer_;
        Err(*s_ev);
        return -1;
    }

    ConnManager::Get().BindFd(toServer, toLocal);

    return 0;
}

int Keep_Sender::operator()(Event& ev)
{
    return 0;
}

// 用于获取发送来的信息
int Work_Recver::operator()(Event& ev)
{
    // 读入读缓冲区
    int readRet = FileUtil::ReadInfo(ev.inbuffer_, ev.fd);
    Log(DEBUG) << LogFormat(ev.fd) << "read: " << ev.inbuffer_ << endl;
    if (readRet < 0)
    {
        Log(DEBUG) << "read error: " << readRet << endl;
        CallBack& Err = *(ev.errorer_);
        Err(ev);
        return -1;
    }

    ev.R_->ModifyEvent(ev.fd, EPOLLET | EPOLLIN | EPOLLOUT);   

    // // 获取user的信息
    // string user_ip;
    // int user_port;
    // WorkManager::Get().GetUserInfo(ev.fd, user_ip, user_port);
    return 0;
}

// 用于转发发送来的信息
int Work_Sender::operator()(Event& ev)
{
    // 获取对端
    int peer_fd = ev.peer_fd_;
    // int peer_fd = WorkManager::Get().Peer(ev.fd);
    if (peer_fd == -1)
    {
        // UserFdError(ev.R_, ev.fd);
        return -1;
    }

    Log(DEBUG) << LogFormat(ev.fd) << "->" << ev.inbuffer_ << LogFormat(ev.peer_fd_) << endl;
    // 向对端转发信息
    int sendRet = FileUtil::SendInfo(ev.inbuffer_, peer_fd);
    if (sendRet == 0)
    {
        ev.R_->ModifyEvent(ev.fd, EPOLLET | EPOLLIN);
    }
    else if (sendRet == -2)
    {
        CallBack& Err = *(ev.errorer_);
        Err(ev);
        return -2;
    }

    return 0;
}
