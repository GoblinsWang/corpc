#include "socket.h"
#include "../coroutine/scheduler.h"

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <stdio.h> // snprintf
#include <fcntl.h>
#include <string.h>
#include <sys/epoll.h>

namespace corpc
{
	Socket::Socket(NetAddress::ptr addr)
	{
		m_local_addr = addr;
		m_fd = ::socket(m_local_addr->getFamily(), SOCK_STREAM, 0);
		if (m_fd < 0)
		{
			LogError("start server error. socket error, sys error=" << strerror(errno));
			_exit(0);
		}
		LogDebug("create listenfd succ, listenfd = " << m_fd);

		// 设置为非阻塞模式
		// setNonBolckSocket();

		int val = 1;
		if (setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) < 0)
		{
			LogError("set REUSEADDR error");
			_exit(0);
		}

		socklen_t len = m_local_addr->getSockLen();
		int rt = ::bind(m_fd, m_local_addr->getSockAddr(), len);
		if (rt != 0)
		{
			LogError("start server error. bind error, errno=" << errno << ", error=" << strerror(errno));
			_exit(0);
		}

		LogDebug("set REUSEADDR succ");
		rt = ::listen(m_fd, 10);
		if (rt != 0)
		{
			LogError("start server error. listen error, fd= " << m_fd << ", errno=" << errno << ", error=" << strerror(errno));
			_exit(0);
		}
	}

	Socket::Socket(int fd, NetAddress::ptr addr)
	{
		m_fd = fd;
		m_local_addr = addr;
		if (fd > 0)
		{
			setNonBolckSocket();
		}
	}

	Socket::~Socket()
	{
		::close(m_fd);
	}

	int Socket::accept()
	{
		socklen_t len = 0;
		int rt = 0;

		if (m_local_addr->getFamily() == AF_INET)
		{
			sockaddr_in cli_addr;
			memset(&cli_addr, 0, sizeof(cli_addr));
			len = sizeof(cli_addr);

			int cli_fd = ::accept(m_fd, (struct sockaddr *)&cli_addr, &len);
			m_peer_addr = std::make_shared<IPAddress>(cli_addr);
			if (cli_fd > 0)
				return cli_fd;
		}
		else if (m_local_addr->getFamily() == AF_UNIX)
		{
			sockaddr_un cli_addr;
			memset(&cli_addr, 0, sizeof(cli_addr));
			len = sizeof(cli_addr);

			int cli_fd = ::accept(m_fd, (struct sockaddr *)&cli_addr, &len);
			m_peer_addr = std::make_shared<UnixDomainAddress>(cli_addr);
			if (cli_fd > 0)
				return cli_fd;
		}
		else
		{
			LogError("unknown type protocol!");
			close(rt);
			return -1;
		}
		LogDebug("error, no new client coming, errno=" << errno << "error=" << strerror(errno));

		corpc::Scheduler::getScheduler()->getProcessor(threadIdx)->waitEvent(m_fd, EPOLLIN | EPOLLPRI | EPOLLRDHUP | EPOLLHUP);

		return accept();
	}

	// 从socket中读数据
	ssize_t Socket::read(void *buf, size_t count)
	{
		auto ret = ::read(m_fd, buf, count);
		if (ret >= 0)
		{
			return ret;
		}
		if (ret == -1 && errno == EINTR) // 被中断信号（只有阻塞状态才会出现）
		{
			return read(buf, count);
		}
		corpc::Scheduler::getScheduler()->getProcessor(threadIdx)->waitEvent(m_fd, EPOLLIN | EPOLLPRI | EPOLLRDHUP | EPOLLHUP);
		return ::read(m_fd, buf, count);
	}

	// 往socket中写数据
	ssize_t Socket::send(const void *buf, size_t count)
	{
		// 忽略SIGPIPE信号
		size_t sendIdx = ::send(m_fd, buf, count, MSG_NOSIGNAL);
		if (sendIdx >= count)
		{
			return count;
		}
		corpc::Scheduler::getScheduler()->getProcessor(threadIdx)->waitEvent(m_fd, EPOLLOUT);
		return send((char *)buf + sendIdx, count - sendIdx);
	}

	int Socket::shutdownWrite()
	{
		int ret = ::shutdown(m_fd, SHUT_WR);
		return ret;
	}

	void Socket::setTcpNoDelay(bool on)
	{
		int val = on ? 1 : 0;
		if (::setsockopt(m_fd, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val)) < 0)
		{
			LogError("set TcpNoDelay error");
			_exit(0);
		}
	}

	void Socket::setReuseAddr(bool on)
	{
		int val = on ? 1 : 0;
		if (setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) < 0)
		{
			LogError("set REUSEADDR error");
			_exit(0);
		}
	}

	void Socket::setReusePort(bool on)
	{
		int val = on ? 1 : 0;
		if (::setsockopt(m_fd, SOL_SOCKET, SO_REUSEPORT, &val, sizeof(val)) < 0)
		{
			LogError("set REUSEPORT error");
			_exit(0);
		}
	}

	int Socket::setKeepAlive(bool on)
	{
		int optval = on ? 1 : 0;
		int ret = ::setsockopt(m_fd, SOL_SOCKET, SO_KEEPALIVE,
							   &optval, static_cast<socklen_t>(sizeof optval));
		return ret;
	}

	// 设置socket为非阻塞的
	int Socket::setNonBolckSocket()
	{
		auto flags = fcntl(m_fd, F_GETFL, 0);
		int ret = fcntl(m_fd, F_SETFL, flags | O_NONBLOCK); // 设置成非阻塞模式
		return ret;
	}

	// 设置socket为阻塞的
	int Socket::setBlockSocket()
	{
		auto flags = fcntl(m_fd, F_GETFL, 0);
		int ret = fcntl(m_fd, F_SETFL, flags & ~O_NONBLOCK); // 设置成阻塞模式；
		return ret;
	}
}