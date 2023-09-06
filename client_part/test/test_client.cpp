#include "../../common/sock.h"
#include "../../common/common.h"

const string ip = "101.43.183.8";
constexpr int port = 8081;

int main()
{
    int x = 0;
    int fd = Sock::Socket();
    Sock::Connect(fd, ip, port);
    string msg = "{\"req\": 0,\"content\": \"101.43.183.8:8083\"}\3";
    send(fd, msg.c_str(), msg.size(), 0);
    FileUtil::SetNoBlock(fd);
    string inbuffer;

    string guid;
    int new_fd = -1;


    while (x != -1)
    {
        FileUtil::ReadInfo(inbuffer, fd);
        std::cout << "[inbuffer] " << inbuffer << endl;
        inbuffer.clear();

        std::cin >> x;
        switch (x)
        {
        case 1:
            msg = "{\"req\": 2,\"content\": \"\"}\3";
            send(fd, msg.c_str(), msg.size(), 0);
            break;
        case 0:
            break;
        case -1:
            msg = "{\"req\": 3,\"content\": \"101.43.183.8:8083\"}\3";
            send(fd, msg.c_str(), msg.size(), 0);
            break;

        case 2:
        {
            std::cin >> guid;
            new_fd = Sock::Socket();
            Sock::Connect(new_fd, ip, port);
            string new_msg = "{\"req\": 1,\"content\": \"" + guid + "\"}\3";
            send(new_fd, new_msg.c_str(), new_msg.size(), 0);
        }
            break;
        case 3:
        {
            string sendMsg;
            std::cin >> sendMsg;
            send(new_fd, sendMsg.c_str(), msg.size(), 0);
        }
            break;
        default:
            break;
        }
    }

    return 0;
}