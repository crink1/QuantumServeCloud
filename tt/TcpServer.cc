#include "../server.hpp"
using namespace std;
void cc(const shared_ptr<Connection> &c)
{
    lg(Debug, "New Connected: %p", c.get());
    std::cout << "thread id: " << this_thread::get_id() << std::endl;
}
void msg(const shared_ptr<Connection> &c, Buffer *buf)
{
    lg(Debug, "client say: %s", buf->ReadPos());

    buf->MoveReadOffset(buf->ReadableSize());
    string str = "hello world!!";
    c->Send(str.c_str(), str.size());
    //c->ShutDown();
}
void clos(const shared_ptr<Connection> &c)
{
    lg(Debug, "Close Connected: %p", c.get());
    std::cout << "thread id: " << this_thread::get_id() << std::endl;
}

int main()
{
    TcpServer ser(8080);
    ser.SetMaxThreads(2);
    //ser.EnableActiveRelease(10);
    ser.SetMessageCallback(msg);
    ser.SetConnectedCallback(cc);
    ser.SetClosedCallback(clos);
    ser.Launch();
    return 0;
}