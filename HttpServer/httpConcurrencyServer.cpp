#include "httpConcurrencyServer.h"
#include "public.h"

static bool globalIsIPStringValid(std::string IPString)
{
	std::string strIpAddr;
#if defined(UNICODE) || defined(_UNICODE)
	unicode_to_ansi(IPString.c_str(), IPString.length(), strIpAddr);
#else
	strIpAddr = IPString;
#endif
	std::vector<std::string> ipVector;
	globalSpliteString(IPString, ipVector, ("."));
	if (ipVector.size() != 4U) return false;
	for (std::vector<std::string>::iterator it = ipVector.begin(); it != ipVector.end(); it++)
	{
		int temp;
		globalStrToIntDef(const_cast<LPTSTR>(it->c_str()), temp);
		if (temp < 0 || temp > 255) return false;
	}
	struct in_addr addr;
	//if (inet_pton(AF_INET, strIpAddr.c_str(), (void *)&addr) == 1)
	if (inet_pton(AF_INET, strIpAddr.c_str(), reinterpret_cast<void*>(&addr)) == 1)
		return true;
	else
		return false;
}

#ifdef WIN32  //SYS-WIN32

#pragma comment(lib,"pthreadVC2.lib")
namespace httpServer
{
	int init_win_socket()
	{
		WSADATA wsaData;
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		{
			return -1;
		}
		return 0;
	}
	int httpserver_bindsocket(int port, int backlog);
	int httpserver_start(int port, int nthreads, int backlog);
	void* httpserver_Dispatch(void *arg);
	void httpserver_GenericHandler(struct evhttp_request *req, void *arg);
	void httpserver_ProcessRequest(struct evhttp_request *req);

	int httpserver_bindsocket(int port, int backlog) {
		int r;

		int nsfd;
		nsfd = socket(AF_INET, SOCK_STREAM, 0);//创建套接字
		if (nsfd < 0) return -1;

		int one = 1;
		r = setsockopt(nsfd, SOL_SOCKET, SO_REUSEADDR, (char *)&one, sizeof(int));//设置套接字的SO_REUSEADDR属性 执行正确返回0

		struct sockaddr_in addr;
		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = INADDR_ANY;
		addr.sin_port = htons(port);

		r = bind(nsfd, (struct sockaddr*)&addr, sizeof(addr));
		if (r < 0) return -1;
		r = listen(nsfd, backlog);
		if (r < 0) return -1;

		unsigned long ul = 1;
		int ret = ioctlsocket(nsfd, FIONBIO, (unsigned long *)&ul);//允许非阻塞模式
		if (ret == SOCKET_ERROR)//设置失败。
		{
			return -1;
		}

		return nsfd;
	}

	int httpserver_start(int port, int nthreads, int backlog) {

		int r, i;
		int nfd = httpserver_bindsocket(port, backlog);
		if (nfd < 0) return -1;
		pthread_t ths[10];
		nthreads = 10;
		for (i = 0; i < nthreads; i++) {
			struct event_base *base = event_base_new();
            if (base == nullptr) return -1;
			struct evhttp *httpd = evhttp_new(base);
            if (httpd == nullptr) return -1;
			r = evhttp_accept_socket(httpd, nfd);
			if (r != 0) return -1;
            evhttp_set_gencb(httpd, httpserver_GenericHandler, nullptr);
            r = pthread_create(&ths[i], nullptr, httpserver_Dispatch, base);
			if (r != 0) return -1;
		}
		for (i = 0; i < nthreads; i++) {
            pthread_join(ths[i], nullptr);
		}
		return 0;
	}

	void* httpserver_Dispatch(void *arg) {
		event_base_dispatch((struct event_base*)arg);
        return nullptr;
	}

	void httpserver_GenericHandler(struct evhttp_request *req, void *arg) {
        //Q_UNUSED(arg);
		httpserver_ProcessRequest(req);
	}

	void httpserver_ProcessRequest(struct evhttp_request *req) {
        //Q_UNUSED(req);
		//struct evbuffer *buf = evbuffer_new();
		//if (buf == NULL) return;

		//here comes the magic
	}
	DWORD WINAPI GlobalHttpBaseFunc(LPVOID lpParam)
	{
        try {
			struct httpThread * pThread = (struct httpThread*)lpParam;
			event_base_dispatch(pThread->event_base);//等待事件被触发，然后调用它们的回调函数
			evhttp_free(pThread->http_server);//释放以前创建的HTTP服务器
            pThread->http_server = nullptr;
			return NULL;
        } catch (...) {
			return NULL;
		}
	}

