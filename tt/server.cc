#include "../Socket.hpp"
#include "../server.hpp"
#include <ctime>
#include <unistd.h>
#include <vector>

using namespace std;
std::unordered_map<uint64_t, shared_ptr<Connection>> conns;
static uint64_t id = 0;
EventLoop loop;
ThreadPool *tp;

void cc(const shared_ptr<Connection> &c)
{
    lg(Debug, "New Connected: %p", c.get());
    std::cout << "thread id: "<<this_thread::get_id() << std::endl;

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
    conns.erase(c->Id());
}

void acceptor(int newfd)
{
   
    shared_ptr<Connection> c(new Connection(tp->GetEventLoop(), id, newfd));
    c->SetConnectedCallback(bind(cc, placeholders::_1));
    c->SetMessageCallback(bind(msg, placeholders::_1, placeholders::_2));
    c->SetSerClosedCallback(bind(clos, placeholders::_1));
    std::cout << "thread id: "<<std::this_thread::get_id() << std::endl;

    c->EnableActiveRelease(10);
    c->Established();
    conns[id] = c;
    id++;
}

int main()
{
    tp = new ThreadPool(&loop);
    tp->SetMaxThreads(2);
    tp->CreateThread();
    Acceptor acpt(&loop, 8080);
    acpt.SetAcceptCallback(bind(acceptor, placeholders::_1));
    acpt.EableListen();
    
    loop.Launch();
    

    return 0;
}