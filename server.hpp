#pragma once
#include <iostream>
#include <sys/epoll.h>
#include <vector>
#include <cassert>
#include <cstring>
#include <functional>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include <signal.h>
#include <typeinfo>
#include "Log.hpp"
#include "Socket.hpp"

#define default_capacity 1024

class Buffer
{
private:
    std::vector<char> _buffer; // 使用vector进行缓冲区管理
    uint64_t _reader_index;    // 读偏移
    uint64_t _writer_index;    // 写偏移

public:
    Buffer()
        : _buffer(default_capacity), _reader_index(0), _writer_index(0)
    {
    }

    char *begin()
    {
        return &*_buffer.begin();
    }

    // 获取读取起始地址
    char *ReadPos()
    {
        return begin() + _reader_index;
    }

    // 获取写入起始地址
    char *WritePos()
    {
        return begin() + _writer_index;
    }

    // 获取前部空闲空间大小
    uint64_t FrontIdleCapacity()
    {
        return _reader_index;
    }

    // 获取后部空闲空间大小
    uint64_t TailIdleCapacity()
    {
        return _buffer.size() - _writer_index;
    }

    // 获取可读数据大小
    uint64_t ReadableSize()
    {
        return _writer_index - _reader_index;
    }

    // 使读偏移向后移动
    void MoveReadOffset(uint64_t len)
    {
        assert(len <= ReadableSize());
        _reader_index += len;
    }

    // 使写偏移向后移动
    void MoveWriteOffset(uint64_t len)
    {
        assert(len <= TailIdleCapacity());
        _writer_index += len;
    }

    // 保证可写空间足够，不够就扩容
    void EnsureWriteCapacity(uint64_t len)
    {
        if (len <= TailIdleCapacity())
        {
            return;
        }
        // 前后空间加起来够就把缓冲区copy到最前面
        if (len <= TailIdleCapacity() + FrontIdleCapacity())
        {
            uint64_t sz = ReadableSize();
            std::copy(ReadPos(), ReadPos() + sz, begin());
            _reader_index = 0;
            _writer_index = sz;
        }
        else
        {
            _buffer.resize(_writer_index + len);
        }
    }

    // 读取数据
    void Read(void *buffer, uint64_t len)
    {
        assert(len <= ReadableSize());
        std::copy(ReadPos(), ReadPos() + len, (char *)buffer);
    }

    void ReadAndPop(void *buffer, uint64_t len)
    {
        Read(buffer, len);
        MoveReadOffset(len);
    }

    std::string ReadString(uint64_t len)
    {
        assert(len <= ReadableSize());
        std::string str;
        str.resize(len);
        Read(&str[0], len);
        return str;
    }

    std::string ReadStringAndPop(uint64_t len)
    {
        assert(len <= ReadableSize());
        std::string str = ReadString(len);
        MoveReadOffset(len);
        return str;
    }

    // 写入数据
    void Write(const void *data, uint64_t len)
    {
        EnsureWriteCapacity(len);
        const char *d = (const char *)data;
        std::copy(d, d + len, WritePos());
    }

    void WriteString(const std::string &data)
    {
        return Write(data.c_str(), data.size());
    }

    void WriteAndPush(const void *data, uint64_t len)
    {
        Write(data, len);
        MoveWriteOffset(len);
    }

    void WriteStringAndPush(const std::string &data)
    {
        WriteString(data);
        MoveWriteOffset(data.size());
    }

    void WriteBuffer(Buffer &data)
    {

        return Write(data.ReadPos(), data.ReadableSize());
    }

    void WriteBufferAndPush(Buffer &data)
    {
        WriteBuffer(data);
        MoveWriteOffset(data.ReadableSize());
    }

    char *FindCRLF()
    {
        char *res = (char *)memchr(ReadPos(), '\n', ReadableSize());
        return res;
    }

    std::string GetLine()
    {
        char *pos = FindCRLF();
        if (NULL == pos)
        {
            return "";
        }
        return ReadString(pos - ReadPos() + 1);
    }

    std::string GetLineAndPop()
    {
        std::string str = GetLine();
        MoveReadOffset(str.size());
        return str;
    }

    // 清空缓冲区
    void Clear()
    {
        _reader_index = 0;
        _writer_index = 0;
    }
};

