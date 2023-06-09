#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <stdio.h> // snprintf
#include <fcntl.h>
#include <string.h>
#include <sys/epoll.h>
#include "net_socket.h"

namespace corpc
{
	int m_max_connect_timeout = 30; // 30s

	NetSocket::NetSocket(NetAddress::ptr addr)
	{
		m_local_addr = addr;
		m_fd = ::socket(m_local_addr->getFamily(), SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0); // | SOCK_NONBLOCK | SOCK_CLOEXEC
		if (m_fd < 0)
		{
			LogError("start server error. socket error, sys error=" << strerror(errno));
			_exit(0);
		}
		LogDebug("create listenfd succ, listenfd = " << m_fd);

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
		rt = ::listen(m_fd, 4096);
		if (rt != 0)
		{
			LogError("start server error. listen error, fd= " << m_fd << ", errno=" << errno << ", error=" << strerror(errno));
			_exit(0);
		}
	}

	NetSocket::NetSocket(int fd, NetAddress::ptr addr)
	{
		m_fd = fd;
		m_peer_addr = addr;
		if (fd > 0)
		{
			setNonBolckSocket();
		}
	}

	NetSocket::~NetSocket()
	{
		LogTrace("socket over, fd = " << m_fd);
		// ::close(m_fd);
	}

	int NetSocket::accept()
	{
		socklen_t len = 0;
		int rt = 0;
		m_peer_addr = nullptr;
		if (m_local_addr->getFamily() == AF_INET)
		{
			sockaddr_in cli_addr;
			memset(&cli_addr, 0, sizeof(cli_addr));
			len = sizeof(cli_addr);

			int cli_fd = ::accept(m_fd, (struct sockaddr *)&cli_addr, &len);
			if (cli_fd > 0)
			{
				LogInfo("new coming client fd, " << KV(cli_fd));
				m_peer_addr = std::make_shared<IPAddress>(cli_addr);
				return cli_fd;
			}
		}
		else if (m_local_addr->getFamily() == AF_UNIX)
		{
			sockaddr_un cli_addr;
			memset(&cli_addr, 0, sizeof(cli_addr));
			len = sizeof(cli_addr);

			int cli_fd = ::accept(m_fd, (struct sockaddr *)&cli_addr, &len);
			if (cli_fd > 0)
			{
				m_peer_addr = std::make_shared<UnixDomainAddress>(cli_addr);
				return cli_fd;
			}
		}
		else
		{
			LogError("unknown type protocol!");
			close(rt);
			return -1;
		}
		// LogDebug("error, no new client coming, errno=" << errno << "error=" << strerror(errno));
		LogTrace("no new client coming, yield this coroutine, " << KV(m_fd));

		auto fd_event = corpc::FdEventContainer::GetFdContainer()->getFdEvent(m_fd);
		corpc::Scheduler::getScheduler()->getProcessor(threadIdx)->waitEvent(fd_event, m_fd, EPOLLIN | EPOLLPRI | EPOLLRDHUP | EPOLLHUP);

		return accept();
	}

	int NetSocket::connect()
	{
		// try connect
		int n = ::connect(m_fd, reinterpret_cast<sockaddr *>(m_peer_addr->getSockAddr()), m_peer_addr->getSockLen());
		if (n == 0)
		{
			LogDebug("direct connect succ, return");
			return n;
		}
		else if (errno != EINPROGRESS)
		{
			// LogDebug("connect error and errno is't EINPROGRESS, errno=" << errno << ",error=" << strerror(errno));
			return n;
		}

		LogDebug("errno == EINPROGRESS");

		bool is_timeout = false; // 是否超时

		Processor *pro = corpc::Scheduler::getScheduler()->getProcessor(threadIdx); // 当前处理实例
		Coroutine *cur_cor = pro->getCurRunningCo();								// 当前协程
		// 超时函数句柄
		auto timeout_cb = [&is_timeout, pro, cur_cor]()
		{
			// 设置超时标志，然后唤醒协程
			is_timeout = true;
			pro->resume(cur_cor);
		};

		// 注册超时事件
		corpc::TimerEvent::ptr event = std::make_shared<corpc::TimerEvent>(m_max_connect_timeout, false, timeout_cb);
		pro->getTimer()->addTimerEvent(event);

		auto fd_event = corpc::FdEventContainer::GetFdContainer()->getFdEvent(m_fd);
		corpc::Scheduler::getScheduler()->getProcessor(threadIdx)->waitEvent(fd_event, m_fd, EPOLLOUT | EPOLLPRI | EPOLLRDHUP | EPOLLHUP);

		// 注销超时事件
		pro->getTimer()->delTimerEvent(event);

		n = ::connect(m_fd, reinterpret_cast<sockaddr *>(m_peer_addr->getSockAddr()), m_peer_addr->getSockLen());
		if ((n < 0 && errno == EISCONN) || n == 0)
		{
			LogDebug("connect succ");
			return 0;
		}

		if (is_timeout)
		{
			LogError("connect error,  timeout[ " << m_max_connect_timeout << "ms]");
			errno = ETIMEDOUT;
		}

		LogDebug("connect error and errno=" << errno << ", error=" << strerror(errno));
		return -1;
	}

