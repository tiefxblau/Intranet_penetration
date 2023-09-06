#pragma once
#include <sys/epoll.h>
#include "common.h"
#include "thread_pool.h"

class Event;
class Reactor;

// 回调函数基类
class CallBack
{
public:
    virtual int operator()(Event& ev) {return 0;}
};

// 事件 每一个sockfd对应一个事件
class Event
{
public:
    ~Event()
    {
        DeleteCallBack();
    }

    int fd = -1;
    // 每个event的读缓冲区和写缓冲区
    std::string inbuffer_;
    std::string outbuffer_;

    // 对端fd
    int peer_fd_ = -1;

    // 读处理、写处理、错误处理
    CallBack* recver_ = nullptr;
    CallBack* sender_ = nullptr;
    CallBack* errorer_ = nullptr;

    // 回指所属的reactor
    Reactor* R_ = nullptr;

    // 注册回调处理
    template <class Recver, class Sender, class Errorer>
    void RegisterCallBack(const Recver& _recver, const Sender& _sender, const Errorer& _errorer)
    {
        recver_ = new Recver(_recver);
        sender_ = new Sender(_sender);
        errorer_ = new Errorer(_errorer);
    }

    // 修改回调处理
    template <class Recver, class Sender, class Errorer>
    void ModifyCallBack(const Recver& _recver, const Sender& _sender, const Errorer& _errorer)
    {
        DeleteCallBack();
        RegisterCallBack(_recver, _sender, _errorer);
    }

    // 删除回调处理
    void DeleteCallBack()
    {
        delete recver_;
        delete sender_;
        delete errorer_;
    }
};


class Task {
public:
    Task(Event* pev, uint32_t ep_events) : pev_(pev), ep_events_(ep_events){}
    Task() = default;
    void operator()() {
        // EPOLLERR: 对端fd发生错误     EPOLLHUP： 对端fd异常关闭
        // 这里交给读事件和写事件处理
        if (ep_events_ & EPOLLERR)   ep_events_ |= EPOLLIN | EPOLLOUT;
        if (ep_events_ & EPOLLHUP)   ep_events_ |= EPOLLIN | EPOLLOUT;

        int ret = 0;
        // 该事件读就绪
        if (ep_events_ & EPOLLIN)
        {
            // Log(DEBUG) << "fd: " << sockfd << "读就绪" << std::endl; 
            // 若读处理已被注册，则执行该读处理
            if (pev_->recver_)
            {
                CallBack& Recver = *(pev_->recver_);
                ret = Recver(*pev_);
            }
            // std::cout << "完成读" << std::endl;
        }

        // ret < 0，表示recver发现对端关闭，该event已被删除，无需执行后续操作
        if (ret < 0)
            return;

        // 该事件写就绪
        if (ep_events_ & EPOLLOUT)
        {
            // Log(DEBUG) << "fd: " << sockfd << "写就绪" << std::endl; 
            // 若写处理已被注册，则执行该写处理
            if (pev_->sender_)
            {
                CallBack& Sender = *(pev_->sender_);
                Sender(*pev_);
            }
        }
    }

    Event* pev_ = nullptr;
    uint32_t ep_events_;
};

constexpr int kNum = 1024;
class Reactor
{
private:
    // epoll的句柄
    int epfd;

    // sockfd和对应event的映射
    std::unordered_map<int, Event*> events;
public:
    Reactor()
    {
        epfd = epoll_create(1);
        if (epfd < 0)
        {
            Log(FATAL) << "epoll create failed" << endl;
            exit(2);
        }
    }

    const Event* GetEvent(int fd)
    {
        if (events.find(fd) != events.end())
            return events[fd];
        else
            return nullptr;
    }

