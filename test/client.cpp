#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <iostream>
#include <string>
#include <WinSock2.h>
#include <vector>

#pragma comment(lib, "ws2_32.lib")

std::string HostIP()
{
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) != 0) {
        std::cerr << "Failed to get hostname" << std::endl;
        return "?";
    }

    struct hostent* host = gethostbyname(hostname);
    if (host == nullptr) {
        std::cerr << "Failed to get host by name" << std::endl;
        return "?";
    }

    char* ipAddress = inet_ntoa(*(struct in_addr*)host->h_addr_list[0]);
    if (ipAddress == nullptr) {
        std::cerr << "Failed to get IP address" << std::endl;
        return "?";
    }

    return ipAddress;
}

SOCKET Socket()
{
    // 创建客户端Socket1
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        std::cout << "Failed to create client socket 1" << std::endl;
        WSACleanup();
        exit(1);
    }
    return sock;
}

int Connect(SOCKET sock, const std::string& ip, int port)
{
    // 设置服务器地址和端口号
    sockaddr_in serverAddr1;
    serverAddr1.sin_family = AF_INET;
    serverAddr1.sin_port = htons(port);
    serverAddr1.sin_addr.s_addr = inet_addr(ip.c_str());

    // 连接到服务器1
    if (connect(sock, reinterpret_cast<sockaddr*>(&serverAddr1), sizeof(serverAddr1)) == SOCKET_ERROR) {
        std::cout << "Failed to connect to server 1" << std::endl;
        closesocket(sock);
        WSACleanup();
        exit(1);
    }
}

const std::string ip = "101.43.183.8";
constexpr int port = 8081;
constexpr int localPort = 8888;

int main() {
    // 初始化 Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cout << "Failed to initialize winsock" << std::endl;
        return 1;
    }

    SOCKET keep = Socket();
    Connect(keep, ip, port);
    std::string str = "{\"req\": 0,\"content\": \"" + HostIP() + ":" + std::to_string(port) + "\"}\3";
    send(keep, str.c_str(), str.size(), 0);
    std::cout << "Connected to server success" << std::endl;

    char buffer[1024] = { 0 };
    int bytesRead = recv(keep, buffer, sizeof(buffer), 0);
    std::cout << buffer << std::endl;


    SOCKET toServer = Socket();
    Connect(toServer, ip, port);
    std::string guid;
    std::cin >> guid;
    std::string msg = "{\"req\": 1,\"content\": \"" + guid + "\"}\3";
    send(toServer, msg.c_str(), msg.size(), 0);
    std::cout << "Connected to server" << std::endl;

    // 创建客户端Socket2
    SOCKET toLocal = Socket();
    Connect(toLocal, "127.0.0.1", localPort);
    std::cout << "Connected to local" << std::endl;


    // 使用select函数同时管理两个Socket连接
    std::vector<SOCKET> sockets = { toServer, toLocal };
    fd_set readSet;

    while (true) {
        FD_ZERO(&readSet);

        // 添加需要监听的Socket到readSet
        for (const auto& socket : sockets) {
            FD_SET(socket, &readSet);
        }

        // 使用select函数监听可读事件
        int activity = select(0, &readSet, nullptr, nullptr, nullptr);
        if (activity == SOCKET_ERROR) {
            std::cout << "select error" << std::endl;
            break;
        }

        // 检查每个Socket，处理可读事件
        for (const auto& socket : sockets) {
            if (FD_ISSET(socket, &readSet)) {
                if (socket == toServer)
                {
                    char buffer[1024] = { 0 };
                    int bytesRead = recv(socket, buffer, sizeof(buffer), 0);
                    if (bytesRead > 0) {
                        std::cout << "Received data from server: " << buffer << std::endl;

                        // 处理数据
                        send(toLocal, buffer, strlen(buffer), 0);
                        // 这里可以根据需要进行相应的处理逻辑
                    }
                    else if (bytesRead == 0) {
                        std::cout << "Server disconnected" << std::endl;
                        closesocket(socket);
                        break;
                    }
                    else {
                        std::cout << "Failed to receive data from server" << std::endl;
                        closesocket(socket);
                        break;
                    }
                }
                else if (socket == toLocal)
                {
                    char buffer[1024] = { 0 };
                    int bytesRead = recv(socket, buffer, sizeof(buffer), 0);
                    if (bytesRead > 0) {
                        std::cout << "Received data from local: " << buffer << std::endl;

                        // 处理数据
                        send(toServer, buffer, strlen(buffer), 0);
                        // 这里可以根据需要进行相应的处理逻辑
                    }
                    else if (bytesRead == 0) {
                        std::cout << "Local disconnected" << std::endl;
                        closesocket(socket);
                        break;
                    }
                    else {
                        std::cout << "Failed to receive data from local" << std::endl;
                        closesocket(socket);
                        break;
                    }
                }
            }
        }
    }

    // 关闭客户端Socket
    closesocket(toServer);
    closesocket(toLocal);

    // 清理 Winsock
    WSACleanup();

    return 0;
}