class Poller;
class EventLoop;
class Channel
{

private:
    int _fd;
    EventLoop *_loop;
    uint32_t _events;  // 要监控的事件
    uint32_t _revents; // 被触发的事件
    using EventCallback = std::function<void()>;
    EventCallback _read_callback;  // 读事件被触发
    EventCallback _write_callback; // 写事件被触发
    EventCallback _close_callback; // 关闭时间被触发
    EventCallback _error_callback; // 错误事件被触发
    EventCallback _event_callback; // 任何时间被触发

public:
    Channel(EventLoop *loop, int fd)
        : _fd(fd), _events(0), _revents(0), _loop(loop)
    {
    }
    int Fd()
    {
        return _fd;
    }

    uint32_t Events()
    {
        return _events;
    }

    void SetREvent(uint32_t events)
    {
        _revents = events;
    }

    void SetReadCallback(const EventCallback &cb)
    {
        _read_callback = cb;
    }
    void SetWriteCallback(const EventCallback &cb)
    {
        _write_callback = cb;
    }
    void SetErrorCallback(const EventCallback &cb)
    {
        _error_callback = cb;
    }
    void SetCloseCallback(const EventCallback &cb)
    {
        _close_callback = cb;
    }
    void SetEventCallback(const EventCallback &cb)
    {
        _event_callback = cb;
    }

    // 是否开启监控可读事件
    bool Readable()
    {
        return (_events & EPOLLIN);
    }
    // 是否开启监控可写事件
    bool Writeable()
    {
        return (_events & EPOLLOUT);
    }

    void EnableRead()
    {
        _events |= EPOLLIN;
        Update();
    }

    void EnableWrite()
    {
        _events |= EPOLLOUT;
        Update();
    }

    void DisableRRead()
    {
        _events &= ~EPOLLIN;
        Update();
    }

    void DisableWrite()
    {
        _events &= ~EPOLLOUT;
        Update();
    }

    // 关闭所有监控
    void DisableAll()
    {
        _events = 0;
        Update();
    }

    // 移除监控
    void Remove();

    void Update();

    // 对监控事件进行处理
    void HandleEvent()
    {
        if ((_revents & EPOLLIN) || (_revents & EPOLLRDHUP) || (_revents & EPOLLPRI))
        {

            if (_read_callback)
            {
                _read_callback();
            }
        }

        if (_revents & EPOLLOUT)
        {
            

            if (_write_callback)
            {
                _write_callback();
            }
        }

        if (_revents & EPOLLERR)
        {
            

            if (_error_callback)
            {
                _error_callback();
            }
        }

        if (_revents & EPOLLHUP)
        {

            

            if (_close_callback)
            {
                _close_callback();
            }
        }
        if (_event_callback)
        {
            _event_callback();
        }
    }
};

#define MAX_EPOLLEVENTS 1024
class Poller
{
private:
    int _epfd;
    struct epoll_event _evs[MAX_EPOLLEVENTS];
    std::unordered_map<int, Channel *> _channels;

private:
    // 更新epoll
    void Update(Channel *channel, int opt)
    {
        int fd = channel->Fd();
        struct epoll_event ev;
        ev.data.fd = fd;
        ev.events = channel->Events();
        int ret = epoll_ctl(_epfd, opt, fd, &ev);
        if (ret < 0)
        {
            lg(Error, "EPOLL_CTL FAILED!");
        }
        return;
    }
    // 判断Channel是否已经添加了事件
    bool HasChannel(Channel *channel)
    {
        auto it = _channels.find(channel->Fd());
        if (it == _channels.end())
        {
            return false;
        }
        return true;
    }

public:
    Poller()
    {
        _epfd = epoll_create(MAX_EPOLLEVENTS);
        if (_epfd < 0)
        {
            lg(Fatal, "EPOLL CREATE FAILED");
            abort();
        }
    }

    // 添加或者修改监控事件
    void UpdateEvent(Channel *channel)
    {
        bool ret = HasChannel(channel);
        if (ret)
        {
            return Update(channel, EPOLL_CTL_MOD);
        }
        else
        {
            _channels[channel->Fd()] = channel;
            return Update(channel, EPOLL_CTL_ADD);
        }
    }

    // 移除监控
    void RemoveEvent(Channel *channel)
    {
        auto it = _channels.find(channel->Fd());
        if (it != _channels.end())
        {
            _channels.erase(it);
        }
        Update(channel, EPOLL_CTL_DEL);
    }

    // 开始监控，返回活跃连接
    void Poll(std::vector<Channel *> *active)
    {
        int nfds = epoll_wait(_epfd, _evs, MAX_EPOLLEVENTS, -1);
        if (nfds < 0)
        {
            if (errno == EINTR)
            {
                return;
            }
            lg(Error, "EPOLL_WAIT ERROR:%s\n", strerror(errno));
            abort();
        }
        for (int i = 0; i < nfds; i++)
        {
            auto it = _channels.find(_evs[i].data.fd);
            assert(it != _channels.end());
            it->second->SetREvent(_evs[i].events);
            active->push_back(it->second);
        }
        return;
    }
};

