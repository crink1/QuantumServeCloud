#include "../Socket.hpp"

using namespace std;
int main()
{
    Socket sock;
    sock.CreateClient(8080, "127.0.0.1");
    for (int i = 0; i < 5; i++)
    {
        std::string str = "hello world!";
        sock.Send(str.c_str(), str.size());
        char buffer[1024] = {0};
        cout << "send" << endl;
        sock.Recv(buffer, 1023);
        std::cout << buffer << std::endl;
        sleep(1);
    }

    while (1)
    {
        sleep(1);
    }

    return 0;
}