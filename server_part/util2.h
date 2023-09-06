#pragma once
#include "../common/common.h"
#include "../common/sock.h"
#include "../common/reactor.h"

class ConnUtil {
public:
    // 创建套接字并绑定监听
    static int SocketListen(int port) {
        int listen_fd = Sock::Socket();
        if (!Sock::Bind(listen_fd, port))
        {
            return -1;
        }
        Sock::Listen(listen_fd);

        // 将该fd设置为非阻塞
        FileUtil::SetNoBlock(listen_fd);
        return listen_fd;
    }
};

class EventUtil {
public:
    // 以一个已进行监听的fd为目标建立对应Event，并加入到reactor中
    template<class Accepter>
    static void InsertListenEvent(Reactor* pR, int listen_fd, Accepter) {
        // 建立对应的event
        Event* listen_ev = new Event;
        listen_ev->fd = listen_fd;
        listen_ev->R_ = pR;
        listen_ev->RegisterCallBack(Accepter(), CallBack(), CallBack());

        // 将监听事件加入reactor, 采用ET模式
        // 因此所有的fd都应设置为非阻塞
        pR->InsertEvent(listen_ev, EPOLLET | EPOLLIN);
    }

    // 以一个已进行监听的fd为目标建立对应Event，并加入到reactor中
    // Event由自己指定
    template<class Accepter>
    static void InsertListenEvent(Reactor* pR, Event* ev, int listen_fd, Accepter) {
        // 建立对应的event
        ev->fd = listen_fd;
        ev->R_ = pR;
        ev->RegisterCallBack(Accepter(), CallBack(), CallBack());

        // 将监听事件加入reactor, 采用ET模式
        // 因此所有的fd都应设置为非阻塞
        pR->InsertEvent(ev, EPOLLET | EPOLLIN);
    }
};