using TaskFunc = std::function<void()>;
using ReleaseFunc = std::function<void()>;

class TimerTask
{
public:
    TimerTask(uint64_t id, uint32_t timeout, const TaskFunc task_cb)
        : _id(id), _timeout(timeout), _task_cb(task_cb), _taskcancel(false)
    {
    }

    ~TimerTask()
    {
        if (_taskcancel == false)
        {
            _task_cb();
        }
        _release_cb();
    }

    void SetRelease(const ReleaseFunc &release_cb)
    {
        _release_cb = release_cb;
    }

    uint32_t get_timeout()
    {
        return _timeout;
    }
    void cancel()
    {
        _taskcancel = true;
    }

private:
    uint64_t _id;            // 定时器任务对象id
    uint32_t _timeout;       // 定时任务的超时时间
    TaskFunc _task_cb;       // 定时任务回调函数
    bool _taskcancel;        // 定时任务是否取消
    ReleaseFunc _release_cb; // 定时任务释放回调函数
};

class Timerwheel
{
private:
    static int CreateTimerFd()
    {
        int timefd = timerfd_create(CLOCK_MONOTONIC, 0);
        if (timefd < 0)
        {
            lg(Error, "Timerfd create failed! err: %s", strerror(errno));
            abort();
        }
        // int timerfd_settime(int fd, int flags, struct itimerspec *new, struct itimerspec *old);
        static itimerspec itime;
        itime.it_value.tv_sec = 1;
        itime.it_value.tv_nsec = 0; // 第一次超时时间
        itime.it_interval.tv_sec = 1;
        itime.it_interval.tv_nsec = 0; // 第一次之后，每次的超时时间
        timerfd_settime(timefd, 0, &itime, NULL);
        return timefd;
    }

    int ReadTimefd()
    {
        uint64_t times;
        int res = read(_timerfd, &times, 8); // 超时次数
        if (res < 0)
        {
            lg(Error, "Read timerfd failed! err: %s", strerror(errno));
            abort();
        }
        return times;
    }

    void OnTime()
    {
        // 超时几次就执行几次任务
        int times = ReadTimefd();
        for (int i = 0; i < times; i++)
        {
            RunTimeWheel();
        }
    }

    // 时间轮应该每秒被触发一次
    void RunTimeWheel()
    {
        _tick = (_tick + 1) % _capacity;
        _wheel[_tick].clear();
    }

    void TimerAddLoop(uint64_t id, uint32_t timeout, const TaskFunc &task_cb)
    {
        std::shared_ptr<TimerTask> sp(new TimerTask(id, timeout, task_cb));
        sp->SetRelease(std::bind(&Timerwheel::removetime, this, id));
        _timers[id] = std::weak_ptr<TimerTask>(sp);
        int pos = (_tick + timeout) % _capacity;
        _wheel[pos].push_back(sp);
    }

    void TimeRefalshInLoop(uint64_t id)
    {
        auto it = _timers.find(id);
        if (it == _timers.end())
        {
            return;
        }

        // 获取shared_ptr对象
        std::shared_ptr<TimerTask> sp = it->second.lock();
        int timeout = sp->get_timeout();
        int pos = (_tick + timeout) % _capacity;
        // 添加到时间轮询
        _wheel[pos].push_back(sp);
    }

    void TaskCancelInLoop(uint64_t id)
    {
        auto it = _timers.find(id);
        if (it != _timers.end())
        {
            std::shared_ptr<TimerTask> sp = it->second.lock();
            if (sp)
            {
                sp->cancel();
            }
        }
    }

public:
    Timerwheel(EventLoop *loop)
        : _capacity(60), _tick(0), _wheel(_capacity), _loop(loop), _timerfd(CreateTimerFd()), _timer_channel(new Channel(_loop, _timerfd))
    {
        _timer_channel->SetReadCallback(std::bind(&Timerwheel::OnTime, this));
        _timer_channel->EnableRead();
    }
    void TimerAdd(uint64_t id, uint32_t timeout, const TaskFunc &task_cb);

    void TimeRefalsh(uint64_t id);

    void TaskCancel(uint64_t id);

