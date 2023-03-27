/***
	@author: Wangzhiming
	@date: 2022-10-29
***/
#include <iostream>
#include <thread>
#include <sys/sysinfo.h>
#include "../corpc/coroutine/scheduler.h"
#include "../corpc/coroutine/processor.h"
#include "../corpc/coroutine/co_api.h"
#include "../corpc/coroutine/rw_mutex.h"
#include "../corpc/log/logger.h"
#include "../corpc/net/tcp/tcp_acceptor.h"
#include "../corpc/coroutine/co_api.h"
#include "../corpc/coroutine/parameter.h"

void testCoroutine()
{
	corpc::co_go(
		[]()
		{
			auto net_addr = std::make_shared<corpc::IPAddress>(8888);
			auto ser = std::make_shared<corpc::TcpAcceptor>(net_addr);
			while (1)
			{
				auto conn = ser->toAccept();
				conn->setTcpNoDelay(true);
				corpc::co_go([conn]
							 {
								 std::string hello("HTTP/1.0 200 OK\r\nServer: cppCo/0.1.0\r\nContent-Length: 72\r\nContent-Type: text/html\r\n\r\n<HTML><TITLE>hello</TITLE>\r\n<BODY><P>hello word!\r\n</BODY></HTML>\r\n");
								 // std::string hello("<HTML><TITLE>hello</TITLE>\r\n<BODY><P>hello word!\r\n</BODY></HTML>\r\n");
								 char buf[1024];
								 std::cout << "----------" << std::endl;
								 if (conn->read((void *)buf, 1024) > 0)
								 {
									 conn->send(hello.c_str(), hello.size());
									 corpc::co_sleep(50); // 需要等一下，否则还没发送完毕就关闭了
								 } });
			}
		},
		corpc::parameter::coroutineStackSize, 0); // 0为监听的主线程

	// corpc::co_go(
	// 	[]()
	// 	{
	// 		cout << "hello world" << endl;
	// 	},
	// 	corpc::parameter::coroutineStackSize, 0); // 程序会为它分配除0以外的线程
}

int main()
{
	testCoroutine();
	corpc::sche_join();

	return 0;
}
