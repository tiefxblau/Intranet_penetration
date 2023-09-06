#pragma once
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <string>

class Sock
{
public:
    static int Socket()
    {
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (socket < 0)
        {
            std::cout << "socket error" << std::endl;
            exit(1);
        }
        return sockfd;
    }

    static void Bind(int sockfd, int port)
    {
        sockaddr_in local;
        local.sin_addr.s_addr = INADDR_ANY;
        local.sin_family = AF_INET;
        local.sin_port = htons(port); 

        if (bind(sockfd, (const sockaddr*)&local, sizeof(local)) < 0)
        {
            std::cout << "bind error" << std::endl;
            exit(2);
        }
    }

    static void Listen(int sockfd)
    {
        if (listen(sockfd, 5) < 0)
        {
            std::cout << "listen error" << std::endl;
            exit(3);
        }
    }

    static int Accept(int sockfd)
    {
        sockaddr_in peer;
        socklen_t len = sizeof(peer);

        int newfd = accept(sockfd, (sockaddr*)&peer, &len);
        return newfd;
    }

    static void Connect(int sockfd, std::string ip, int port)
    {
        sockaddr_in dst;
        dst.sin_addr.s_addr = inet_addr(ip.c_str());
        dst.sin_family = AF_INET;
        dst.sin_port = htons(port);

        if (connect(sockfd, (const sockaddr*)&dst, sizeof(dst)) < 0)
        {
            std::cout << "connect error" << std::endl;
            exit(4);
        }
    }
};