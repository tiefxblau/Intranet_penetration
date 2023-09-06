#pragma once
#include <jsoncpp/json/json.h>
#include <string>

struct Request
{
public:
    int _x;
    int _y;
    char _op;
public:
    Request() = default;
    Request(int x, int y, char op)
        : _x(x)
        , _y(y)
        , _op(op) {}

};

std::string SerializeRequest(Request& req)
{
    Json::Value root;
    root["x"] = req._x;
    root["y"] = req._y;
    root["op"] = req._op;

    Json::FastWriter writer;

    return writer.write(root);
}
Request DeserializeRequest(std::string json_string)
{
    Json::Value root;

    Json::Reader reader;
    reader.parse(json_string, root);

    Request req;
    req._x = root["x"].asInt();
    req._y = root["y"].asInt();
    req._op = root["op"].asInt();

    return req;
}

struct Response
{
public:
    int _res;
    int _code;
public:
    Response() = default;
    Response(int res, int code)
        : _res(res)
        , _code(code) {}
};

std::string SerializeResponse(Response& resp)
{
    Json::Value root;
    root["res"] = resp._res;
    root["code"] = resp._code;

    Json::FastWriter writer;

    return writer.write(root);
}
Response DeserializeResponse(std::string json_string)
{
    Json::Value root;

    Json::Reader reader;
    reader.parse(json_string, root);

    Response resp;
    resp._res = root["res"].asInt();
    resp._code = root["code"].asInt();

    return resp;
}