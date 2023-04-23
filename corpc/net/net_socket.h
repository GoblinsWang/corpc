#ifndef CORPC_NET_NET_SOCKET_H
#define CORPC_NET_NET_SOCKET_H

#include <arpa/inet.h>
#include <sys/types.h>
#include <string>
#include <unistd.h>
#include <sys/socket.h>
#include <memory>
#include "common.h"
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

		// close socket write operations
		int shutdownWrite();

		void setTcpNoDelay(bool on);

		void setReuseAddr(bool on);

		void setReusePort(bool on);

		// set whether to use the activation mechanism
		int setKeepAlive(bool on);

		// set socket to non blocking
		int setNonBolckSocket();

		// Set socket to blocked
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

	public:
		/*
			The following functions have been modified to the native functions to adapt to the coroutine framework
		*/
		// accept a new conn, return connfd;
		int accept();

		// for client to connect remote server
		int connect();

		// read data from socket
		ssize_t read(void *buf, size_t count);

		// write data to socket
		ssize_t send(const void *buf, size_t count);

	private:
		int m_fd;

		NetAddress::ptr m_local_addr{nullptr};
		NetAddress::ptr m_peer_addr{nullptr};
	};

}
#endif