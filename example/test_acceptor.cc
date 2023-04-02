/***
	@author: Wangzhiming
	@date: 2022-10-29
***/
#include <iostream>
#include "../corpc/net/tcp/tcp_acceptor.h"
#include "../corpc/coroutine/co_api.h"
#include "../corpc/coroutine/parameter.h"
#include "../corpc/log/logger.h"
using namespace corpc;

void testAcceptor()
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
								 std::string hello("HTTP/1.0 200 OK\r\nServer: cppCo/0.1.0\r\nContent-Length: 72\r\nContent-Type: text/html\r\n\r\n<HTML><TITLE>hello</TITLE>\r\n<BODY><P>hello word!\r\n<P></BODY></HTML>\r\n");
								 // std::string hello("<HTML><TITLE>hello</TITLE>\r\n<BODY><P>hello word!\r\n</BODY></HTML>\r\n");
								 char buf[1024];
								 LogInfo("------------------------");
								 if (conn->read((void *)buf, 1024) > 0)
								 {
									 conn->send(hello.c_str(), hello.size());
									 corpc::co_sleep(50); // 需要等一下，否则还没发送完毕就关闭了
								 } });
			}
		},
		corpc::parameter::coroutineStackSize, 0); // 0为监听的主线程
}

int main()
{
	testAcceptor();
	corpc::sche_join();

	return 0;
}
