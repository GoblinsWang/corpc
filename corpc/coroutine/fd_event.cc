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

        m_mutex.rlock(); // 读锁
        corpc::FdEvent::ptr res;
        if (fd < static_cast<int>(m_fds.size()))
        {
            res = m_fds[fd];
            m_mutex.runlock(); // 释放读锁
            return res;
        }
        m_mutex.runlock(); // 读锁

        m_mutex.wlock(); // 写锁
        /*
            对文件描述数组进行1.5倍扩容
        */
        int n = (int)(fd * 1.5);
        for (int i = m_fds.size(); i < n; ++i)
        {
            m_fds.push_back(std::make_shared<FdEvent>(i));
        }
        res = m_fds[fd];
        m_mutex.wunlock(); // 写锁
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
