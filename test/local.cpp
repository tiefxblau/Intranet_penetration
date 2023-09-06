#include <iostream>
#include <WinSock2.h>

#pragma comment(lib, "ws2_32.lib")

int main() {
    // 初始化 Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cout << "Failed to initialize winsock" << std::endl;
        return 1;
    }

    // 创建服务器端Socket
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        std::cout << "Failed to create server socket" << std::endl;
        WSACleanup();
        return 1;
    }

    // 绑定服务器地址和端口号
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8888);
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    if (bind(serverSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cout << "Failed to bind server socket" << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    // 开始监听
    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cout << "Failed to listen on server socket" << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Server started, waiting for client connections..." << std::endl;

    while (true) {
        // 等待客户端连接
        SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET) {
            std::cout << "Failed to accept client connection" << std::endl;
            closesocket(serverSocket);
            WSACleanup();
            return 1;
        }

        std::cout << "Client connected" << std::endl;

        while (true)
        {
            // 接收客户端发送的数据
            char buffer[1024] = { 0 };
            int bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
            if (bytesRead > 0) {
                std::cout << "Received data from client: " << buffer << std::endl;

                // 发送响应给客户端
                const char* response = "Hello, client!\n";
                send(clientSocket, response, strlen(response), 0);
            }
        }

        // 关闭连接
        closesocket(clientSocket);
        std::cout << "Client disconnected" << std::endl;
    }

    // 关闭服务器端Socket
    closesocket(serverSocket);

    // 清理 Winsock
    WSACleanup();

    return 0;
}