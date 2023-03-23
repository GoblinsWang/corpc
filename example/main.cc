/***
	@author: Wangzhiming
	@date: 2022-10-29
***/
#include <iostream>
#include <sys/sysinfo.h>

#include "../corpc/coroutine/processor.h"
#include "../corpc/coroutine/co_api.h"
#include "../corpc/coroutine/socket.h"
#include "../corpc/coroutine/mutex.h"
#include "../corpc/log/logger.h"

using namespace corpc;

// corpc http response with one acceptor test
// 只有一个acceptor的服务
void single_acceptor_server_test()
{
	corpc::co_go(
		[]
		{
			corpc::Socket listener;
			if (listener.isUseful())
			{
				listener.setTcpNoDelay(true);
				listener.setReuseAddr(true);
				listener.setReusePort(true);
				if (listener.bind(8888) < 0)

				{
					return;
				}
				listener.listen();
			}
			while (1)
			{
				corpc::Socket *conn = new corpc::Socket(listener.accept());
				conn->setTcpNoDelay(true);
				corpc::co_go(
					[conn]
					{
						std::vector<char> buf;
						buf.resize(2048);
						while (1)
						{
							auto readNum = conn->read((void *)&(buf[0]), buf.size());
							std::string ok = "HTTP/1.0 200 OK\r\nServer: corpc/0.1.0\r\nContent-Type: text/html\r\n\r\n";
							if (readNum < 0)
							{
								break;
							}
							conn->send(ok.c_str(), ok.size());
							conn->send((void *)&(buf[0]), readNum);
							if (readNum < (int)buf.size())
							{
								break;
							}
						}
						corpc::co_sleep(100); // 需要等一下，否则还没发送完毕就关闭了
						delete conn;
					});
			}
		});
}

// corpc http response with multi acceptor test
// 每条线程一个acceptor的服务
void multi_acceptor_server_test()
{
	auto tCnt = ::get_nprocs_conf();
	for (int i = 0; i < tCnt; ++i)
	{
		corpc::co_go(
			[i]
			{
				corpc::Socket listener;
				if (listener.isUseful())
				{
					listener.setTcpNoDelay(true);
					listener.setReuseAddr(true);
					listener.setReusePort(true);
					if (listener.bind(8888) < 0)
					{
						return;
					}
					listener.listen();
				}
				LogDebug(" 初始化完毕，正在监听" << i);
				while (1)
				{
					corpc::Socket *conn = new corpc::Socket(listener.accept());
					LogDebug("accept " << i);
					conn->setTcpNoDelay(true);
					corpc::co_go(
						[conn]
						{
							std::string hello("HTTP/1.0 200 OK\r\nServer: corpc/0.1.0\r\nContent-Length: 72\r\nContent-Type: text/html\r\n\r\n<HTML><TITLE>hello</TITLE>\r\n<BODY><P>hello word!\r\n</BODY></HTML>\r\n");
							// std::string hello("<HTML><TITLE>hello</TITLE>\r\n<BODY><P>hello word!\r\n</BODY></HTML>\r\n");
							char buf[1024];
							if (conn->read((void *)buf, 1024) > 0)
							{
								// LogDebug(buf);
								conn->send(hello.c_str(), hello.size());
								corpc::co_sleep(50); // 需要等一下，否则还没发送完毕就关闭了
							}
							delete conn;
						});
				}
			},
			parameter::coroutineStackSize, i);
	}
}

// 作为客户端的测试，可配合上述server测试
void client_test()
{
	corpc::co_go(
		[]
		{
			char buf[1024];
			while (1)
			{
				corpc::co_sleep(2000);
				corpc::Socket s;
				s.connect("127.0.0.1", 8099);
				s.send("ping", 4);
				s.read(buf, 1024);
				std::cout << std::string(buf) << std::endl;
			}
		});
}

// 读写锁测试
void mutex_test(corpc::RWMutex &mu)
{
	for (int i = 0; i < 10; ++i)
		if (i < 5)
		{
			corpc::co_go(
				[&mu, i]
				{
					mu.rlock();
					std::cout << i << " : start reading" << std::endl;
					corpc::co_sleep(50 + i);
					std::cout << i << " : finish reading" << std::endl;
					mu.runlock();
					mu.wlock();
					std::cout << i << " : start writing" << std::endl;
					corpc::co_sleep(50);
					std::cout << i << " : finish writing" << std::endl;
					mu.wunlock();
				});
		}
		else
		{
			corpc::co_go(
				[&mu, i]
				{
					mu.wlock();
					std::cout << i << " : start writing" << std::endl;
					corpc::co_sleep(50);
					std::cout << i << " : finish writing" << std::endl;
					mu.wunlock();
					mu.rlock();
					std::cout << i << " : start reading" << std::endl;
					corpc::co_sleep(50);
					std::cout << i << " : finish reading" << std::endl;
					mu.runlock();
				});
		}
}

int main()
{
	corpc::RWMutex mu;
	// mutex_test(mu);
	// single_acceptor_server_test();
	multi_acceptor_server_test();
	// client_test();
	corpc::sche_join();
	std::cout << "end" << std::endl;
	return 0;
}