	int complex_httpServer::start_http_server()
	{
		stop_http_server();

		//http_checkEvent[0] = ::CreateEvent(NULL, true, false, NULL);
		//http_checkEvent[1] = ::CreateEvent(NULL, true, false, NULL);

#ifdef WIN32
		init_win_socket();//初始化winsock服务
#endif
		if (nthreads > 0)
		{
			try
			{
				pSeverThread = new struct httpThread[nthreads];//创建100个httpTreads结构体
                if (pSeverThread == nullptr)
				{
                    _debug_to(0,("start http server failed,error:new http thread failed\n"));
					return -1;
				}
			}
			catch (...)
			{
                _debug_to(0,("start http server failed, error:new http thread catch exception\n"));
				return -1;
			}
		}
        _debug_to(0,("start http server begin, port is %d, ipaddr is %s\n"), port, ipaddr.c_str());
		try
		{
			int r, i;
			evthread_use_windows_threads();//开启多线程功能
			//evthread_set_condition_callbacks();

			//evthread_set_id_callback();
			
			nfd = httpserver_bindsocket(port, backlog);//返回一个初始化的监听ANY_IPADDR的套接字
			if (nfd < 0) return -1;
			for (i = 0; i < nthreads; i++)
			{
				pSeverThread[i].get_CriticalSection = get_CriticalSection;//get_CriticalSection已经初始化的临界区资源
				pSeverThread[i].mainWindow = mainWindow;//NULL
				struct event_base *base = event_base_new();//event_base是libevent的事务处理框架，负责事件注册、删除等 
				//event_base_new 创建event_base对象
                if (base == nullptr)
				{
                    _debug_to(0,("start http server failed,error:new http base failed\n"));
					stop_failed_server(i);
					return -1;
				}
				pSeverThread[i].event_base = base;
				struct evhttp *httpd = evhttp_new(base);//创建http服务器 base是用于接收HTTP事件的事件库
                if (httpd == nullptr)
				{
                    _debug_to(0,("start http server failed,error:new evhttp_new failed\n"));
					stop_failed_server(i);
					event_base_free(base);
					return -1;
				}
				pSeverThread[i].http_server = httpd;
				r = evhttp_accept_socket(httpd, nfd);//使http server可以接受来自指定的socket的连接，可重复调用来绑定到不同的socket
				if (r != 0)
				{
                    _debug_to(0,("start http server failed,error:accept socket failed\n"));
					stop_failed_server(i);
					evhttp_free(httpd);
					event_base_free(base);
					return -1;
				}
                evhttp_set_timeout(httpd, 60);//为一个http请求设置超时时间 以秒为单位，最大60秒
				//evhttp_set_gencb(httpd, pFunc, this);
				evhttp_set_gencb(httpd, pFunc, &pSeverThread[i]);//设置请求处理回调方法

                //evhttp_set_max_body_size(httpd, 65536);
				
				//DWORD dwThread;
                pSeverThread[i].threadhandle = ::CreateThread(nullptr, 1024, GlobalHttpBaseFunc, &pSeverThread[i], 0, &pSeverThread[i].threadid);
	
                //CreateThread 第一个参数 安全属性，第二个参数 栈空间大小 ，第三个参数 线程函数 ，传入的参数，是否创建完毕即可调度，保存线程ID
				//返回新线程的句柄
                if (pSeverThread[i].threadhandle == nullptr)
				{
                    _debug_to(0,("start http server failed,error:create thread failed\n"));
					stop_failed_server(i);
					evhttp_free(httpd);
					event_base_free(base);
					return -1;
				}
			}
			return 0;
		}
		catch (...)
		{
            _debug_to(0,("start http server failed,error:exception\n"));
			return -1;
		}
		return 0;
	}

	//httpServer命名空间内
	int complex_httpServer::start_http_server(http_cb_Func _pFunc, HWND _mainWindow, int _port, int httpthreads, int nbacklog, std::string _ipaddr)
	{
		port = _port;
		nthreads = httpthreads; //100
		backlog = nbacklog; //1024*100

		if (port < 1 || port > 65535) port = 8130;

        if (!globalIsIPStringValid(_ipaddr))//判断ip是否合法
		{
			ipaddr = ("0.0.0.0");
		}
		else
		{
			ipaddr = _ipaddr;
		}
		pFunc = _pFunc;
		mainWindow = _mainWindow;
		std::string http_addr;
#if defined(UNICODE) || defined(_UNICODE)
        unicode_to_ansi(ipaddr.c_str(), ipaddr.length(), http_addr);
#else	
		http_addr = ipaddr;
#endif
		if (start_http_server() == 0)
			return 0;
		else return -1;
	}

