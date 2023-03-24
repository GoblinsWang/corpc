#include <fcntl.h>
#include <unistd.h>
#include "fd_event.h"

namespace corpc
{

    static FdEventContainer *g_FdContainer = nullptr;

    FdEvent::FdEvent(corpc::Processor *processor, int fd /*=-1*/) : m_fd(fd), m_processor(processor)
    {
        if (processor == nullptr)
        {
            LogError("create processor first");
        }
        // assert(processor != nullptr);
    }

    FdEvent::FdEvent(int fd) : m_fd(fd)
    {
    }

    FdEvent::~FdEvent() {}

    void FdEvent::handleEvent(int flag)
    {

        if (flag == READ)
        {
            m_read_callback();
        }
        else if (flag == WRITE)
        {
            m_write_callback();
        }
        else
        {
            LogError("error flag");
        }
    }

    void FdEvent::setCallBack(IOEvent flag, std::function<void()> cb)
    {
        if (flag == READ)
        {
            m_read_callback = cb;
        }
        else if (flag == WRITE)
        {
            m_write_callback = cb;
        }
        else
        {
            LogError("error flag");
        }
    }

    std::function<void()> FdEvent::getCallBack(IOEvent flag) const
    {
        if (flag == READ)
        {
            return m_read_callback;
        }
        else if (flag == WRITE)
        {
            return m_write_callback;
        }
        return nullptr;
    }

    void FdEvent::addListenEvents(IOEvent event)
    {
        if (m_listen_events & event)
        {
            LogDebug("already has this event, skip");
            return;
        }
        m_listen_events |= event;
        updateToProcessor();
    }

    void FdEvent::delListenEvents(IOEvent event)
    {
        if (m_listen_events & event)
        {

            LogDebug("delete succ");
            m_listen_events &= ~event;
            updateToProcessor();
            return;
        }
        LogDebug("this event not exist, skip");
    }

    void FdEvent::updateToProcessor()
    {

        epoll_event event;
        event.events = m_listen_events;
        event.data.ptr = this;

        if (!m_processor)
        {
            m_processor = corpc::Processor::GetProcessor();
        }

        m_processor->addEvent(m_fd, event);
    }

    void FdEvent::unregisterFromProcessor()
    {
        if (!m_processor)
        {
            m_processor = corpc::Processor::GetProcessor();
        }
        m_processor->delEvent(m_fd);
        m_listen_events = 0;
        m_read_callback = nullptr;
        m_write_callback = nullptr;
    }

    int FdEvent::getFd() const
    {
        return m_fd;
    }

    void FdEvent::setFd(const int fd)
    {
        m_fd = fd;
    }

    int FdEvent::getListenEvents() const
    {
        return m_listen_events;
    }

    Processor *FdEvent::getProcessor() const
    {
        return m_processor;
    }

    void FdEvent::setProcessor(Processor *pro)
    {
        m_processor = pro;
    }

    void FdEvent::setNonBlock()
    {
        if (m_fd == -1)
        {
            LogError("error, fd=-1");
            return;
        }

        int flag = fcntl(m_fd, F_GETFL, 0);
        if (flag & O_NONBLOCK)
        {
            LogDebug("fd already set o_nonblock");
            return;
        }

        fcntl(m_fd, F_SETFL, flag | O_NONBLOCK);
        flag = fcntl(m_fd, F_GETFL, 0);
        if (flag & O_NONBLOCK)
        {
            LogDebug("succ set o_nonblock");
        }
        else
        {
            LogError("set o_nonblock error");
        }
    }

    bool FdEvent::isNonBlock()
    {
        if (m_fd == -1)
        {
            LogError("error, fd=-1");
            return false;
        }
        int flag = fcntl(m_fd, F_GETFL, 0);
        return (flag & O_NONBLOCK);
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
