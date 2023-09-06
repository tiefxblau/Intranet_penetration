#include "io_service.h"

// 接受user新连接
int Accepter_User::operator()(Event& ev)
{
    int listen_sock = ev.fd;
    // Log(DEBUG) << "userListen_fd: " << listen_sock << "正在获取新的用户链接" << std::endl;

    string self_ip;
    int self_port;
    Sock::SelfInfo(ev.fd, self_ip, self_port);
    
    int new_fd;
    // 循环读取，直到全连接队列没有链接
    while ((new_fd = Sock::Accept(listen_sock)) >= 0)
    {
        FileUtil::SetNoBlock(new_fd);

        Event* new_ev = new Event;
        new_ev->fd = new_fd;
        new_ev->R_ = ev.R_;

        new_ev->RegisterCallBack(Work_Recver(), Work_Sender(), WorkUserEventErrorer());
        // new_ev.RegisterCallBack(Recver(), Sender(), Errorer());

        // 申请建立与client通信的隧道
        // 在成功建立之前，会加入WorkManager的等待队列中，并加入到reactor中
        if (WorkManager::Get().MakeTunnel(new_ev, ev.peer_fd_) == false)
        {
            // 返回错误信息
            // ...
            delete new_ev;
            continue;
        }
        ev.R_->InsertEvent(new_ev, EPOLLET | EPOLLIN);
        Log(DEBUG) << "已建立user_fd " << new_fd << endl;
        Log(DEBUG) << "user_fd: " << new_fd << " 正在等待客户端回应..." << endl;

        // ev.R_->InsertEvent(new_ev, EPOLLIN | EPOLLET);
        // std::cout << "已获取新的用户fd: " << new_fd << std::endl;
    }

    return 0;
}

int Accepter_Client::operator()(Event& ev)
{
    int listen_sock = ev.fd;
    // Log(DEBUG) << "clientListen_fd: " << listen_sock << "正在获取新的客户端链接" << std::endl;
    
    int new_fd;
    // 循环读取，直到全连接队列没有链接
    while ((new_fd = Sock::Accept(listen_sock)) >= 0)
    {
        FileUtil::SetNoBlock(new_fd);

        Event* new_ev = new Event;
        new_ev->fd = new_fd;
        new_ev->R_ = ev.R_;

        new_ev->RegisterCallBack(Wait(), CallBack(), Errorer());
        // new_ev.RegisterCallBack(Recver(), Sender(), Errorer());

        ev.R_->InsertEvent(new_ev, EPOLLIN | EPOLLET);
        Log(DEBUG) << "已获取新的客户端fd: " << new_fd << std::endl;
    }

    return 0;
}

int Errorer::operator()(Event& ev)
{
    // Log(ERROR) << "call errorer" << std::endl;
    Log(INFO) << "已关闭fd: " << ev.fd << std::endl;
    FdError(ev.R_, ev.fd);
    return 0; 
}

int KeepEventErrorer::operator()(Event& ev)
{
    int peer_listen = ev.peer_fd_;
    ConnManager::Get().Logout(ev.fd);
    Log(INFO) << "已关闭fd: " << peer_listen << std::endl;
    FdError(ev.R_, peer_listen);
    Log(INFO) << "已关闭fd: " << ev.fd << std::endl;
    FdError(ev.R_, ev.fd);
    return 0;
}

int WorkUserEventErrorer::operator()(Event& ev)
{
    WorkManager::Get().DeleteTunnelFromUser(ev.fd);
    int fd = ev.fd, peer = ev.peer_fd_;
    Log(DEBUG) << "已关闭fd: " << ev.fd << std::endl;
    FdError(ev.R_, fd);
    if (peer >= 0)
    {
        Log(DEBUG) << "已关闭fd: " << peer << std::endl;
        FdError(ev.R_, peer);
    }
    return 0;
}