	void complex_httpServer::stop_failed_server(int threadcount)
	{
		if (threadcount < 1)
			return;

		try
		{
			if (pSeverThread)
			{
                _debug_to(0,("stop_failed_server begin\n"));
				int i = threadcount - 1;
				while (i >= 0)
				{
					if (pSeverThread[i].event_base)
					{
						event_base_loopbreak(pSeverThread[i].event_base);
					}
					i--;
				}

				i = threadcount - 1;
				while (i >= 0)
				{
					if (pSeverThread[i].threadhandle)
					{
						if (::WaitForSingleObject(pSeverThread[i].threadhandle, 1000) != WAIT_OBJECT_0)
						{
							try
							{
								::TerminateThread(pSeverThread[i].threadhandle, 0);
							}
							catch (...)
							{
								;
							}
                            _debug_to(0,("stop_failed_server timeout ,terminate thread\n"));
						}
						::CloseHandle(pSeverThread[i].threadhandle);
					}
					i--;
				}
				i = threadcount - 1;
				while (i >= 0)
				{
					if (pSeverThread[i].event_base)
					{
						//event_base_loopbreak(pSeverThread[i].event_base);
						event_base_free(pSeverThread[i].event_base);
                        pSeverThread[i].event_base = nullptr;
					}
					i--;
				}
				WSACleanup();
				delete[]pSeverThread;
                pSeverThread = nullptr;
                _debug_to(0,("stop failed server finish\n"));
			}
		}
		catch (...)
		{
			;;
		}
	}

	void complex_httpServer::stop_http_server()
	{
		try
		{
			if (pSeverThread)
			{
                _debug_to(0,("stop http sever begin\n"));
				int i = nthreads - 1;
				while (i >= 0)
				{
					if (pSeverThread[i].event_base)
					{
						event_base_loopbreak(pSeverThread[i].event_base);
						evthread_make_base_notifiable(pSeverThread[i].event_base);
					}
					i--;
				}
				i = nthreads - 1;
                _debug_to(0,("stop http sever base finished\n"));
				while (i >= 0)
				{
					if (pSeverThread[i].threadhandle)
					{
						if (::WaitForSingleObject(pSeverThread[i].threadhandle, 10000) != WAIT_OBJECT_0)
						{
							try
							{
								::TerminateThread(pSeverThread[i].threadhandle, 0);
							}
							catch (...)
							{
								;
							}
                            _debug_to(0,("stop http server timeout ,terminate thread\n"));
						}
						::CloseHandle(pSeverThread[i].threadhandle);
					}
					i--;
				}
                _debug_to(0,("stop http sever thread finished\n"));
				i = nthreads - 1;
				while (i >= 0)
				{
					if (pSeverThread[i].event_base)
					{
						//event_base_loopbreak(pSeverThread[i].event_base);
						event_base_free(pSeverThread[i].event_base);
                        pSeverThread[i].event_base = nullptr;
					}
					i--;		
				}
                _debug_to(0,("stop http sever event base release finished\n"));
				if (nfd > 0)
				{
					shutdown(nfd, SD_BOTH);
					Sleep(100);
				}
				WSACleanup();
                mainWindow = nullptr;
				delete[]pSeverThread;
                pSeverThread = nullptr;
                _debug_to(0,("stop http server finish\n"));
			}
		}
		catch (...)
		{
			;
		}

        if (http_checkEvent[0] != nullptr)
		{
			::CloseHandle(http_checkEvent[0]);
            http_checkEvent[0] = nullptr;
		}

        if (http_checkEvent[1] != nullptr)
		{
			::CloseHandle(http_checkEvent[1]);
            http_checkEvent[1] = nullptr;
		}
	}
};

#else //SYS-LINUX

namespace httpServer
{
    void* GlobalHttpBaseFunc(void * lpParam)
    {
        httpThread *ptd = reinterpret_cast<httpThread*>(lpParam);
		event_base_dispatch(ptd->eventbase);//等待事件被触发，然后调用它们的回调函数
		evhttp_free(ptd->httpserver);//释放以前创建的HTTP服务器
        ptd->httpserver = nullptr;
        return nullptr;
	}

    int httpserver_bindsocket(int port, int backlog)
    {
        int nsfd = socket(AF_INET, SOCK_STREAM, 0);//创建套接字
		if (nsfd < 0) return -1;

		int one = 1;
        socklen_t intLen = static_cast<socklen_t>(sizeof(int));
        int r = setsockopt(nsfd, SOL_SOCKET, SO_REUSEADDR, static_cast<const void*>(&one), intLen);//设置套接字的SO_REUSEADDR属性 执行正确返回0

		struct sockaddr_in addr;
		memset(&addr, 0, sizeof(addr));
        addr.sin_family = static_cast<unsigned short>(AF_INET);
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(static_cast<unsigned short>(port));

        ULONG tempSize = sizeof(addr);
        r = bind(nsfd, reinterpret_cast<struct sockaddr*>(&addr), static_cast<socklen_t>(tempSize));
        if (r < 0) {
            ::close(nsfd);
            return -1;
        }
		r = listen(nsfd, backlog);
        if (r < 0) {
            ::close(nsfd);
            return -1;
        }
		int flags = fcntl(nsfd, F_GETFL, 0);
        if (fcntl(nsfd, F_SETFL, flags | O_NONBLOCK) < 0) {//允许非阻塞模式
            _debug_to(1,("http server fcntl fail\n"));
        }
		return nsfd;
	}

