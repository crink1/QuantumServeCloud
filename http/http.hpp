#pragma once
#include <fstream>
#include <sys/stat.h>
#include <regex>
#include "../server.hpp"

std::unordered_map<int, std::string> _status = {
    {100, "Continue"},
    {101, "Switching Protocol"},
    {102, "Processing"},
    {103, "Early Hints"},
    {200, "OK"},
    {201, "Created"},
    {202, "Accepted"},
    {203, "Non-Authoritative Information"},
    {204, "No Content"},
    {205, "Reset Content"},
    {206, "Partial Content"},
    {207, "Multi-Status"},
    {208, "Already Reported"},
    {226, "IM Used"},
    {300, "Multiple Choice"},
    {301, "Moved Permanently"},
    {302, "Found"},
    {303, "See Other"},
    {304, "Not Modified"},
    {305, "Use Proxy"},
    {306, "unused"},
    {307, "Temporary Redirect"},
    {308, "Permanent Redirect"},
    {400, "Bad Request"},
    {401, "Unauthorized"},
    {402, "Payment Required"},
    {403, "Forbidden"},
    {404, "Not Found"},
    {405, "Method Not Allowed"},
    {406, "Not Acceptable"},
    {407, "Proxy Authentication Required"},
    {408, "Request Timeout"},
    {409, "Conflict"},
    {410, "Gone"},
    {411, "Length Required"},
    {412, "Precondition Failed"},
    {413, "Payload Too Large"},
    {414, "URI Too Long"},
    {415, "Unsupported Media Type"},
    {416, "Range Not Satisfiable"},
    {417, "Expectation Failed"},
    {418, "I'm a teapot"},
    {421, "Misdirected Request"},
    {422, "Unprocessable Entity"},
    {423, "Locked"},
    {424, "Failed Dependency"},
    {425, "Too Early"},
    {426, "Upgrade Required"},
    {428, "Precondition Required"},
    {429, "Too Many Requests"},
    {431, "Request Header Fields Too Large"},
    {451, "Unavailable For Legal Reasons"},
    {501, "Not Implemented"},
    {502, "Bad Gateway"},
    {503, "Service Unavailable"},
    {504, "Gateway Timeout"},
    {505, "HTTP Version Not Supported"},
    {506, "Variant Also Negotiates"},
    {507, "Insufficient Storage"},
    {508, "Loop Detected"},
    {510, "Not Extended"},
    {511, "Network Authentication Required"}};

