#ifndef CORPC_NET_FD_EVNET_H
#define CORPC_NET_FD_EVNET_H

#include <functional>
#include <memory>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <assert.h>
#include <mutex>
#include "../coroutine/processor.h"
#include "../log/logger.h"
#include "../coroutine/coroutine.h"
#include "../coroutine/rw_mutex.h"

namespace corpc
{

    class Processor;

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

        FdEvent(corpc::Processor *processor, int fd = -1);

        FdEvent(int fd);

        virtual ~FdEvent();

        void handleEvent(int flag);

        void setCallBack(IOEvent flag, std::function<void()> cb);

        std::function<void()> getCallBack(IOEvent flag) const;

        void addListenEvents(IOEvent event);

        void delListenEvents(IOEvent event);

        void updateToProcessor();

        void unregisterFromProcessor();

        int getFd() const;

        void setFd(const int fd);

        int getListenEvents() const;

        Processor *getProcessor() const;

        void setProcessor(Processor *r);

        void setNonBlock();

        bool isNonBlock();

        void setCoroutine(Coroutine *cor);

        Coroutine *getCoroutine();

        void clearCoroutine();

    public:
        std::mutex m_mutex;

    protected:
        int m_fd{-1};
        std::function<void()> m_read_callback;
        std::function<void()> m_write_callback;

        int m_listen_events{0};

        Processor *m_processor{nullptr};

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
        RWMutex m_rwmutex;
        std::vector<FdEvent::ptr> m_fds;
    };

}

#endif