	int complex_httpServer::starthttpserver()
	{
        pSeverThread = new struct httpThread[static_cast<ULONG>(nthreads)];
		evthread_use_pthreads();//开启多线程功能
		int nfd = httpserver_bindsocket(port, backlog);
		if (nfd < 0)
		{
			_debug_to(1,("httpserver_bindsocket\n"));
			return -1;
		}
		for (int i = 0; i < nthreads; i++)
		{
			struct event_base *base = event_base_new();
            if (base == nullptr)
			{
				_debug_to(1,("event_base_new\n"));
				return -1;
			}
			pSeverThread[i].eventbase = base;
			struct evhttp *httpd = evhttp_new(base);
            if (httpd == nullptr)
			{
				_debug_to(1,("evhttp_new\n"));
				return -1;
			}
			pSeverThread[i].httpserver = httpd;
			int r = evhttp_accept_socket(httpd, nfd);
			if (r != 0)
			{
				_debug_to(1,("evhttp_accept_socket\n"));
				return 0;
			}
			evhttp_set_timeout(httpd, 10);
			evhttp_set_gencb(httpd, pFunc, &pSeverThread[i]);
            r = pthread_create(&pSeverThread[i].ths, nullptr, GlobalHttpBaseFunc, &pSeverThread[i]);
			if (r != 0)
			{
				_debug_to(1,("pthread_create\n"));
				return -1;
			}
			//threadt[i] = pSeverThread[i].ths;
		}
        //for (int i = 0; i < nthreads; i++)
        //{
        //    pthread_join(pSeverThread[i].ths, nullptr);
        //}
		return 0;

	}

    int complex_httpServer::start_http_server(http_cb_Func _pFunc, void* _mainWindow, int _port, int httpthreads, int nbacklog, std::string _ipaddr)
	{
		pFunc = _pFunc;
		mainWindow = _mainWindow;
		port = _port;
		nthreads = httpthreads;
        if (nthreads < 1 || nthreads > 1000) nthreads = 10;
		backlog = nbacklog;

        if (!globalIsIPStringValid(_ipaddr))//判断ip是否合法
        {
            ipaddr = ("0.0.0.0");
        }
        else
        {
            ipaddr = _ipaddr;
        }

		if (starthttpserver() == 0)
			return 0;
        else
            return -1;
	}

	void complex_httpServer::stop_http_server()
	{
        /*if (pSeverThread)
		{
			int i = nthreads - 1;
			while (i >= 0)
			{
				if (pSeverThread[i].eventbase)
				{
					event_base_loopbreak(pSeverThread[i].eventbase);
					evthread_make_base_notifiable(pSeverThread[i].eventbase);
				}
				i--;
			}

			i = nthreads - 1;
			while (i >= 0)
			{
				if (pSeverThread[i].eventbase)
				{
					event_base_free(pSeverThread[i].eventbase);
                    pSeverThread[i].eventbase = nullptr;
				}
				i--;
			}
			if (nfd > 0)
			{
				shutdown(nfd, SHUT_RDWR);
				close(nfd);
				sleep(100);
			}

			delete[]pSeverThread;
            pSeverThread = nullptr;
        }*/
        try
        {
            if (pSeverThread)
            {
                _debug_to(0,("stop http sever begin\n"));
                int i = nthreads - 1;
                while (i >= 0)
                {
                    if (pSeverThread[i].eventbase)
                    {
                        event_base_loopbreak(pSeverThread[i].eventbase);
                        evthread_make_base_notifiable(pSeverThread[i].eventbase);
                    }
                    i--;
                }
                i = nthreads - 1;
                _debug_to(0,("stop http sever base finished\n"));
                while (i >= 0)
                {
                    pthread_join(pSeverThread[i].ths, nullptr);
                    i--;
                }
                _debug_to(0,("stop http sever thread finished\n"));
                i = nthreads - 1;
                while (i >= 0)
                {
                    if (pSeverThread[i].eventbase)
                    {
                        event_base_free(pSeverThread[i].eventbase);
                        pSeverThread[i].eventbase = nullptr;
                    }
                    i--;
                }
                _debug_to(0,("stop http sever event base release finished\n"));
                if (nfd > 0)
                {
                    shutdown(nfd, SHUT_RDWR);
                    close(nfd);
                    sleep(100U);
                }
                mainWindow = nullptr;
                delete[]pSeverThread;
                pSeverThread = nullptr;
                _debug_to(0,("stop http server finish\n"));
            }
        }
        catch (...)
        {
            ;
        }
	}
}
#endif
