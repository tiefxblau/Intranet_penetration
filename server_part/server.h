#pragma once
#include "../common/common.h"
#include "../common/sock.h"
#include "../common/reactor.h"
#include "manager.h"
#include "util2.h"
#include "io_service.h"

const string ip = "101.43.183.8"; 
constexpr int port_user = 8080;
constexpr int port_client = 8081;

// 是否开启心跳检测
constexpr bool enableHeartBeat = false;

class ServerControl {
public:
    // 入口，服务运行
    void Run()
    {
        // 待修改
        ConnManager::Init(reactor_);
        WorkManager::Init(reactor_);

        // // 建立监听用户的socket
        // int listen_fd_user = ConnUtil::SocketListen(port_user);
        // // 以该fd为目标建立对应Event，并加入到reactor中
        // EventUtil::InsertListenEvent(reactor_, listen_fd_user, Accepter_User());


        // 建立监听客户端的socket
        int listen_fd_client = ConnUtil::SocketListen(port_client);
        if (listen_fd_client < 0)
            exit(1);

        EventUtil::InsertListenEvent(reactor_, listen_fd_client, Accepter_Client());
        

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
