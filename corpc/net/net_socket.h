#ifndef CORPC_NET_NET_SOCKET_H
#define CORPC_NET_NET_SOCKET_H

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

	class NetSocket : public std::enable_shared_from_this<NetSocket>
	{
	public:
		using ptr = std::shared_ptr<NetSocket>;

		// init a listener
		explicit NetSocket(NetAddress::ptr addr);

		// init a clientfd
		explicit NetSocket(int fd, NetAddress::ptr addr);

		~NetSocket();

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

		inline NetAddress::ptr getPeerAddr()
		{
			return m_peer_addr;
		}

		inline NetAddress::ptr getLocalAddr()
		{
			return m_local_addr;
		}

		inline int getFd()
		{
			return m_fd;
		}

	private:
		int m_fd;

		NetAddress::ptr m_local_addr{nullptr};
		NetAddress::ptr m_peer_addr{nullptr};
	};

}
#endif