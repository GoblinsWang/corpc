#include <queue>
#include <vector>
#include "tcp_connection.h"
#include "tcp_connection_time_wheel.h"

namespace corpc
{

    TcpTimeWheel::TcpTimeWheel(int bucket_count /*= 10*/, int inteval /*= 10*/)
        : m_bucket_count(bucket_count), m_inteval(inteval)
    {

        for (int i = 0; i < bucket_count; ++i)
        {
            std::vector<TcpConnectionSlot::ptr> tmp;
            m_wheel.push(tmp);
        }

        m_event = std::make_shared<TimerEvent>(m_inteval * 1000, true, std::bind(&TcpTimeWheel::loopFunc, this));
        corpc::Scheduler::getScheduler()->getProcessor(0)->GetTimer()->addTimerEvent(m_event); // 0
    }

    TcpTimeWheel::~TcpTimeWheel()
    {
        corpc::Scheduler::getScheduler()->getProcessor(0)->GetTimer()->delTimerEvent(m_event); // 0
    }

    void TcpTimeWheel::loopFunc()
    {
        LogDebug("pop src bucket");
        m_wheel.pop();
        std::vector<TcpConnectionSlot::ptr> tmp;
        m_wheel.push(tmp);
        LogDebug("push new bucket");
    }

    void TcpTimeWheel::fresh(TcpConnectionSlot::ptr slot)
    {
        LogDebug("fresh connection");
        m_wheel.back().emplace_back(slot);
    }

}