    bool HasTimer(uint64_t id)
    {
        auto it = _timers.find(id);
        if (it == _timers.end())
        {
            return false;
        }

        return true;
    }

private:
    void removetime(uint64_t id)
    {
        auto it = _timers.find(id);
        if (it != _timers.end())
        {
            _timers.erase(it);
        }
    }

private:
    int _tick;     // 秒针用于释放对象来执行任务
    int _capacity; // 时间队列的最大容量
    std::vector<std::vector<std::shared_ptr<TimerTask>>> _wheel;
    std::unordered_map<uint64_t, std::weak_ptr<TimerTask>> _timers;

    EventLoop *_loop;
    int _timerfd; // 定时器文件描述符，可读事件是获取当前计数器，来执行定时任务
    std::unique_ptr<Channel> _timer_channel;
};

class EventLoop
{
private:
    using Functor = std::function<void()>;
    Poller _poller;              // 监控
    int _event_fd;               // eventfd唤醒事件可能导致的阻塞
    std::thread::id _thread_id;  // 线程ID
    std::vector<Functor> _tasks; // 线程池
    std::mutex _mutex;
    std::unique_ptr<Channel> _event_channel;
    Timerwheel _time_wheel;

public:
    void RunAllTask()
    {
        std::vector<Functor> functor;
        {
            std::unique_lock<std::mutex> _lock(_mutex);
            _tasks.swap(functor);
        }
        for (auto &f : functor)
        {
            f();
        }
        return;
    }
    static int CreateEvent()
    {

        int efd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
        if (efd < 0)
        {
            lg(Error, "cteate eventfd error");
            abort();
        }
        return efd;
    }
    void ReadEventFd()
    {
        uint64_t ret = 0;
        ssize_t n = read(_event_fd, &ret, sizeof(ret));
        if (n < 0)
        {
            if (errno == EINTR || errno == EAGAIN)
            {
                return;
            }
            lg(Error, "read eventfd error");
            abort();
        }
        return;
    }
    void WeakUpEventFd()
    {
        uint64_t ret = 0;
        ssize_t n = write(_event_fd, &ret, sizeof(ret));
        if (n < 0)
        {
            if (errno == EINTR)
            {
                return;
            }
            lg(Error, "write eventfd error");
            abort();
        }
        return;
    }

public:
    EventLoop()
        : _thread_id(std::this_thread::get_id()), _event_fd(CreateEvent()), _event_channel(new Channel(this, _event_fd)), _time_wheel(this)
    {
        _event_channel->SetReadCallback(std::bind(&EventLoop::ReadEventFd, this));
        _event_channel->EnableRead();
    }

    // 判断当前线程是否是EventLoop对应的线程
    bool IsInLoop()
    {
        return (_thread_id == std::this_thread::get_id());
    }
    void AssertInLoop()
    {
        assert(_thread_id == std::this_thread::get_id());
    }

    void QueueInLoop(const Functor &cb) // 加入队列并唤醒epoll
    {
        {
            std::unique_lock<std::mutex> _lock(_mutex);
            _tasks.push_back(cb);
        }
        WeakUpEventFd();
    }

    // 判断要执行的任务是否在当前线程中，如果是就执行，不是则入队列。
    void RunInLoop(const Functor &cb)
    {
        if (IsInLoop())
        {
            cb();
        }
        else
        {
            QueueInLoop(cb);
        }
    }

    void UpdateEvent(Channel *channel) // 添加或者修改描述符的事件监控
    {
        return (_poller.UpdateEvent(channel));
    }
    void RemoveEvent(Channel *channel) // 移除描述符的事件监控
    {
        return (_poller.RemoveEvent(channel));
    }
    void Launch() // 开启事件循环--监控--处理就绪事件--执行任务
    {
        while (1)
        {
            std::vector<Channel *> actives;
            _poller.Poll(&actives);

            for (auto &c : actives)
            {
                c->HandleEvent();
            }

            RunAllTask();
        }
    }

    void TimerAdd(uint64_t id, uint32_t timeout, const TaskFunc &task_cb)
    {
        return _time_wheel.TimerAdd(id, timeout, task_cb);
    }

    void TimerRefresh(uint64_t id)
    {
        return _time_wheel.TimeRefalsh(id);
    }

    void TimerCancel(uint64_t id)
    {
        return _time_wheel.TaskCancel(id);
    }

    bool HasTimer(uint64_t id)
    {
        return _time_wheel.HasTimer(id);
    }
};

