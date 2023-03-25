#ifndef CORPC_NET_SOCKET_H
#define CORPC_NET_SOCKET_H

#include <arpa/inet.h>
#include <sys/types.h>
#include <string>
#include <unistd.h>
#include <sys/socket.h>
#include <memory>
#include "../coroutine/utils.h"
#include "../coroutine/parameter.h"
#include "../log/logger.h"
#include "net_address.h"

namespace corpc
{
	/*
		Socket类，创建的Socket对象默认都是非阻塞的
		职责：
			1、提供fd操作的相关API
			2、管理fd的生命周期
		其中有引用计数，若某一fd没人用了就会close
	*/
	class Socket
	{
	public:
		using ptr = std::shared_ptr<Socket>;

		// init a listener
		explicit Socket(NetAddress::ptr addr);

		// init a clientfd
		explicit Socket(int fd, NetAddress::ptr addr);

		~Socket();

		void setTcpNoDelay(bool on);

		void setReuseAddr(bool on);

		void setReusePort(bool on);

		// accept a new conn, return connfd;
		int accept();

		// 从socket中读数据
		ssize_t read(void *buf, size_t count);

		// 往socket中写数据
		ssize_t send(const void *buf, size_t count);

		// 关闭套接字的写操作
		int shutdownWrite();

		// 设置是否使用心跳检测
		int setKeepAlive(bool on);

		// 设置socket为非阻塞的
		int setNonBolckSocket();

		// 设置socket为阻塞的
		int setBlockSocket();

		NetAddress::ptr getPeerAddr()
		{
			return m_peer_addr;
		}

		NetAddress::ptr geLocalAddr()
		{
			return m_local_addr;
		}

	private:
		int m_fd;

		NetAddress::ptr m_local_addr{nullptr};
		NetAddress::ptr m_peer_addr{nullptr};
	};

}
#endif