std::unordered_map<std::string, std::string> _mime = {
    {".aac", "audio/aac"},
    {".abw", "application/x-abiword"},
    {".arc", "application/x-freearc"},
    {".avi", "video/x-msvideo"},
    {".azw", "application/vnd.amazon.ebook"},
    {".bin", "application/octet-stream"},
    {".bmp", "image/bmp"},
    {".bz", "application/x-bzip"},
    {".bz2", "application/x-bzip2"},
    {".csh", "application/x-csh"},
    {".css", "text/css"},
    {".csv", "text/csv"},
    {".doc", "application/msword"},
    {".docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
    {".eot", "application/vnd.ms-fontobject"},
    {".epub", "application/epub+zip"},
    {".gif", "image/gif"},
    {".htm", "text/html"},
    {".html", "text/html"},
    {".ico", "image/vnd.microsoft.icon"},
    {".ics", "text/calendar"},
    {".jar", "application/java-archive"},
    {".jpeg", "image/jpeg"},
    {".jpg", "image/jpeg"},
    {".js", "text/javascript"},
    {".json", "application/json"},
    {".jsonld", "application/ld+json"},
    {".mid", "audio/midi"},
    {".midi", "audio/x-midi"},
    {".mjs", "text/javascript"},
    {".mp3", "audio/mpeg"},
    {".mpeg", "video/mpeg"},
    {".mpkg", "application/vnd.apple.installer+xml"},
    {".odp", "application/vnd.oasis.opendocument.presentation"},
    {".ods", "application/vnd.oasis.opendocument.spreadsheet"},
    {".odt", "application/vnd.oasis.opendocument.text"},
    {".oga", "audio/ogg"},
    {".ogv", "video/ogg"},
    {".ogx", "application/ogg"},
    {".otf", "font/otf"},
    {".png", "image/png"},
    {".pdf", "application/pdf"},
    {".ppt", "application/vnd.ms-powerpoint"},
    {".pptx", "application/vnd.openxmlformats-officedocument.presentationml.presentation"},
    {".rar", "application/x-rar-compressed"},
    {".rtf", "application/rtf"},
    {".sh", "application/x-sh"},
    {".svg", "image/svg+xml"},
    {".swf", "application/x-shockwave-flash"},
    {".tar", "application/x-tar"},
    {".tif", "image/tiff"},
    {".tiff", "image/tiff"},
    {".ttf", "font/ttf"},
    {".txt", "text/plain"},
    {".vsd", "application/vnd.visio"},
    {".wav", "audio/wav"},
    {".weba", "audio/webm"},
    {".webm", "video/webm"},
    {".webp", "image/webp"},
    {".woff", "font/woff"},
    {".woff2", "font/woff2"},
    {".xhtml", "application/xhtml+xml"},
    {".xls", "application/vnd.ms-excel"},
    {".xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
    {".xml", "application/xml"},
    {".xul", "application/vnd.mozilla.xul+xml"},
    {".zip", "application/zip"},
    {".3gp", "video/3gpp"},
    {".3g2", "video/3gpp2"},
    {".7z", "application/x-7z-compressed"}};

class Util
{
public:
    // 读文件
    static bool ReadFile(const std::string &filename, std::string *buf)
    {
        std::ifstream ifs(filename, std::ios::binary);
        if (ifs.is_open() == false)
        {
            lg(Fatal, "Open %s file failed!!", filename.c_str());
            return false;
        }
        size_t fsize = 0;
        ifs.seekg(0, ifs.end); // 指针跳转到文件尾
        fsize = ifs.tellg();   // 获取当前位置相对于起始位置的偏移量， 文件末尾偏移就是文件大小
        ifs.seekg(0, ifs.beg); // 指针跳转文件起始位置
        buf->resize(fsize);
        ifs.read(&(*buf)[0], fsize);
        if (ifs.good() == false)
        {
            lg(Fatal, "Read %s File failed!!", filename.c_str());
            ifs.close();
            return false;
        }

        ifs.close();
        return true;
    }
    // 写文件
    static bool WriteFile(const std::string &filename, const std::string &buf)
    {
        std::ofstream ifs(filename, std::ios::binary);
        if (ifs.is_open() == false)
        {
            lg(Fatal, "Open %s file failed!!", filename.c_str());
            return false;
        }

        ifs.write(buf.c_str(), buf.size());
        if (ifs.good() == false)
        {
            lg(Fatal, "Write %s File failed!!", filename.c_str());
            ifs.close();
            return false;
        }

        ifs.close();
        return true;
    }
    // URL编码
    static std::string URLEncode(const std::string url, bool space_to_plus)
    {
        std::string res;
        for (auto &c : url)
        {
            if (c == '.' || c == '-' || c == '_' || c == '~' || isalnum(c))
            {
                res += c;
                continue;
            }

            if (c == ' ' && space_to_plus == true)
            {
                res += '+';
                continue;
            }
            char tmp[4] = {0};
            snprintf(tmp, 4, "%%%02X", c);
            res += tmp;
        }
        return res;
    }
    // URL解码
    static char ctoi(char c)
    {
        if (isdigit(c))
        {
            return c - '0';
        }
        else if (isupper(c))
        {
            return c - 'A' + 10;
        }
        else if (islower(c))
        {
            return c - 'a' + 10;
        }
        else
        {
            lg(Error, "%c is other char", c);
        }
        return -1;
    }
    static std::string URLDecode(const std::string url, bool plus_to_space)
    {
        std::string res;
        for (int i = 0; i < url.size(); i++)
        {
            if (url[i] == '+' && plus_to_space == true)
            {
                res += ' ';
                continue;
            }
            if (url[i] == '%')
            {
                char c1 = ctoi(url[i + 1]);
                char c2 = ctoi(url[i + 2]);
                char c = c1 << 4 + c2;
                res += c;
                i += 2;
                continue;
            }
            res += url[i];
        }
        return res;
    }
    // 获取http状态码
    static std::string StatuDesc(int statu)
    {
        auto it = _status.find(statu);
        if (it != _status.end())
        {
            return it->second;
        }
        return "Unkown";
    }
    // 获取文件类型
    static std::string ExtMime(const std::string &filename)
    {
        size_t pos = filename.rfind('.');
        if (pos == std::string::npos)
        {
            return "application/octet-stream";
        }
        std::string tmp = filename.substr(pos);
        auto it = _mime.find(tmp);
        if (it != _mime.end())
        {
            return it->second;
        }
        else
        {
            return "application/octet-stream";
        }
    }
    // 判断一个文件是否是目录
    static bool IsDirectory(const std::string &filename)
    {
        struct stat st;
        int ret = stat(filename.c_str(), &st);
        if (ret < 0)
        {
            return false;
        }
        return S_ISDIR(st.st_mode);
    }
    // 判断一个文件是否是普通文件
    static bool IsRegular(const std::string &filename)
    {
        struct stat st;
        int ret = stat(filename.c_str(), &st);
        if (ret < 0)
        {
            return false;
        }
        return S_ISREG(st.st_mode);
    }
    // 判断http请求路径的有效性
    static bool ValidPath(const std::string &path)
    {
        // 通过计算目录深度来限制
        std::vector<std::string> substring;
        Split(path, "/", &substring);
        int level = 0;
        for (auto &i : substring)
        {
            if (i == "..")
            {
                level--;
                if (level < 0)
                {
                    return false;
                }
                continue;
            }
            level++;
        }
        return true;
    }
    // 分割字符串
    static size_t Split(const std::string &str, const std::string &sep, std::vector<std::string> *ret)
    {
        std::size_t previous = 0;
        std::size_t current = str.find(sep);
        while (current != std::string::npos)
        {
            if (current > previous)
            {
                ret->push_back(str.substr(previous, current - previous));
            }
            previous = current + 1;
            current = str.find(sep, previous);
        }
        if (previous != str.size())
        {
            ret->push_back(str.substr(previous));
        }
        return ret->size();
    }
};

struct HttpRequest
{
    HttpRequest()
        : _version("HTTP/1.1")
    {
    }
    std::string _method;                                   // 请求方式
    std::string _path;                                     // 请求路径
    std::string _version;                                  // 协议版本
    std::string _body;                                     // 请求正文
    std::smatch _matches;                                  // 正则提取资源路径
    std::unordered_map<std::string, std::string> _headers; // 头部字段
    std::unordered_map<std::string, std::string> _params;  // 查询字符串

    // 插入头部字段
    void SetHeader(const std::string &key, const std::string &val)
    {
        _headers.insert(std::make_pair(key, val));
    }
    // 插入查询字符串
    void SetParam(const std::string &key, const std::string &val)
    {
        _params.insert(std::make_pair(key, val));
    }
    // 判断头部字段是否存在
    bool HasHeader(const std::string &key) const
    {
        auto it = _headers.find(key);
        if (it == _headers.end())
        {
            return false;
        }
        return true;
    }
    // 判断查询字符串是否存在
    bool HasParam(const std::string &key) const
    {
        auto it = _params.find(key);
        if (it == _params.end())
        {
            return false;
        }
        return true;
    }
    // 获取指定头部字段的值
    std::string GetHeader(const std::string &key) const
    {
        auto it = _headers.find(key);
        if (it == _headers.end())
        {
            return "";
        }
        return it->second;
    }
    // 获取内容长度
    size_t ContentLength()
    {
        bool flag = HasHeader("Content-Length");
        if (flag)
        {
            std::string res = GetHeader("Content-Length");
            return std::stol(res);
        }
        return 0;
    }
    // 是否短连接
    bool Close() const
    {
        if (HasHeader("Connection") && GetHeader("Connection") == "keep-alive")
        {
            return false;
        }
        return true;
    }

    void Reset()
    {
        _method.clear();
        _path.clear();
        _version = "HTTP/1.1";
        _body.clear();
        std::smatch tmp;
        _matches.swap(tmp);
        _headers.clear();
        _params.clear();
    }
};

struct HttpResponse
{
    int _status;
    bool _redirect_flag;
    std::string _body;
    std::string _redirect_url;
    std::unordered_map<std::string, std::string> _headers;

    HttpResponse()
        : _status(200), _redirect_flag(false)
    {
    }
    HttpResponse(int status)
        : _status(status), _redirect_flag(false)
    {
    }
    void SetHeader(const std::string &k, const std::string &v)
    {
        _headers.insert(std::make_pair(k, v));
    }

    void SetContent(const std::string &body, const std::string &type = "text/html")
    {
        _body = body;
        SetHeader("Content-Type", type);
    }

    void SetReadirect(const std::string &url, int status = 302)
    {
        _status = status;
        _redirect_flag = true;
        _redirect_url = url;
    }

    bool HasHeader(const std::string &k)
    {
        auto it = _headers.find(k);
        if (it == _headers.end())
        {
            return false;
        }
        return true;
    }

    std::string GetHeader(const std::string &k)
    {
        auto it = _headers.find(k);
        if (it == _headers.end())
        {
            return "";
        }
        return it->second;
    }

    // 是否短连接
    bool Close()
    {
        if (HasHeader("Connection") && GetHeader("Connection") == "keep-alive")
        {
            return false;
        }
        return true;
    }

    void Reset()
    {
        _status = 200;
        _redirect_flag = false;
        _body.clear();
        _redirect_url.clear();
        _headers.clear();
    }
};

typedef enum
{
    RECV_HTTP_ERROR,
    RECV_HTTP_LINE,
    RECV_HTTP_HEAD,
    RECV_HTTP_BODY,
    RECV_HTTP_OVER
} HttpRecvStatus;

#define MAX_LINE 8192
class HttpContext
{
public:
    HttpContext()
        : _resp_status(200), _recv_status(RECV_HTTP_LINE)
    {
    }

    void Reset()
    {
        _recv_status = RECV_HTTP_LINE;
        _resp_status = 200;
        _request.Reset();
    }

    int RespStatus()
    {
        return _resp_status;
    }

    HttpRecvStatus RecvStatus()
    {
        return _recv_status;
    }
    HttpRequest &Request()
    {
        return _request;
    }
    void RecvHttpRequest(Buffer *buf) // 用于接受并解析HTTP请求的
    {
        // 根据不同的状态选择， 处理完请求行就要处理头部了，以此类推
        switch (_recv_status)
        {
        case RECV_HTTP_LINE:
            RecvHttpLine(buf);
        case RECV_HTTP_HEAD:
            RecvHttpHead(buf);
        case RECV_HTTP_BODY:
            RecvHttpBody(buf);
        }
        return;
    }

private:
    bool ParseHttpLine(const std::string &line)
    {
        std::regex e("(GET|HEAD|POST|PUT|DELETE) ([^?]*)(?:\\?(.*))? (HTTP/[12]\\.[01])(?:\n|\r\n)?", std::regex::icase);
        std::smatch matches;

        bool re = std::regex_match(line, matches, e);
        if (re == false)
        {
            _recv_status = RECV_HTTP_ERROR;
            _resp_status = 400;
            return false;
        }

        _request._method = matches[1];
        std::transform(_request._method.begin(), _request._method.end(), _request._method.begin(), ::toupper);
        // 获取请求方法
        _request._method = matches[1];
        // 获取请求url,url需要解码
        _request._path = Util::URLDecode(matches[2], false);
        // 获取协议
        _request._version = matches[4];
        // 获取参数
        std::vector<std::string> ver;
        std::string str = matches[3];
        // 字符串格式 k=v&k=v
        Util::Split(str, "&", &ver);
        for (auto &i : ver)
        {
            size_t pos = i.find("=");
            if (pos == std::string::npos)
            {
                _recv_status = RECV_HTTP_ERROR;

                _resp_status = 400;
                return false;
            }
            // 需要开启空格转换
            std::string k = Util::URLDecode(i.substr(0, pos), true);
            std::string v = Util::URLDecode(i.substr(pos + 1), true);
            _request.SetParam(k, v);
        }
        return true;
    }

    bool RecvHttpLine(Buffer *buf)
    {
        if (_recv_status != RECV_HTTP_LINE)
        {
            return false;
        }
        // 获取一行数据
        std::string line = buf->GetLineAndPop();
        // 判断是否为空以及一行数据超出了设定范围
        if (line.size() == 0)
        {
            if (buf->ReadableSize() > MAX_LINE)
            {
                _recv_status = RECV_HTTP_ERROR;
                _resp_status = 414;
                return false;
            }
            return true;
        }
        if (line.size() > MAX_LINE)
        {
            _recv_status = RECV_HTTP_ERROR;
            _resp_status = 414;
            return false;
        }
        // 首行处理完毕，开始处理http头部
        _recv_status = RECV_HTTP_HEAD;
        return ParseHttpLine(line);
    }

    bool ParseHttpHead(std::string &line)
    {
        // 头部数据是k:v/r/n的
        if (line.back() == '\n')
        {
            line.pop_back();
        }
        if (line.back() == '\r')
        {
            line.pop_back();
        }
        size_t pos = line.find(": ");
        if (pos == std::string::npos)
        {
            _recv_status = RECV_HTTP_ERROR;
            _resp_status = 400;
            return false;
        }
        std::string k = line.substr(0, pos);
        std::string v = line.substr(pos + 2);
        _request.SetHeader(k, v);

        return true;
    }

    bool RecvHttpHead(Buffer *buf)
    {
        if (_recv_status != RECV_HTTP_HEAD)
        {
            return false;
        }
        while (1)
        {
            std::string line = buf->GetLineAndPop();

            // 判断是否为空以及一行数据超出了设定范围
            if (line.size() == 0)
            {
                if (buf->ReadableSize() > MAX_LINE)
                {
                    _recv_status = RECV_HTTP_ERROR;
                    _resp_status = 414;
                    return false;
                }
                // 缓冲区数据不够一行，继续等数据到来
                return true;
            }

            if (line.size() > MAX_LINE)
            {
                _recv_status = RECV_HTTP_ERROR;
                _resp_status = 414;
                return false;
            }

            if (line == "\n" || line == "\r\n")
            {
                break;
            }

            bool ret = ParseHttpHead(line);
            if (!ret)
            {
                return false;
            }
        }
        // 头部处理完成，开始处理正文
        _recv_status = RECV_HTTP_BODY;
        return true;
    }

    bool RecvHttpBody(Buffer *buf)
    {
        if (_recv_status != RECV_HTTP_BODY)
        {
            return false;
        }
        // 正文长度
        size_t clen = _request.ContentLength();
        if (clen == 0)
        {
            _recv_status = RECV_HTTP_OVER;
            return true;
        }
        // 已经接受了多少数据
        size_t rlen = clen - _request._body.size();
        // 只取出需要的数据
        if (buf->ReadableSize() >= rlen)
        {
            _request._body.append(buf->ReadPos(), rlen);
            buf->MoveReadOffset(rlen);
            _recv_status = RECV_HTTP_OVER;
            return true;
        }
        // 缓冲区的数据还不够contentlength，有多少取多少
        _request._body.append(buf->ReadPos(), buf->ReadableSize());
        buf->MoveReadOffset(buf->ReadableSize());
        return true;
    }

private:
    int _resp_status;            // 状态码
    HttpRecvStatus _recv_status; // 当前接收数据和解析数据的位置
    HttpRequest _request;        // 解析完成的请求
};




#define DEFAULT_TIMEOUT 10
class HttpServer
{
private:
    using Handler = std::function<void(const HttpRequest &, HttpResponse *)>;
    using Handlers = std::vector<std::pair<std::regex, Handler>>;
    Handlers _get_route;
    Handlers _post_route;
    Handlers _put_route;
    Handlers _delete_route;
    std::string _basedir; // 静态资源根目录
    TcpServer _server;

private:
    void ErrorHandler(const HttpRequest &req, HttpResponse *rsp)
    {
        std::string body;
        body += "<html>";
        body += "<head>";
        body += "<meta http-equiv='Content-Type' content='text/html;charset=utf-8'>";
        body += "</head>";
        body += "<body>";
        body += "<h1>";
        body += std::to_string(rsp->_status);
        body += " ";
        body += Util::StatuDesc(rsp->_status);
        body += "</h1>";
        body += "</body>";
        body += "</html>";
        rsp->SetContent(body, "text/html");
    }

    void WriteReponse(const std::shared_ptr<Connection> &conn, const HttpRequest &req, HttpResponse &rsp) // 组织httpresponse并发送
    {
        if (req.Close())
        {
            rsp.SetHeader("Connection", "close");
        }
        else
        {
            rsp.SetHeader("Connection", "keep-alive");
        }

        if (rsp._body.empty() == false && rsp.HasHeader("Content-Length") == false)
        {
            rsp.SetHeader("Content-Length", std::to_string(rsp._body.size()));
        }
        if (rsp._body.empty() == false && rsp.HasHeader("Content-Type") == false)
        {
            rsp.SetHeader("Content-Type", "application/octet-stream");
        }
        if (rsp._redirect_flag == true)
        {
            rsp.SetHeader("Location", rsp._redirect_url);
        }
        // 按照http格式组织
        std::stringstream rsp_str;
        rsp_str << req._version << " " << std::to_string(rsp._status) << " " << Util::StatuDesc(rsp._status) << "\r\n";

        for (auto &i : rsp._headers)
        {
            rsp_str << i.first << ": " << i.second << "\r\n";
        }
        rsp_str << "\r\n";
        rsp_str << rsp._body;
        // 发送数据
        conn->Send(rsp_str.str().c_str(), rsp_str.str().size());
    }

    bool IsFileHandler(const HttpRequest &req)
    {
        // 判断是否有静态根目录
        if (_basedir.empty())
        {
            return false;
        }
        // 判断请求是否为get head
        if (req._method != "GET" && req._method != "HEAD")
        {
            return false;
        }
        // 判断是否为合法路径
        if (!Util::ValidPath(req._path))
        {
            return false;
        }

        // 判断请求资源是否合法
        std::string path = _basedir + req._path;
        if (path.back() == '/')
        {
            path += "index.html";
        }
        if (!Util::IsRegular(path))
        {
            return false;
        }

        return true;
    }
    void FileHandler(const HttpRequest &req, HttpResponse *rsp) // 处理静态资源的请求
    {
        std::string path = _basedir + req._path;
        if (path.back() == '/')
        {
            path += "index.html";
        }
        bool res = Util::ReadFile(path, &rsp->_body);
        if (!res)
        {
            return;
        }
        std::string mine = Util::ExtMime(path);
        rsp->SetHeader("Content-Type", mine);
    }

    void Dispatcher(HttpRequest &req, HttpResponse *rsp, Handlers &handlers) // 功能性的请求进行分类处理
    {
        // 在路由表中查找是否有对应得到资源处理函数，有就调用，没有就404
        for (auto &i : handlers)
        {
            const std::regex &re = i.first;
            bool ret = std::regex_match(req._path, req._matches, re);
            if (!ret)
            {
                continue;
            }
            const Handler &fun = i.second;
            return fun(req, rsp);
        }
        rsp->_status = 404;
    }
    void Route(HttpRequest &req, HttpResponse *rsp)
    {
        // 判断是否是请求静态资源
        if (IsFileHandler(req) == true)
        {
            return FileHandler(req, rsp);
        }
        // 默认GET HEAD是静态资源
        if (req._method == "GET" || req._method == "HEAD")
        {
            return Dispatcher(req, rsp, _get_route);
        }
        else if (req._method == "POST")
        {
            return Dispatcher(req, rsp, _post_route);
        }
        else if (req._method == "PUT")
        {
            return Dispatcher(req, rsp, _put_route);
        }
        else if (req._method == "DELETE")
        {
            return Dispatcher(req, rsp, _delete_route);
        }
        rsp->_status = 405;
    }
    void Onconnected(const std::shared_ptr<Connection> &conn) // 上下文
    {
        conn->SetContext(HttpContext());
        lg(Debug, "New Connection %p", conn.get());
    }
    // 处理缓冲区的数据
    void OnMessage(const std::shared_ptr<Connection> &conn, Buffer *buf)
    {
        while (buf->ReadableSize() > 0)
        {
            // 获取上下文
            HttpContext *context = conn->GetContext()->get<HttpContext>();
            // 解析上下文，得到HttpRequest
            context->RecvHttpRequest(buf);
            HttpRequest &req = context->Request();
            HttpResponse rsp(context->RespStatus());
            if (context->RespStatus() >= 400)
            {
                ErrorHandler(req, &rsp);      // 给response填充错误信息
                WriteReponse(conn, req, rsp); // 发送给客户端
                context->Reset();
                buf->MoveReadOffset(buf->ReadableSize());
                conn->ShutDown();
                return;
            }
            if (context->RecvStatus() != RECV_HTTP_OVER)
            {
                // 数据不完整等待新的连接再继续处理
                return;
            }
            // 路由+ 处理
            Route(req, &rsp);
            // 发送HttpReaponse
            WriteReponse(conn, req, rsp);
            // 重置上下文
            context->Reset();
            // 判断链接类型
            if (rsp.Close() == true)
            {
                conn->ShutDown();
            }
        }
    }

public:
    HttpServer(int port, int timeout = DEFAULT_TIMEOUT)
        : _server(port)
    {
        _server.EnableActiveRelease(timeout);
        _server.SetConnectedCallback(std::bind(&HttpServer::Onconnected, this, std::placeholders::_1));
        _server.SetMessageCallback(std::bind(&HttpServer::OnMessage, this, std::placeholders::_1, std::placeholders::_2));
    }
    
    void SetBaseDir(const std::string &path)
    {
        bool ret = Util::IsDirectory(path);
        assert(ret);
        _basedir = path;
    }

    void Get(const std::string &pattern, const Handler &handler)
    {
        _get_route.push_back(std::make_pair(std::regex(pattern), handler));
    }
    void Post(const std::string &pattern, const Handler &handler)
    {
        _post_route.push_back(std::make_pair(std::regex(pattern), handler));
    }
    void Put(const std::string &pattern, const Handler &handler)
    {
        _put_route.push_back(std::make_pair(std::regex(pattern), handler));
    }
    void Delete(const std::string &pattern, const Handler &handler)
    {
        _delete_route.push_back(std::make_pair(std::regex(pattern), handler));
    }
    void SetMaxThread(size_t n)
    {
        _server.SetMaxThreads(n);
    }

    void Launch()
    {
        _server.Launch();
    }
};