class ThreadLoop
{
public:
    // 线程入口
    ThreadLoop()
        : _loop(nullptr), _thread(std::thread(&ThreadLoop::ThreadEntry, this))
    {
    }
    EventLoop *GetLoop()
    {
        EventLoop *loop = nullptr;
        {
            std::unique_lock<std::mutex> _lock(_mutex);
            _cond.wait(_lock, [&]()
                       { return _loop != nullptr; });
            loop = _loop;
        }
        return loop;
    }

private:
    // 同步loop避免在loop实例化之前去获取loop
    std::mutex _mutex;             // 互斥锁
    std::condition_variable _cond; // 条件变量
    EventLoop *_loop;              // 在线程内初始化的eventloop指针
    std::thread _thread;           // loop对应的线程
private:
    // 实例化loop开始运行loop里面的模块
    void ThreadEntry()
    {
        EventLoop loop;
        {
            std::unique_lock<std::mutex> _lock(_mutex);
            _loop = &loop;
            _cond.notify_all();
        }
        loop.Launch();
    }
};

class ThreadPool
{

public:
    ThreadPool(EventLoop *loop)
        : _max_threads(0), _thread_id(0), _base_loop(loop)
    {
    }

    void SetMaxThreads(int max)
    {
        _max_threads = max;
    }

    void CreateThread()
    {
        if (_max_threads > 0)
        {
            _threads.resize(_max_threads);
            _loops.resize(_max_threads);
            for (int i = 0; i < _max_threads; i++)
            {
                _threads[i] = new ThreadLoop();
                _loops[i] = _threads[i]->GetLoop();
            }
        }
    }

    EventLoop *GetEventLoop()
    {
        if (0 == _max_threads)
        {
            return _base_loop;
        }
        _thread_id = (_thread_id + 1) % _max_threads;
        return _loops[_thread_id];
    }

private:
    int _max_threads;
    int _thread_id;
    EventLoop *_base_loop;
    std::vector<ThreadLoop *> _threads;
    std::vector<EventLoop *> _loops;
};

class Any
{

public:
    Any()
        : _content(NULL)
    {
    }

    template <class T>
    Any(const T &val)
        : _content(new placeholder<T>(val))
    {
    }

    Any(const Any &other)
        : _content(other._content ? other._content->clone() : NULL)
    {
    }

    // 返回子类对象保存的数据指针
    template <class T>
    T *get()
    {
        // 保证类型一致
        assert(typeid(T) == _content->type());
        return &((placeholder<T> *)_content)->_val;
    }

    Any &swap(Any &other)
    {
        std::swap(_content, other._content);
        return *this;
    }

    Any &operator=(const Any &other)
    {
        Any(other).swap(*this);
        return *this;
    }

    // 构造一个容器的匿名对象，交换里面的值，匿名对象析构，自动释放旧值
    template <class T>
    Any &operator=(const T &val)
    {
        Any(val).swap(*this);
        return *this;
    }
    ~Any()
    {
        delete _content;
    }

private:
    class holder
    {
    public:
        virtual ~holder() {}
        virtual const std::type_info &type() = 0;
        virtual holder *clone() = 0;
        virtual void swap(holder &other) = 0;
    };

    template <class T>
    class placeholder : public holder
    {
    public:
        placeholder(const T &val)
            : _val(val)
        {
        }

        virtual const std::type_info &type()
        {
            return typeid(T);
        }

        // 克隆一份数据
        virtual holder *clone()
        {

            return new placeholder(_val);
        }

        virtual void swap(holder &other)
        {
            placeholder *p = dynamic_cast<placeholder *>(&other);
            std::swap(_val, p->_val);
        }

    public:
        T _val;
    };

    holder *_content;
};

enum ConnStatus
{
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    DISCONNECTING,
};

class Connection : public std::enable_shared_from_this<Connection>
{
private:
    uint64_t _conn_id;             // 连接id，同时也是定时器id
    int _sockfd;                   // 连接的文件描述符
    Socket _socket;                // 管理socket
    Channel _channel;              // sockfd的时间管理
    Buffer _in_buffer;             // 输入缓冲区
    Buffer _out_buffer;            // 输出缓冲区
    Any _context;                  // 请求连接的上下文
    ConnStatus _status;            // 连接状态
    EventLoop *_loop;              // 链接关联的loop
    bool _enable_inactive_release; // 开启超时释放

    using ConnectedCallback = std::function<void(const std::shared_ptr<Connection> &)>;
    using MessageCallback = std::function<void(const std::shared_ptr<Connection> &, Buffer *)>;
    using ClosedCallback = std::function<void(const std::shared_ptr<Connection> &)>;
    using AnyEventCallback = std::function<void(const std::shared_ptr<Connection> &)>;

