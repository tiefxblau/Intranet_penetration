#pragma once
#include "../common/reactor.h"
#include "../common/common.h"
#include "util2.h"

#include "manager.h"

// listen_fd的读处理行为，获取全连接队列的链接，并进行一系列处理
class Accepter_User : public CallBack {
public:
    virtual int operator()(Event& ev);
};

class Accepter_Client : public CallBack {
public:
    virtual int operator()(Event& ev);
};

class Errorer : public CallBack {
public:
    virtual int operator()(Event& ev);
};

class KeepEventErrorer : public CallBack {
public:
    virtual int operator()(Event& ev);
};

class WorkUserEventErrorer : public CallBack {
public:
    virtual int operator()(Event& ev);
};

class WorkClientEventErrorer : public CallBack {
public:
    virtual int operator()(Event& ev);
};

// 在client与server建立连接后，client发送注册信息前的中间状态
// 等待client的注册信息
class Wait : public CallBack {
public:
    virtual int operator()(Event& ev);
private:
    static int DoLogin(Event& ev, const string& loginInfo);
    static int DoWorKConn(Event& ev, const string& loginInfo);
};

// 保证server与client的通信能力的链接
class Keep_Recver : public CallBack {
public:
    virtual int operator()(Event& ev);
};
class Keep_Sender : public CallBack {
public:
    virtual int operator()(Event& ev);
};

class Recver_Client : public CallBack {
public:
    // json:
    // conn {
    //      req : login/workconn
    //      content : 
    //}
    virtual int operator()(Event& ev);
};

class Work_Recver : public CallBack {
public:
    virtual int operator()(Event& ev);
};

class Work_Sender : public CallBack {
public:
    virtual int operator()(Event& ev);
};