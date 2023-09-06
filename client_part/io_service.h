#pragma once
#include "../common/reactor.h"
#include "../common/common.h"

#include "manager.h"


class Errorer : public CallBack {
public:
    virtual int operator()(Event& ev);
};

class Work_Errorer : public CallBack {
public:
    virtual int operator()(Event& ev);
};

// 保证server与client的通信能力的链接
class Keep_Recver : public CallBack {
public:
    virtual int operator()(Event& ev);
private:
    static int DoSwap(Event& ev, const string& guid);
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