    // 用户使用的回调函数
    ConnectedCallback _connected_callback;
    MessageCallback _message_callback;
    ClosedCallback _closed_callback;
    AnyEventCallback _event_callback;
    // 服务器端关闭并删除连接
    ClosedCallback _server_closed_callback;

private:
    // channel回调事件
    void HandleRead() // 触发可读事件，把socket的数据读到接受缓冲区，调用_message_callback
    {
        // 接收数据放到缓冲区
        char buffer[65536];
        ssize_t n = _socket.NonBlockRecv(buffer, 65536);
        if (n < 0)
        {
            return ShutDownInLoop();
        }

        _in_buffer.WriteAndPush(buffer, n);
        // 调用msgcallback
        if (_in_buffer.ReadableSize() > 0)
        {
            return _message_callback(shared_from_this(), &_in_buffer);
        }
    }
    void HandleWrite()
    {
        ssize_t n = _socket.NonBlockSend(_out_buffer.ReadPos(), _out_buffer.ReadableSize());
        if (n < 0)
        {

            if (_in_buffer.ReadableSize() > 0)
            {
                _message_callback(shared_from_this(), &_in_buffer);
            }
            return Release();
        }
        _out_buffer.MoveReadOffset(n); // 移动偏移量
        if (_out_buffer.ReadableSize() == 0)
        {
            _channel.DisableWrite(); // 一旦发送缓冲区有空间就会一直写事件就绪，所以要关闭可写。
            if (_status == DISCONNECTING)
            {
                return Release();
            }
        }
    }
    void HandleClose()
    {
        if (_in_buffer.ReadableSize() > 0)
        {
            _message_callback(shared_from_this(), &_in_buffer);
        }
        return Release();
    }

    void HandleError()
    {
        HandleClose();
    }

    void HandleEvent()
    {
        if (true == _enable_inactive_release)
        {
            _loop->TimerRefresh(_conn_id);
        }
        if (_event_callback)
        {
            _event_callback(shared_from_this());
        }
    }
    void EstablishedInLoop()
    {
        // 1.改变status
        assert(CONNECTING == _status);
        _status = CONNECTED;

        // 2.启动读时事件监控
        _channel.EnableRead();

        // 3.调用对应的回调函数
        if (_connected_callback)
        {
            _connected_callback(shared_from_this());
        }
    }
    void ReleaseInLoop() // 实际释放的接口
    {
        // 1.更改statusd
        _status = DISCONNECTED;
        // 2.移除事件监控
        _channel.Remove();
        // 3.关闭文件描述符
        _socket.Close();
        // 4.取消定时任务
        if (_loop->HasTimer(_conn_id))
        {
            DisActiveReleaseInLoop();
        }
        // 5.客户端关闭连接
        if (_closed_callback)
        {
            _closed_callback(shared_from_this());
        }
        // 6.服务器关闭连接
        if (_server_closed_callback)
        {
            _server_closed_callback(shared_from_this());
        }
    }

    void SendInLoop(Buffer &buf) // 伪发送数据,放到缓冲区，开启监控
    {
        if (DISCONNECTED == _status)
        {
            return;
        }
        _out_buffer.WriteBufferAndPush(buf);

        if (false == _channel.Writeable())
        {
            _channel.EnableWrite();
        }
    }
    void ShutDownInLoop() // 给用户的关闭接口
    {
        // 判断是否有数据没有处理完，没发送完
        _status = DISCONNECTING;
        if (_in_buffer.ReadableSize() > 0)
        {
            if (_message_callback)
            {
                _message_callback(shared_from_this(), &_in_buffer);
            }
        }
        if (_out_buffer.ReadableSize() > 0)
        {
            if (false == _channel.Writeable())
            {
                _channel.EnableWrite();
            }
        }
        if (_out_buffer.ReadableSize() == 0)
        {
            Release();
        }
    }

    void EnableActiveReleaseInLoop(int timeout) // 开启非活跃连接释放
    {
        // 设置标志位
        _enable_inactive_release = true;
        // 判断任务存在，刷新延迟
        if (_loop->HasTimer(_conn_id))
        {
            _loop->TimerRefresh(_conn_id);
        }
        else // 任务不存在，就新增
        {
            _loop->TimerAdd(_conn_id, timeout, std::bind(&Connection::Release, this));
        }
    }
    void DisActiveReleaseInLoop() // 关闭非活跃连接释放
    {
        _enable_inactive_release = false;
        if (_loop->HasTimer(_conn_id))
        {
            _loop->TimerCancel(_conn_id);
        }
    }
    void UpgrdeInLoop(const Any &context,
                      const ConnectedCallback &cnctcb,
                      const MessageCallback &msgcb,
                      const ClosedCallback &clscb,
                      const AnyEventCallback &eventcb)
    {
        _context = context;
        _connected_callback = cnctcb;
        _message_callback = msgcb;
        _closed_callback = clscb;
        _event_callback = eventcb;
    }

public:
    Connection(EventLoop *loop, uint64_t id, int sockfd)
        : _conn_id(id), _sockfd(sockfd), _socket(_sockfd), _loop(loop), _enable_inactive_release(false), _status(CONNECTING), _channel(loop, _sockfd)
    {
        _channel.SetReadCallback(std::bind(&Connection::HandleRead, this));
        _channel.SetWriteCallback(std::bind(&Connection::HandleWrite, this));
        _channel.SetCloseCallback(std::bind(&Connection::HandleClose, this));
        _channel.SetErrorCallback(std::bind(&Connection::HandleError, this));
        _channel.SetEventCallback(std::bind(&Connection::HandleEvent, this));
    }
    ~Connection()
    {
        lg(Debug, " Release connect:%p", this);
        // std::cout << "thread id: " << std::this_thread::get_id() << std::endl;
    }

