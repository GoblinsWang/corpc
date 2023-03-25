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
				int cli_fd = ser->toAccept();
				cout << "recv info from cli_fd = " << cli_fd << endl;
				close(cli_fd);
			}
		},
		corpc::parameter::coroutineStackSize, 0); // 0为监听的主线程

	corpc::co_go(
		[]()
		{
			cout << "hello world1" << endl;
		}); // 程序会为它分配除0以外的线程
	corpc::co_go(
		[]()
		{
			cout << "hello world" << endl;
		}); // 程序会为它分配除0以外的线程
}

int main()
{
	testCoroutine();
	corpc::sche_join();

	return 0;
}