    // 向reactor中加入事件
    bool InsertEvent(Event* ev, uint32_t ep_events)
    {
        // 监听事件ev的fd
        epoll_event ep;
        ep.events = ep_events;
        ep.data.fd = ev->fd;
        if (epoll_ctl(epfd, EPOLL_CTL_ADD, ev->fd, &ep) < 0)
        {
            Log(ERROR) << "epoll add failed" << std::endl;
            return false;
        }

        // 建立ev的fd与ev的映射
        events[ev->fd] = ev;
        // Log(DEBUG) << "已添加fd: " << ev->fd << std::endl;
        return true;
    }

    // 删除reactor中的事件
    bool DeleteEvent(int fd)
    {
        // 取消epoll对该fd的监听
        if (epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr) < 0)
        {
            std::cerr << "epoll delete failed" << std::endl;
            return false;
        }

        // 删除fd对应的映射
        auto it = events.find(fd);
        if (it != events.end())
        {
            delete it->second;
            it->second = nullptr;
            // it->second->DeleteCallBack();
            events.erase(fd);
        }
        // Log(DEBUG) << "已删除fd: " << fd << std::endl;
        return true;
    }

    // 更新事件
    bool ModifyEvent(int fd, uint32_t ep_events)
    {
        // 对epoll监听的fd进行修改
        epoll_event ep;
        ep.events = ep_events;
        ep.data.fd = fd;
        if (epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ep) < 0)
        {
            Log(DEBUG) << "epoll modify failed" << std::endl;
            return false;
        }

        // Log(DEBUG) << "已修改fd: " << fd << std::endl;
        return true;
    }

    // 事件派发
    void Dispatcher()
    {
        while (true)
        {
            // 等待文件描述符上的事件就绪
            epoll_event eps[kNum];
            // Log(DEBUG) << "epoll wait..." << std::endl;
            int n = epoll_wait(epfd, eps, kNum, -1);
            if (n < 0)
            {
                Log(FATAL) << "epoll wait failed" << std::endl;
                exit(2);
            }
            else if (n == 0) // 等待时间超过timeout
            {
                Log(DEBUG) << "epoll wait timeout" << std::endl;
            }
            else // 有事件就绪
            {
                // Log(DEBUG) << "已经有fd就绪" << std::endl;
                // 遍历ep_events数组
                for (int i = 0; i < n; i++)
                {
                    int sockfd = eps[i].data.fd;
                    uint32_t ep_events = eps[i].events;

                    Task task(events[sockfd], ep_events);
                    ns_thread_pool::ThreadPool<Task>::GetInstance().Push(task);

                    // // EPOLLERR: 对端fd发生错误     EPOLLHUP： 对端fd异常关闭
                    // // 这里交给读事件和写事件处理
                    // if (ep_events & EPOLLERR)   ep_events |= EPOLLIN | EPOLLOUT;
                    // if (ep_events & EPOLLHUP)   ep_events |= EPOLLIN | EPOLLOUT;

                    // int ret = 0;
                    // // 该事件读就绪
                    // if (ep_events & EPOLLIN)
                    // {
                    //     // Log(DEBUG) << "fd: " << sockfd << "读就绪" << std::endl; 
                    //     // 若读处理已被注册，则执行该读处理
                    //     if (events[sockfd]->recver_)
                    //     {
                    //         CallBack& Recver = *(events[sockfd]->recver_);
                    //         ret = Recver(*events[sockfd]);
                    //     }
                    //     // std::cout << "完成读" << std::endl;
                    // }

                    // // ret < 0，表示recver发现对端关闭，该event已被删除，无需执行后续操作
                    // if (ret < 0)
                    //     continue;

                    // // 该事件写就绪
                    // if (ep_events & EPOLLOUT)
                    // {
                    //     Log(DEBUG) << "fd: " << sockfd << "写就绪" << std::endl; 
                    //     // 若写处理已被注册，则执行该写处理
                    //     if (events[sockfd]->sender_)
                    //     {
                    //         CallBack& Sender = *(events[sockfd]->sender_);
                    //         Sender(*events[sockfd]);
                    //     }
                    // }
                }
            }
        }
    }
};
