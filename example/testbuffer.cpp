#include <iostream>
#include <thread>
#include "../corpc/coroutine/lock_free_ring_buffer.h"
using namespace corpc;
LockFreeRingBuffer<int, 10> buffer;

void producer()
{
    for (int i = 0; i < 20; ++i)
    {
        if (!buffer.push(i))
        {
            std::cout << "Buffer is full, " << i << std::endl;
        }
    }
}

void consumer()
{
    int item;
    for (int i = 0; i < 20; ++i)
    {
        if (buffer.pop(item))
        {
            std::cout << "Consumed: " << item << std::endl;
        }
        else
        {
            std::cout << "Buffer is empty" << std::endl;
        }
    }
}

int main()
{
    std::thread producer_thread(producer);
    std::thread consumer_thread(consumer);

    producer_thread.join();
    consumer_thread.join();

    return 0;
}
