#pragma once
#include "../common/common.h"
#include "../common/sock.h"
#include "../common/reactor.h"
#include "../common/message.h"
#include "manager.h"
#include "io_service.h"

const string ip = "101.43.183.8";
constexpr int port = 8081;
constexpr int localServerPort = 8083;

// 是否开启心跳检测
constexpr bool enableHeartBeat = true;

class ServerControl {
public:
    // 入口，服务运行
    void Run()
    {
        // 待修改
        ConnManager::Init(reactor_);
        LocalServer::Init(localServerPort, ip, port);

        // 建立socket
        int keep_fd = ConnManager::Get().LinkToServer();

        Event* ev = new Event;
        ev->fd = keep_fd;
        ev->R_ = reactor_;
        ev->RegisterCallBack(Keep_Recver(), Keep_Sender(), Errorer());
        reactor_->InsertEvent(ev, EPOLLET | EPOLLIN);
        
        // 
        Log(INFO) << "服务成功运行" << endl;
        // 如果选项中开启了心跳检测
        if (enableHeartBeat)
            ConnManager::Get().StartCheckHeartBeat();

        reactor_->Dispatcher();
    }

private:

    Reactor* reactor_ = new Reactor;
    
};
