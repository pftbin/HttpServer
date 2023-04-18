#pragma once

#include <thread>
#include <list>
#include "logger.h"

using namespace std;
using namespace logger;

static int log_test()
{
    list<shared_ptr<thread>> oThreads;

#if 0
    //控制台输出
    ConsoleLogger ocl;
    for (int i = 0; i < 10; i++)
    {
        oThreads.push_back(shared_ptr<thread>(new thread([=]() 
            {
            for (int j = 0; j < 100; ++j)
                debug() << "Thread " << i << ", Message " << j;
            })));
    }
    for (int i = 0; i < 100; i++)
        debug() << "Main thread, Message " << i;
    for (auto oThread : oThreads)
        oThread->join();

    debug(Level::Info) << "output to console, done.";
    oThreads.clear();
#endif

    //日志文档输出
    FileLogger ofl("shit.log");
    for (int i = 0; i < 10; i++)
    {
        oThreads.push_back(shared_ptr<thread>(new thread([=]() {
            for (int j = 0; j < 100; ++j)
                record() << "Thread " << i << ", Message " << j;
            })));
    }
    for (int i = 0; i < 100; i++)
        record() << "Main thread, Message " << i;

    //等待子线程执行结束
    for (auto oThread : oThreads)
        oThread->join();

    //主线程提示
    debug(Level::Info) << "output to file done.";
    oThreads.clear();

    return 0;
}

