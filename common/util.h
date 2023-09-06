#pragma once
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <chrono>
#include "log.h"

// ReadInfo单次读取的数量
constexpr int kReadNum = 1024;

class FileUtil {
public:
    // 设置fd为非阻塞
    static void SetNoBlock(int fd)
    {
        int fl = fcntl(fd, F_GETFL);
        if (fl < 0)
        {
            Log(FATAL) << "fcntl getfl error" << std::endl;
            assert(false);
        }

        if (fcntl(fd, F_SETFL, fl | O_NONBLOCK) < 0)
        {
            Log(FATAL) << "fcntl setfl error" << std::endl;
            assert(false);
        }
    }


    // 从fd对应文件中读取信息
    // return value:
    // 0: 正常  -1: 对端退出  -2: 错误
    static int ReadInfo(string& buffer, int fd)
    {
        while (true)
        {
            char tmpBuffer[1024];
            ssize_t sz = read(fd, tmpBuffer, kReadNum - 1);
            if (sz > 0) //  成功读取到sz个字符
            {
                tmpBuffer[sz] = 0;
                buffer += tmpBuffer;
            }
            else if (sz == 0) // 对端退出
            {
                if (buffer.empty())
                    return -1;
                else
                    return 0;
            }
            else
            {
                if (errno == EINTR) continue; // 被信号中断
                if (errno == EAGAIN || errno == EWOULDBLOCK) break; // 底层无数据了

                return -2; // 错误
            }
        }

        return 0;
    }

    // 返回值 -2: 失败  -1: 未完全发送,但无数据可写  0: 成功
    static int SendInfo(string& buffer, int fd)
    {
        while (true)
        {
            int cur = 0;
            const char* start = buffer.c_str();
            int len = buffer.size();

            int n = write(fd, start + cur, len - cur);
            if (n > 0)
            {
                cur += n;
                if (cur == len)
                {
                    buffer.clear();
                    return 0;
                }
            }
            else
            {
                // 被中断
                if (errno == EINTR) continue;
                // 当前无法写入数据，需要等待套接字变为可写状态
                if (errno == EAGAIN || errno == EWOULDBLOCK) 
                {
                    buffer.erase(0, cur);
                    return -1;
                }

                return -2;
            }
        }
    }
};

class StrUtil {
public:
    // 一直读取msg直到遇到sep分隔符, 并在msg中移除已读取的数据
    static bool SplitRead(string& msg, string& out, char sep)
    {
        int end = msg.size();
        for (int i = 0; i < msg.size(); ++i)
        {
            if (msg[i] == sep)
            {
                end = i;
                out = msg.substr(0, end);
                msg.erase(0, end + 1);
                return true;
            }
        }

        return false;
    }
};

class TimeUtil {
public:
    // 返回当前时间(ms)
    static int64_t Now()
    {
        auto now = std::chrono::system_clock::now();
        auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
        auto epoch = now_ms.time_since_epoch();
        return std::chrono::duration_cast<std::chrono::milliseconds>(epoch).count();
    }

    // 返回当前时间戳
    static int TimeStamp()
    {
        return std::time(nullptr);
    }

    // 返回当前日期与时间
    static string DateTime()
    {
        return DateTime();
    }
};