int WorkClientEventErrorer::operator()(Event& ev)
{
    WorkManager::Get().DeleteTunnelFromClient(ev.fd);
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

// 在client与server建立连接后，client发送信息前的中间状态
// 等待client的请求信息
int Wait::operator()(Event& ev)
{
    // 读取对端发来的信息，放入输入缓冲区
    int readRet = FileUtil::ReadInfo(ev.inbuffer_, ev.fd);
    Log(DEBUG) << LogFormat(ev.fd) << "read: " << ev.inbuffer_ << endl;
    if (readRet < 0)
    {
        Log(DEBUG) << "ReadInfo error: " << readRet << endl;
        CallBack& Err = *(ev.errorer_);
        Err(ev);
        return -1;
    }

    // 读取用户发来的json串
    string json_info;
    if (StrUtil::SplitRead(ev.inbuffer_, json_info, kSep) == false)
    {
        Log(DEBUG) << "SplitRead error" << endl;
        CallBack& Err = *(ev.errorer_);
        Err(ev);
        return -1;
    }

    // 解析用户发来的json串
    Json::Reader reader;
    Json::Value clientMsg;

    reader.parse(json_info, clientMsg);
    
    ClientReq req = static_cast<ClientReq>(clientMsg["req"].asInt());
    string content = clientMsg["content"].asString();

    // 对不同client请求类型作出不同响应
    ConnManager& connManager = ConnManager::Get();
    int ret = 0;
    switch (req)
    {
    case kLogin:    // 注册
        ret = DoLogin(ev, content);
        break;
    case kWorkConn: // 流量转发
        ret = DoWorKConn(ev, content);
        break;
    default:
        ret = -1;
        Log(FATAL) << "???" << endl;
        // assert(false);
        break;
    }

    if (ret < 0)
    {
        CallBack& Err = *(ev.errorer_);
        Err(ev);
        return -1;
    }

    return 0;
}

int Wait::DoLogin(Event& ev, const string& loginInfo)
{
    int mapped_port = ConnManager::Get().Login(loginInfo, ev.fd);
    if (mapped_port == -1)
    {
        Log(DEBUG) << "Login error" << endl;
        // 向客户端返回错误信息
        // ...

        // // 执行错误处理
        // CallBack& Err = *(ev.errorer_);
        // Err(ev);
        return -1;
    }

    // Log(DEBUG) << "fd: " << ev.fd << " 注册成功" << endl;
    // 注册成功
    // 向客户端返回成功信息
    // ...

    // 对已映射的端口进行监听，可以获取外界的信息了
    int listen_fd = ConnUtil::SocketListen(mapped_port);
    if (listen_fd < 0)
    {
        Log(DEBUG) << "Login error: mapped_port timewait" << endl;
        ConnManager::Get().Logout(loginInfo, ev.fd);
        return -1;
    }
    
    // 待修改
    ev.ModifyCallBack(Keep_Recver(), Keep_Sender(), KeepEventErrorer());
    // Log(DEBUG) << "fd: " << ev.fd << "(Wait->Keep_Recver)" << endl;

    Event* listen_ev = new Event;
    listen_ev->peer_fd_ = ev.fd;
    EventUtil::InsertListenEvent(ev.R_, listen_ev, listen_fd, Accepter_User());

    // 记录user端的listenfd
    ev.peer_fd_ = listen_fd;
    Log(DEBUG) << "已建立userListen_fd: " << listen_fd << "(Accepter_User)-> client_fd " << ev.fd << endl;
    return 0;
}

int Wait::DoWorKConn(Event& ev, const string& guid)
{
    // 与user端建立隧道
    if (WorkManager::Get().MakeTunnel_Rsp(&ev, guid) == false)
    {
        // 向客户端返回错误信息
        // ...

        // CallBack& Err = *(ev.errorer_);
        // Err(ev);
        return -1;
    }

    // 改变状态
    // 待修改
    ev.RegisterCallBack(Work_Recver(), Work_Sender(), WorkClientEventErrorer());
    Log(DEBUG) << "fd: " << ev.fd << " (Wait->Recver_Client)" << endl;
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
    
    Json::Reader reader;
    Json::Value clientMsg;

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
        reader.parse(json_info, clientMsg);
        
        ClientReq req = static_cast<ClientReq>(clientMsg["req"].asInt());
        // Log(DEBUG) << "req: " << clientMsg["req"].asInt() << endl;
        string content = clientMsg["content"].asString();

        // 对不同client请求类型作出不同响应
        ConnManager& connManager = ConnManager::Get();
        switch (req)
        {
        case kLogout: {   // 注销
            if (connManager.Logout(content, ev.fd) == false)
            {
                // 向客户端返回错误信息
                // ...

                // 执行错误处理
                // CallBack& Err = *(ev.errorer_);
                // Err(ev);
                return -1;
            }

            Log(DEBUG) << "fd: " << ev.fd << " 注销成功" << endl;
            // 注销成功
            // 向客户端返回成功信息
            // ...

            // 待修改
            CallBack& Err = *(ev.errorer_);
            Err(ev);
            Log(DEBUG) << "Err success" << endl;
            return 0;
            break;
        }
        case kHeartBeat:    // 心跳
            // Log(DEBUG) << "heart beat..." << endl;
            // 无内容，不做处理
            break;
        default:
            Log(FATAL) << req << " ???" << endl;
            CallBack& Err = *(ev.errorer_);
            Err(ev);
            return -1;
            break;
        }
    }

    // Log(DEBUG) << "client_recver end success" << endl;
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
    // Log(DEBUG) << LogFormat(ev.fd) << "read: " << ev.inbuffer_ << endl;
    if (readRet < 0)
    {
        // Log(DEBUG) << "read error" << endl;
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
    int peer_fd = WorkManager::Get().Peer(ev.fd);
    if (peer_fd == -1)
    {
        // UserFdError(ev.R_, ev.fd);
        return -1;
    }

    // Log(DEBUG) << LogFormat(ev.fd) << "->" << ev.inbuffer_ << LogFormat(ev.peer_fd_) << endl;
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

// json:
// conn {
//      req : login/workconn
//      content : 
//}
int Recver_Client::operator()(Event& ev)
{
    // int readRet = FileUtil::ReadInfo(ev.inbuffer_, ev.fd);
    // if (readRet < 0)
    // {
    //     CallBack& Err = *(ev.errorer_);
    //     Err(ev);
    //     return -1;
    // }

    // while (true)
    // {
    //     string json_info;
    //     if (StrUtil::SplitRead(ev.inbuffer_, json_info, kSep) == false)
    //     {
    //         break;
    //     }

    //     Json::Reader reader;
    //     Json::Value clientMsg;

    //     reader.parse(json_info, clientMsg);
        
    //     ClientReq req = static_cast<ClientReq>(clientMsg["req"].asInt());
    //     string content = clientMsg["content"].asString();

    //     switch (req)
    //     {
    //     case kLogin:
    //         /* code */
    //         break;
    //     case kWorkConn:

    //         break;
    //     default:
    //         assert(false);
    //         break;
    //     }
    // }
    
    return 0;
}