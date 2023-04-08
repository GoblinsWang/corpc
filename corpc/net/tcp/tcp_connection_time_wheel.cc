#include <queue>
#include <vector>
#include "tcp_connection_time_wheel.h"

namespace corpc
{

    TcpTimeWheel::TcpTimeWheel(int bucket_count /*= 3*/, int inteval /*= 5*/)
        : m_bucket_count(bucket_count), m_inteval(inteval)
    {
        // initial wheel slots
        for (int i = 0; i < bucket_count; ++i)
        {
            std::vector<TcpConnectionSlot::ptr> tmp;
            m_wheel.emplace_back(tmp);
        }

        m_event = std::make_shared<TimerEvent>(m_inteval * 1000, true, std::bind(&TcpTimeWheel::loopFunc, this));
        corpc::Scheduler::getScheduler()->getProcessor(0)->getTimer()->addTimerEvent(m_event); // 0
    }

    TcpTimeWheel::~TcpTimeWheel()
    {
        corpc::Scheduler::getScheduler()->getProcessor(0)->getTimer()->delTimerEvent(m_event); // 0
    }

    void TcpTimeWheel::fresh(TcpConnectionSlot::ptr slot)
    {
        // LogDebug("fresh connection");
        int index = (m_cur_bucket - 1 + m_bucket_count) % m_bucket_count;
        m_wheel[index].emplace_back(slot);
    }

    void TcpTimeWheel::loopFunc()
    {
        // LogDebug("clear bucket size: " << m_wheel[m_cur_bucket].size() << ", index = " << m_cur_bucket);
        m_wheel[m_cur_bucket].clear();
        m_cur_bucket = (m_cur_bucket + 1) % m_bucket_count;
    }

    // void TcpTimeWheel::addConnection(int fd)
    // {
    //     LogDebug("add connection");
    //     int index = (m_cur_bucket - 1 + m_bucket_count) % m_bucket_count;
    //     // m_wheel[index].insert({fd, slot});
    //     m_wheel[index].insert(fd);
    //     m_conn_map[fd] = index; // update map
    // }

    // void TcpTimeWheel::updateConnection(int fd)
    // {
    //     LogDebug("update connection");
    //     int old_index = m_conn_map[fd];
    //     int new_index = (m_cur_bucket - 1 + m_bucket_count) % m_bucket_count;
    //     if (old_index != new_index)
    //     {
    //         m_wheel[old_index].erase(fd);
    //         // m_wheel[new_index].insert({fd, slot});
    //         m_wheel[new_index].insert(fd);
    //     }
    // }

}