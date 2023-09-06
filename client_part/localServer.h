#pragma once
#include <string>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

class LocalServer {
public:

    static void Init(int local_port, const std::string& server_ip, int server_port) {
        ins_.local_port_ = local_port;
        ins_.server_ip_ = server_ip;
        ins_.server_port_ = server_port;
    }

    static int LocalPort() {return ins_.local_port_;}
    static int ServerPort() {return ins_.server_port_;}
    static std::string ServerIP() {return ins_.server_ip_;}
    static std::string LocalIP() {
        char hostname[256];
        gethostname(hostname, sizeof(hostname));

        hostent* host = gethostbyname(hostname);

        in_addr** address_list = (in_addr**)host->h_addr_list;
        return inet_ntoa(*address_list[0]);
    }
private:
    LocalServer() = default;
    LocalServer(LocalServer&) = delete;

    int local_port_;
    int server_port_;
    std::string server_ip_;

    static LocalServer ins_;
};

