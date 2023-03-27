#ifndef CORPC_COROUTINE_LOCK_FREE_RING_BUFFER_H
#define CORPC_COROUTINE_LOCK_FREE_RING_BUFFER_H

#include <atomic>
#include <cstddef>
namespace corpc
{

    template <typename T, size_t Capacity>
    class LockFreeRingBuffer
    {
    public:
        LockFreeRingBuffer() : m_read_pos(0), m_write_pos(0) {}

        bool push(const T &item)
        {
            size_t current_write_pos = m_write_pos.load(std::memory_order_relaxed);
            size_t next_write_pos = increment_pos(current_write_pos);

            if (next_write_pos == m_read_pos.load(std::memory_order_acquire))
            {
                return false; // Buffer is full
            }

            m_buffer[current_write_pos] = item;
            m_write_pos.store(next_write_pos, std::memory_order_release);
            return true;
        }

        bool pop(T &item)
        {
            size_t current_read_pos = m_read_pos.load(std::memory_order_relaxed);

            if (current_read_pos == m_write_pos.load(std::memory_order_acquire))
            {
                return false; // Buffer is empty
            }

            item = m_buffer[current_read_pos];
            m_read_pos.store(increment_pos(current_read_pos), std::memory_order_release);
            return true;
        }

    private:
        size_t increment_pos(size_t pos) const
        {
            return (pos + 1) % Capacity;
        }

        std::atomic<size_t> m_read_pos;
        std::atomic<size_t> m_write_pos;
        T m_buffer[Capacity];
    };
}
#endif