    int Fd() // 管理的文件描述符
    {
        return _sockfd;
    }
    int Id() // 返回连接id
    {
        return _conn_id;
    }
    bool Connected() // 获取连接状态
    {
        return (CONNECTED == _status);
    }
    void SetContext(const Any &context) // 设置上下文
    {
        _context = context;
    }
    Any *GetContext() // 获取上下文
    {
        return &_context;
    }

    void SetConnectedCallback(const ConnectedCallback &cb)
    {
        _connected_callback = cb;
    }
    void SetMessageCallback(const MessageCallback &cb)
    {
        _message_callback = cb;
    }
    void SetClosedCallback(const ClosedCallback &cb)
    {
        _closed_callback = cb;
    }
    void SetAnyEventCallback(const AnyEventCallback &cb)
    {
        _event_callback = cb;
    }

    void SetSerClosedCallback(const ClosedCallback &cb)
    {
        _server_closed_callback = cb;
    }

    void Established() // 设置channel回调，启动读事件监控，调用_connected_callback
    {
        _loop->RunInLoop(std::bind(&Connection::EstablishedInLoop, this));
    }

    void Send(const char *data, size_t len) // 发送数据,放到缓冲区，开启监控
    {
        Buffer buf;
        buf.WriteAndPush(data, len);
        _loop->RunInLoop(std::bind(&Connection::SendInLoop, this, std::move(buf)));
    }
    void Release()
    {
        _loop->QueueInLoop(std::bind(&Connection::ReleaseInLoop, this));
    }
    void ShutDown() // 给用户的关闭接口
    {
        _loop->RunInLoop(std::bind(&Connection::ShutDownInLoop, this));
    }
    void EnableActiveRelease(int timeout) // 开启非活跃连接释放
    {
        _loop->RunInLoop(std::bind(&Connection::EnableActiveReleaseInLoop, this, timeout));
    }
    void DisActiveRelease() // 关闭非活跃连接释放
    {
        _loop->RunInLoop(std::bind(&Connection::DisActiveReleaseInLoop, this));
    }
    void Upgrde(const Any &context,
                const ConnectedCallback &ccb,
                const MessageCallback &mcb,
                const ClosedCallback &clscb,
                const AnyEventCallback &acb)
    {
        _loop->AssertInLoop();
        _loop->RunInLoop(std::bind(&Connection::UpgrdeInLoop, this, context, ccb, mcb, clscb, acb));
    }
};

class Acceptor
{
private:
    Socket _socket;   // 监听套接字
    EventLoop *_loop; // 事件监控
    Channel _channel; // 事件管理

    using AcceptCallback = std::function<void(int)>;
    AcceptCallback _accept_callback;

private:
    // 获取新链接，并调用_accept_callback
    void HandleRead()
    {
        std::string cip;
        uint16_t cport;
        int fd = _socket.Accept(&cip, &cport);
        if (fd < 0)
        {
            return;
        }
        if (_accept_callback)
        {
            _accept_callback(fd);
        }
    }

    int CreateFd(int port)
    {
        bool flag = _socket.CreateServer(port);
        assert(flag);
        return _socket.Fd();
    }

public:
    Acceptor(EventLoop *loop, int port)
        : _loop(loop), _socket(CreateFd(port)), _channel(loop, _socket.Fd())
    {
        _channel.SetReadCallback(std::bind(&Acceptor::HandleRead, this));
    }

    void SetAcceptCallback(const AcceptCallback &cb)
    {
        _accept_callback = cb;
    }

    void EableListen()
    {
        _channel.EnableRead();
    }
};

