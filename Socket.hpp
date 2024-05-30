#pragma once

#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "Log.hpp"

enum
{
    SocketError = 2,
    BindError,
    ListenError,
};

const int max_listen = 10;

class Socket
{
public:
    Socket()
        : _sockfd(-1)
    {
    }

    Socket(int fd)
        : _sockfd(fd)
    {
    }

    ~Socket()
    {
        Close();
    }

public:
    // 创建套接字
    bool Initsocket()
    {
        _sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (_sockfd < 0)
        {
            lg(Error, "socket error, %s: %d", strerror(errno), errno);
            return false;
        }
        return true;
    }

    bool Bind(const std::string &ip, uint16_t port)
    {
        struct sockaddr_in local;
        memset(&local, 0, sizeof(local));
        local.sin_family = AF_INET;
        local.sin_port = htons(port);
        local.sin_addr.s_addr = inet_addr(ip.c_str()); // INADDR_ANY

        if (bind(_sockfd, (struct sockaddr *)&local, sizeof(local)) < 0)
        {
            lg(Error, "bind error, %s: %d", strerror(errno), errno);
            return false;
        }
        return true;
    }

    bool Listen(int backlog = max_listen)
    {
        if (listen(_sockfd, backlog) < 0)
        {
            lg(Error, "listen error, %s: %d", strerror(errno), errno);
            return false;
        }
        return true;
    }

    int Accept(std::string *outip, uint16_t *outport)
    {
        struct sockaddr_in c;
        socklen_t len = sizeof(c);

        int newfd = accept(_sockfd, (struct sockaddr *)&c, &len);
        if (newfd < 0)
        {
            lg(Warning, "accept error, %s: %d", strerror(errno), errno);
            return -1;
        }

        char ip[64];
        inet_ntop(AF_INET, &c.sin_addr, ip, sizeof(ip));

        *outip = ip;
        *outport = ntohs(c.sin_port);
        return newfd;
    }

    bool Connect(const std::string &ip, const uint16_t &port)
    {
        struct sockaddr_in c;
        memset(&c, 0, sizeof(c));
        c.sin_family = AF_INET;
        c.sin_port = htons(port);
        inet_pton(AF_INET, ip.c_str(), &(c.sin_addr));

        int n = connect(_sockfd, (struct sockaddr *)&c, sizeof(c));
        if (n < 0)
        {
            // std::cerr << "connect to " << ip << ":" << port << " error" << std::endl;
            lg(Error, "connect to %s:%d error", ip.c_str(), port);
            return false;
        }
        return true;
    }

    // 接受数据
    ssize_t Recv(void *buf, size_t len, int flag = 0)
    {
        ssize_t ret = recv(_sockfd, buf, len, flag);
        if (ret <= 0)
        {
            if (errno == EAGAIN || errno == EINTR)
            {
                return 0;
            }
            lg(Error, "socket recv failed !!");
            return -1;
        }
        return ret;
    }

    ssize_t NonBlockRecv(void *buf, size_t len)
    {
        return Recv(buf, len, MSG_DONTWAIT);
    }

    // 发送数据
    ssize_t Send(const void *buf, size_t len, int flag = 0)
    {
        ssize_t ret = send(_sockfd, buf, len, flag);
        if (ret <= 0)
        {

            lg(Error, "socket send failed !!");
            return -1;
        }
        return ret;
    }

    ssize_t NonBlockSend(void *buf, size_t len)
    {
        return Send(buf, len, MSG_DONTWAIT);
    }

    // 建立一个服务端连接
    bool CreateServer(const uint16_t &port, const std::string &ip = "0.0.0.0", bool block_flag = false)
    {
        if (false == Initsocket())
        {
            return false;
        }

        if (block_flag)
        {
            NonBlock();
        }

        if (false == Bind(ip, port))
        {
            return false;
        }

        if (false == Listen())
        {
            return false;
        }

        ReuseAddress();
        return true;
    }
    // 建立一个客户端连接
    bool CreateClient(const uint16_t &port, const std::string &ip)
    {
        if (false == Initsocket())
        {
            return true;
        }

        if (false == Connect(ip, port))
        {
            return false;
        }
        return true;
    }

    // 设置套接字地址重用
    void ReuseAddress()
    {
        int val = 1;
        setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, (void *)&val, sizeof(val));
        val = 1;
        setsockopt(_sockfd, SOL_SOCKET, SO_REUSEPORT, (void *)&val, sizeof(val));
    }
    // 设置套接字为非阻塞
    void NonBlock()
    {
        int fl = fcntl(_sockfd, F_GETFL, 0);
        if (fl < 0)
        {
            lg(Error, "get fcntl failed!!");
            return;
        }
        fcntl(_sockfd, F_SETFL, fl | O_NONBLOCK);
    }

    void Close()
    {
        if (_sockfd != -1)
        {
            close(_sockfd);
            _sockfd = -1;
        }
    }

    int Fd()
    {
        return _sockfd;
    }

private:
    int _sockfd;
};