#ifndef CORPC_NET_TCP_TCPCONNECTIONTIMEWHEEL_H
#define CORPC_NET_TCP_TCPCONNECTIONTIMEWHEEL_H

#include <queue>
#include <vector>
#include <set>
#include <unordered_map>
#include "tcp_connection.h"
#include "abstract_slot.h"
#include "../common.h"

namespace corpc
{

    class TcpTimeWheel
    {

    public:
        typedef std::shared_ptr<TcpTimeWheel> ptr;

        typedef AbstractSlot<TcpConnection> TcpConnectionSlot;

        TcpTimeWheel(int bucket_count = 10, int interval = 6);

        ~TcpTimeWheel();

        void fresh(TcpConnectionSlot::ptr slot);

        // // void addConnection(int fd, TcpConnection::weakptr slot);
        // void addConnection(int fd);

        // // void updateConnection(int fd, TcpConnection::weakptr slot);
        // void updateConnection(int fd);

        void loopFunc();

    public:
        inline int64_t getInterval()
        {
            return m_inteval * 1000;
        }

    private:
        int m_bucket_count{0};
        int m_inteval{0}; // second

        int m_cur_bucket{0}; // for loopFunc

        TimerEvent::ptr m_event;

        std::vector<std::vector<TcpConnectionSlot::ptr>> m_wheel;
    };

}

#endif