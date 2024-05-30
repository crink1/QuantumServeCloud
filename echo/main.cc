#include "EchoServer.hpp"

int main()
{
    EchoServer server(8080);
    server.Launch();

    return 0;
}