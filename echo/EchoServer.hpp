#pragma once
#include "../server.hpp"
class EchoServer
{
private:
    void cc(const std::shared_ptr<Connection> &c)
    {
        lg(Debug, "New Connected: %p", c.get());
        std::cout << "thread id: " << std::this_thread::get_id() << std::endl;
    }
    void msg(const std::shared_ptr<Connection> &c, Buffer *buf)
    {
        lg(Debug, "client say: %s", buf->ReadPos());

        c->Send(buf->ReadPos(), buf->ReadableSize());
        buf->MoveReadOffset(buf->ReadableSize());
        c->ShutDown();
    }
    void clos(const std::shared_ptr<Connection> &c)
    {
        lg(Debug, "Close Connected: %p", c.get());
        std::cout << "thread id: " << std::this_thread::get_id() << std::endl;
    }

public:
    EchoServer(uint16_t port)
        : _server(port)
    {
        _server.SetMaxThreads(2);
        _server.EnableActiveRelease(10);
        _server.SetMessageCallback(std::bind(&EchoServer::msg, this, std::placeholders::_1, std::placeholders::_2));
        _server.SetConnectedCallback(std::bind(&EchoServer::cc, this, std::placeholders::_1));
        _server.SetClosedCallback(std::bind(&EchoServer::clos, this, std::placeholders::_1));
    }

    void Launch()
    {
        _server.Launch();
    }

private:
    TcpServer _server;
};