class TcpServer
{
private:
    using Func = std::function<void()>;
    using ConnectedCallback = std::function<void(const std::shared_ptr<Connection> &)>;
    using MessageCallback = std::function<void(const std::shared_ptr<Connection> &, Buffer *)>;
    using ClosedCallback = std::function<void(const std::shared_ptr<Connection> &)>;
    using AnyEventCallback = std::function<void(const std::shared_ptr<Connection> &)>;

    // 用户使用的回调函数
    ConnectedCallback _connected_callback;
    MessageCallback _message_callback;
    ClosedCallback _closed_callback;
    AnyEventCallback _event_callback;

    void CreateConnection(int fd) // 构造一个新connection来管理新连接
    {

        std::shared_ptr<Connection> con(new Connection(_pool.GetEventLoop(), _connect_id, fd));
        con->SetConnectedCallback(_connected_callback);
        con->SetMessageCallback(_message_callback);
        con->SetClosedCallback(_closed_callback);
        con->SetAnyEventCallback(_event_callback);
        con->SetSerClosedCallback(std::bind(&TcpServer::RemoveConnection, this, std::placeholders::_1));
        if (_enable_inactive_release)
        {
            con->EnableActiveRelease(_timeout);
        }
        con->Established();
        _conns[_connect_id] = con;
        _connect_id++;
    }

    void RemoveConnectionInLoop(const std::shared_ptr<Connection> &c)
    {
        int id = c->Id();
        _conns.erase(id);
    }

    void RemoveConnection(const std::shared_ptr<Connection> &c) // 从哈希表中删除connection
    {
        _base_loop.RunInLoop(std::bind(&TcpServer::RemoveConnectionInLoop, this, c));
    }

    void AddTimerTaskInLoop(const Func &Task, int timeout)
    {
        _connect_id++;
        _base_loop.TimerAdd(_connect_id, timeout, Task);
    }

public:
    TcpServer(uint16_t port)
        : _port(port), _connect_id(0), _enable_inactive_release(false), _accept(&_base_loop, port), _pool(&_base_loop)
    {
        _accept.SetAcceptCallback(std::bind(&TcpServer::CreateConnection, this, std::placeholders::_1));
        _accept.EableListen();
    }
    void SetMaxThreads(int max)
    {
        _pool.SetMaxThreads(max);
    }

    void SetConnectedCallback(const ConnectedCallback &cb)
    {
        _connected_callback = cb;
    }

    void SetMessageCallback(const MessageCallback &cb)
    {
        _message_callback = cb;
    }

    void SetClosedCallback(const ClosedCallback &cb)
    {
        _closed_callback = cb;
    }

    void SetAnyEventCallback(const AnyEventCallback &cb)
    {
        _event_callback = cb;
    }

    void EnableActiveRelease(int timeout) // 开启非活跃连接释放
    {
        _timeout = timeout;
        _enable_inactive_release = true;
    }

    void AddTimerTask(const Func &Task, int timeout) // 添加一个定时任务
    {
        _base_loop.RunInLoop(std::bind(&TcpServer::AddTimerTaskInLoop, this, Task, timeout));
    }

    void Launch()
    {
        _pool.CreateThread();
        _base_loop.Launch();
    }

private:
    uint16_t _port;
    uint64_t _connect_id;                                             // 线性增长的id
    int _timeout;                                                     // 超时时间
    bool _enable_inactive_release;                                    // 启动超时销毁
    Acceptor _accept;                                                 // 用来管理listen套接字
    EventLoop _base_loop;                                             // 主EventLoop，用来管理listen套接字
    ThreadPool _pool;                                                 // 用来管理从属EventLoop线程的线程池
    std::unordered_map<uint64_t, std::shared_ptr<Connection>> _conns; // 保存连接对应的智能指针对象
};

void Timerwheel::TimerAdd(uint64_t id, uint32_t timeout, const TaskFunc &task_cb)
{
    _loop->RunInLoop(std::bind(&Timerwheel::TimerAddLoop, this, id, timeout, task_cb));
}

void Timerwheel::TimeRefalsh(uint64_t id)
{
    _loop->RunInLoop(std::bind(&Timerwheel::TimeRefalshInLoop, this, id));
}

void Timerwheel::TaskCancel(uint64_t id)
{
    _loop->RunInLoop(std::bind(&Timerwheel::TaskCancelInLoop, this, id));
}

void Channel::Remove()
{
    return _loop->RemoveEvent(this);
}

void Channel::Update()
{
    return _loop->UpdateEvent(this);
}

class NetWork
{
public:
    NetWork()
    {
        lg(Debug, "sigpipe sig_ign");
        signal(SIGPIPE, SIG_IGN);
    }
};

static NetWork nw;