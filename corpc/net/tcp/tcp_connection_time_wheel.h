#ifndef CORPC_NET_TCP_TCPCONNECTIONTIMEWHEEL_H
#define CORPC_NET_TCP_TCPCONNECTIONTIMEWHEEL_H

#include <queue>
#include <vector>
#include "abstract_slot.h"
#include "../common.h"

namespace corpc
{

    class TcpConnection;

    class TcpTimeWheel
    {

    public:
        typedef std::shared_ptr<TcpTimeWheel> ptr;

        typedef AbstractSlot<TcpConnection> TcpConnectionSlot;

        TcpTimeWheel(int bucket_count = 2, int invetal = 5);

        ~TcpTimeWheel();

        void fresh(TcpConnectionSlot::ptr slot);

        void loopFunc();

    private:
        int m_bucket_count{0};
        int m_inteval{0}; // second

        TimerEvent::ptr m_event;
        std::queue<std::vector<TcpConnectionSlot::ptr>> m_wheel;
    };

}

#endif