#ifndef CORPC_COROUTINE_FD_EVNET_H
#define CORPC_COROUTINE_FD_EVNET_H

#include <functional>
#include <memory>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <assert.h>
#include "coroutine.h"
#include "rw_mutex.h"
#include "../log/logger.h"

namespace corpc
{
    enum IOEvent
    {
        READ = EPOLLIN,
        WRITE = EPOLLOUT,
        ETModel = EPOLLET,
    };

    class FdEvent : public std::enable_shared_from_this<FdEvent>
    {
    public:
        using ptr = std::shared_ptr<FdEvent>;

        FdEvent(int fd);

        virtual ~FdEvent();

    public:
        inline int getFd()
        {
            return m_fd;
        }

        inline void setCoroutine(Coroutine *cor)
        {
            m_coroutine = cor;
        }

        inline Coroutine *getCoroutine()
        {
            return m_coroutine;
        }

        inline void clearCoroutine()
        {
            m_coroutine = nullptr;
        }

    protected:
        int m_fd{-1};

        Coroutine *m_coroutine{nullptr};
    };

    class FdEventContainer
    {

    public:
        FdEventContainer(int size);

        FdEvent::ptr getFdEvent(int fd);

    public:
        static FdEventContainer *GetFdContainer();

    private:
        RWMutex m_mutex;

        std::vector<FdEvent::ptr> m_fds;
    };

}

#endif
