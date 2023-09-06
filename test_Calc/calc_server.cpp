#include <iostream>
#include <string>
#include <pthread.h>
#include <unistd.h>
#include "Socket.hpp"
#include "Protocol.hpp"

void Usage(std::string proc)
{
    std::cout << "Usage:\n\t" << proc << " server_port" << std::endl;
}
void* Calc(void* args)
{
    pthread_detach(pthread_self());

    int sockfd = *(int*)args;
    delete (int*)args;

    char buffer[1024] = { 0 };
    ssize_t sz = read(sockfd, buffer, sizeof(buffer) - 1);
    if (sz > 0)
    {
        buffer[sz] = 0;
        std::string json_req = buffer;
        std::cout << "get request: " << json_req << std::endl;
        Request req = DeserializeRequest(json_req);

        Response resp = {0, 0};
        switch (req._op)
        {
        case '+':
            resp._res = req._x + req._y;
            break;
        case '-':
            resp._res = req._x - req._y;
            break;
        case '*':
            resp._res = req._x * req._y;
            break;
        case '/':
            if (req._y == 0)
                resp._code = 1;
            else
                resp._res = req._x / req._y;
            break;
        case '%':
            if (req._y == 0)
                resp._code = 2;
            else
                resp._res = req._x % req._y;
            break;
        default:
            resp._code = 3;
            break;
        }

        std::string json_resp = SerializeResponse(resp);
        write(sockfd, json_resp.c_str(), json_resp.size());
    }

    close(sockfd);
    return nullptr;
}

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        Usage(argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);

    int listen_sock = Sock::Socket();
    Sock::Bind(listen_sock, port);
    Sock::Listen(listen_sock);

    while (true)
    {
        int sockfd = Sock::Accept(listen_sock);
        int* pram = new int(sockfd);

        pthread_t tid;
        pthread_create(&tid, nullptr, Calc, pram);
    }

    return 0;
}