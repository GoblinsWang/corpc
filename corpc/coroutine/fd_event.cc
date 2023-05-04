#include "fd_event.h"

namespace corpc
{

	static FdEventContainer *g_FdContainer = nullptr;

	FdEvent::FdEvent(int fd) : m_fd(fd)
	{
	}

	FdEvent::~FdEvent() {}

	FdEvent::ptr FdEventContainer::getFdEvent(int fd)
	{

		// readlock
		m_mutex.rlock();
		corpc::FdEvent::ptr res;
		if (fd < static_cast<int>(m_fds.size()))
		{
			res = m_fds[fd];
			// release readlock
			m_mutex.runlock();
			return res;
		}
		// release readlock
		m_mutex.runlock();

		// writelock
		m_mutex.wlock();

		// Expand the file description array by 1.5 times.
		int n = (int)(fd * 1.5);
		for (int i = m_fds.size(); i < n; ++i)
		{
			m_fds.push_back(std::make_shared<FdEvent>(i));
		}

		res = m_fds[fd];
		// release writelock
		m_mutex.wunlock();

		return res;
	}

	FdEventContainer::FdEventContainer(int size)
	{
		for (int i = 0; i < size; ++i)
		{
			m_fds.push_back(std::make_shared<FdEvent>(i));
		}
	}

	FdEventContainer *FdEventContainer::GetFdContainer()
	{
		if (g_FdContainer == nullptr)
		{
			g_FdContainer = new FdEventContainer(2000);
		}
		return g_FdContainer;
	}

}
