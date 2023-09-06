#include <iostream>
#include <string>
#include <unistd.h>
#include "Socket.hpp"
#include "Protocol.hpp"

void Usage(std::string proc)
{
    std::cout << "Usage:\n\t" << proc << " server_ip server_port" << std::endl;
}

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        Usage(argv[0]);
        return 1;
    }

    int port = atoi(argv[2]);
    std::string ip = argv[1];

    int sockfd = Sock::Socket();
    Sock::Connect(sockfd, ip, port);

    Request req;
    std::cout << "Please enter the 1st data: ";
    std::cin >> req._x;
    std::cout << "Please enter the 2nd data: ";
    std::cin >> req._y;
    std::cout << "Please enter operator: ";
    std::cin >> req._op;

    std::string json_req = SerializeRequest(req);
    write(sockfd, json_req.c_str(), json_req.size());

    char buffer[1024] = { 0 };
    read(sockfd, buffer, sizeof(buffer) - 1);
    std::string json_resp = buffer;
    std::cout << "get response: " << json_resp << std::endl;

    return 0;
    
}