	ssize_t NetSocket::read(void *buf, size_t count)
	{
		auto ret = ::read(m_fd, buf, count);
		if (ret >= 0)
		{
			return ret;
		}
		// if (ret == -1 && errno == EINTR) // 被中断信号（只有阻塞状态才会出现）
		// {
		// 	return read(buf, count);
		// }
		LogTrace("there is no data can read," << KV(m_fd) << ", yield this coroutine");
		auto fd_event = corpc::FdEventContainer::GetFdContainer()->getFdEvent(m_fd);
		corpc::Scheduler::getScheduler()->getProcessor(threadIdx)->waitEvent(fd_event, m_fd, EPOLLIN | EPOLLPRI | EPOLLRDHUP | EPOLLHUP);
		return ::read(m_fd, buf, count);
	}

	ssize_t NetSocket::send(const void *buf, size_t count)
	{
		// 忽略SIGPIPE信号
		// size_t sendIdx = ::send(m_fd, buf, count, MSG_NOSIGNAL);
		size_t sendIdx = ::write(m_fd, buf, count);
		if (sendIdx >= count)
		{
			return count;
		}
		LogTrace("there is no data can send, " << KV(m_fd) << "yield this coroutine");
		auto fd_event = corpc::FdEventContainer::GetFdContainer()->getFdEvent(m_fd);
		corpc::Scheduler::getScheduler()->getProcessor(threadIdx)->waitEvent(fd_event, m_fd, EPOLLOUT);
		return send((char *)buf + sendIdx, count - sendIdx);
	}

	int NetSocket::shutdownWrite()
	{
		int ret = ::shutdown(m_fd, SHUT_WR);
		return ret;
	}

	void NetSocket::setTcpNoDelay(bool on)
	{
		int val = on ? 1 : 0;
		if (::setsockopt(m_fd, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val)) < 0)
		{
			LogError("set TcpNoDelay error");
			_exit(0);
		}
	}

	void NetSocket::setReuseAddr(bool on)
	{
		int val = on ? 1 : 0;
		if (setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) < 0)
		{
			LogError("set REUSEADDR error");
			_exit(0);
		}
	}

	void NetSocket::setReusePort(bool on)
	{
		int val = on ? 1 : 0;
		if (::setsockopt(m_fd, SOL_SOCKET, SO_REUSEPORT, &val, sizeof(val)) < 0)
		{
			LogError("set REUSEPORT error");
			_exit(0);
		}
	}

	int NetSocket::setKeepAlive(bool on)
	{
		int optval = on ? 1 : 0;
		int ret = ::setsockopt(m_fd, SOL_SOCKET, SO_KEEPALIVE,
							   &optval, static_cast<socklen_t>(sizeof optval));
		return ret;
	}

	int NetSocket::setNonBolckSocket()
	{
		auto flags = fcntl(m_fd, F_GETFL, 0);
		int ret = fcntl(m_fd, F_SETFL, flags | O_NONBLOCK); // 设置成非阻塞模式
		return ret;
	}

	int NetSocket::setBlockSocket()
	{
		auto flags = fcntl(m_fd, F_GETFL, 0);
		int ret = fcntl(m_fd, F_SETFL, flags & ~O_NONBLOCK); // 设置成阻塞模式；
		return ret;
	}
}