#pragma once
#include <iostream>
#include <string>
#include <chrono>
#include <ctime>

using std::cout;
using std::endl;
using std::string;

// 日志等级
enum Level {
    INFO,
    WARNING,
    ERROR,
    FATAL,
    DEBUG
};

// 使用: Log(日志等级) << "信息"
#define Log(level) log(#level, DateTime(), __FILE__, __LINE__)

static string LogFormat(const string& str)
{
    return "[" + str + "]";
}

static string LogFormat(int num)
{
    return "[" + std::to_string(num) + "]";
}


static std::ostream& log(const string& level, const string& date_time,
                    const string& file, int line)
{
    cout << LogFormat(level) << LogFormat(date_time) << LogFormat(file) << LogFormat(line) << ": ";
    return cout;
}

static string DateTime()
{
    // 获取当前系统时间
    auto current_time_point = std::chrono::system_clock::now();
    // 将当前时间点转换为 time_t
    std::time_t current_time_t = std::chrono::system_clock::to_time_t(current_time_point);
    // 使用 localtime 将 time_t 转换为结构化时间（本地时间）
    std::tm* local_tm = std::localtime(&current_time_t);

    // 使用 strftime 格式化输出本地时间
    char time_str[32] = {0};
    std::strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", local_tm);
    return time_str;
}
