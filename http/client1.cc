#include "http.hpp"
using namespace std;

void test1()
{
    Socket sock;
    sock.CreateClient(8080, "127.0.0.1");
    string str = "GET /hello HTTP/1.1\r\nConnection: keep-alive\r\nContent-Length: 100\r\n\r\nwuhuqifei";
    while (1)
    {
        assert(sock.Send(str.c_str(), str.size()) != -1);
        char buffer[1024] = {0};
        assert(sock.Recv(buffer, 1023));
        lg(Debug, "[%s]", buffer);
        sleep(3);
    }
    sock.Close();
}

void test2()
{
    Socket sock;
    sock.CreateClient(8080, "127.0.0.1");
    string str = "GET /hello HTTP/1.1\r\nConnection: keep-alive\r\nContent-Length: 100\r\n\r\nwuhuqifei";
    while (1)
    {
        assert(sock.Send(str.c_str(), str.size()) != -1);
        assert(sock.Send(str.c_str(), str.size()) != -1);
        assert(sock.Send(str.c_str(), str.size()) != -1);
        assert(sock.Send(str.c_str(), str.size()) != -1);

        char buffer[1024] = {0};
        assert(sock.Recv(buffer, 1023));
        lg(Debug, "[%s]", buffer);
        sleep(15);
    }
    sock.Close();
}

void test3()
{
    for (int i = 0; i < 10; i++)
    {
        pid_t pid = fork();
        if (pid < 0)
        {
            return;
        }
        else if (pid == 0)
        {
            Socket sock;
            sock.CreateClient(8080, "127.0.0.1");
            string str = "GET /hello HTTP/1.1\r\nConnection: keep-alive\r\nContent-Length: 0\r\n\r\n";
            while (1)
            {
                assert(sock.Send(str.c_str(), str.size()) != -1);
                char buffer[1024] = {0};
                assert(sock.Recv(buffer, 1023));
                lg(Debug, "[%s]", buffer);
            }
            sock.Close();
            exit(0);
        }
    }
}

void test4()
{
    Socket sock;
    sock.CreateClient(8080, "127.0.0.1");
    string str = "GET /hello HTTP/1.1\r\nConnection: keep-alive\r\nContent-Length: 0\r\n\r\n";
    str += "GET /hello HTTP/1.1\r\nConnection: keep-alive\r\nContent-Length: 0\r\n\r\n";
    str += "GET /hello HTTP/1.1\r\nConnection: keep-alive\r\nContent-Length: 0\r\n\r\n";
    while (1)
    {
        assert(sock.Send(str.c_str(), str.size()) != -1);
        char buffer[1024] = {0};
        assert(sock.Recv(buffer, 1023));
        lg(Debug, "[%s]", buffer);
        sleep(3);
    }
    sock.Close();
}

void test5()
{
    Socket sock;
    sock.CreateClient(8080, "127.0.0.1");
    string str = "PUT /1234.txt HTTP/1.1\r\nConnection: keep-alive\r\n";
    string body;
    Util::ReadFile("./hello.txt", &body);
    str += "Content-Length: " + to_string(body.size()) + "\r\n\r\n";

    assert(sock.Send(str.c_str(), str.size()) != -1);
    assert(sock.Send(body.c_str(), body.size()) != -1);

    char buffer[1024] = {0};
    assert(sock.Recv(buffer, 1023));
    lg(Debug, "[%s]", buffer);
    sleep(10);

    sock.Close();
    //exit(0);
}

int main()
{
    signal(SIGCHLD, SIG_IGN);
    // test1();
    // test2();
     test3();
    // test4();
    //test5();
    // sleep(100);

    return 0;
}