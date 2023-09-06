#pragma once
#include "common.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

class Sock
{
public:
    static int Socket()
    {
        int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sockfd < 0)
        {
            Log(FATAL) << "socket() fail" << endl;
            exit(1);
        }

        // for debug
        // 开启reuseaddr,可重用在timewait状态下的端口
        int opt = 1;
        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        return sockfd;
    }

    static bool Bind(int sockfd, int port)
    {
        sockaddr_in local;
        local.sin_addr.s_addr = INADDR_ANY;
        local.sin_family = AF_INET;
        local.sin_port = htons(port);

        if (bind(sockfd, (const sockaddr*)&local, sizeof(local)) < 0)
        {
            Log(FATAL) << "bind() fail" << endl;
            return false;
        }
        return true;
    }

    static void Listen(int sockfd)
    {
        if (listen(sockfd, 5) < 0)
        {
            Log(FATAL) << "listen() fail" << endl;
            exit(1);
        }
    }

    static int Accept(int sockfd)
    {
        sockaddr_in peer;
        socklen_t len = sizeof(peer);

        int newfd = accept(sockfd, (sockaddr*)&peer, &len);

        return newfd;
    }

    static int Accept(int sockfd, string& peer_ip, int& peer_port)
    {
        sockaddr_in peer;
        socklen_t len = sizeof(peer);

        int newfd = accept(sockfd, (sockaddr*)&peer, &len);
        peer_ip = inet_ntoa(peer.sin_addr);
        peer_port = ntohs(peer.sin_port);

        return newfd;
    }

    static void PeerInfo(int sockfd, string& peer_ip, int& peer_port)
    {
        sockaddr_in peer;
        socklen_t len = sizeof(peer);

        if (getpeername(sockfd, (sockaddr*)&peer, &len) != 0)
        {
            Log(FATAL) << "getpeername error" << endl;
            exit(1);
        }

        peer_ip = inet_ntoa(peer.sin_addr);
        peer_port = ntohs(peer.sin_port);
    }

    static void SelfInfo(int sockfd, string& self_ip, int& self_port)
    {
        sockaddr_in self;
        socklen_t len = sizeof(self);

        if (getsockname(sockfd, (sockaddr*)&self, &len) != 0)
        {
            Log(FATAL) << "getpeername error" << endl;
            exit(1);
        }

        self_ip = inet_ntoa(self.sin_addr);
        self_port = ntohs(self.sin_port);
    }

    static void Connect(int sockfd, const string& ip, int port)
    {
        sockaddr_in dst;
        dst.sin_addr.s_addr = inet_addr(ip.c_str());
        dst.sin_family = AF_INET;
        dst.sin_port = htons(port);

        if (connect(sockfd, (const sockaddr*)&dst, sizeof(dst)) < 0)
        {
            Log(ERROR) << "connect() error" << endl;
            exit(1);
        }
    }
};