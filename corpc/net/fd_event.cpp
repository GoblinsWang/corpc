#include <fcntl.h>
#include <unistd.h>
#include "fd_event.h"

namespace corpc
{

    static FdEventContainer *g_FdContainer = nullptr;

    FdEvent::FdEvent(int fd) : m_fd(fd)
    {
    }

    FdEvent::~FdEvent() {}

    void FdEvent::setSocket(corpc::Socket::ptr sock)
    {
        m_socket = sock;
    }

    int FdEvent::getFd() const
    {
        return m_fd;
    }

    void FdEvent::setCoroutine(Coroutine *cor)
    {
        m_coroutine = cor;
    }

    void FdEvent::clearCoroutine()
    {
        m_coroutine = nullptr;
    }

    Coroutine *FdEvent::getCoroutine()
    {
        return m_coroutine;
    }

    FdEvent::ptr FdEventContainer::getFdEvent(int fd)
    {

        m_rwmutex.rlock();
        if (fd < static_cast<int>(m_fds.size()))
        {
            corpc::FdEvent::ptr res = m_fds[fd];
            m_rwmutex.runlock();
            return res;
        }
        m_rwmutex.runlock();

        m_rwmutex.wlock();
        /*
            对文件描述数组进行1.5倍扩容
        */
        int n = (int)(fd * 1.5);
        for (int i = m_fds.size(); i < n; ++i)
        {
            m_fds.push_back(std::make_shared<FdEvent>(i));
        }
        corpc::FdEvent::ptr res = m_fds[fd];
        m_rwmutex.wunlock();
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
            g_FdContainer = new FdEventContainer(1000);
        }
        return g_FdContainer;
    }

}
