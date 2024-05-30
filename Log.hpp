#pragma once

#include <iostream>
#include <time.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#define SIZE 2048

#define Info 0
#define Debug 1
#define Warning 2
#define Error 3
#define Fatal 4

#define Screen 1
#define Onefile 2
#define Classfile 3

#define LogFile "log.txt"

class Log
{
public:
    Log()
    {
        print_mod = Screen;
        path = "./log/";
    }

    void changemod(int mod)
    {
        print_mod = mod;
    }

    std::string levelToString(int level)
    {
        switch (level)
        {
        case Info:
            return "Info";
        case Debug:
            return "Debug";
        case Warning:
            return "Warning";
        case Error:
            return "Error";
        case Fatal:
            return "Fatal";
        default:
            return "None";
        }
    }

    void printLog(int level, const std::string &logtxt)
    {
        switch (print_mod)
        {
        case Screen:
            std::cout << logtxt << std::endl;
            break;
        case Onefile:
            PrintOneFile(LogFile, logtxt);
            break;
        case Classfile:
            PrintClassFile(level, logtxt);
            break;
        default:
            break;
        }
    }

    void PrintOneFile(const std::string &logname, const std::string &logtxt)
    {
        std::string logpath = path + logname;
        int fd = open(logpath.c_str(), O_CREAT | O_WRONLY | O_APPEND, 0666);
        if (fd < 0)
        {
            return;
        }
        write(fd, logtxt.c_str(), logtxt.size());
        write(fd, "\n", sizeof("\n"));
        close(fd);
    }

    void PrintClassFile(int level, const std::string &logtxt)
    {
        std::string filename = LogFile;
        filename += ".";
        filename += levelToString(level);
        PrintOneFile(filename, logtxt);
    }

    void operator()(int level, const char *format, ...)
    {
        time_t t = time(nullptr);
        struct tm *ctime = localtime(&t);
        char leftbuffer[SIZE];
        snprintf(leftbuffer, sizeof(leftbuffer), "[%s][%d-%d-%d %d:%d:%d]", levelToString(level).c_str(),
                 ctime->tm_year + 1900, ctime->tm_mon + 1, ctime->tm_mday,
                 ctime->tm_hour, ctime->tm_min, ctime->tm_sec);

        va_list s;
        va_start(s, format);
        char rightbuffer[SIZE];
        vsnprintf(rightbuffer, sizeof(rightbuffer), format, s);
        va_end(s);

        // 默认+自定义
        char logtxt[SIZE * 2];
        snprintf(logtxt, sizeof(logtxt), "%s %s", leftbuffer, rightbuffer);
        printLog(level, logtxt);
    }

    ~Log()
    {
    }

private:
    int print_mod;
    std::string path;
};

Log lg;
