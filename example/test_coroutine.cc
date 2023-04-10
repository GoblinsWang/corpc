#include <iostream>
#include "../corpc/coroutine/co_api.h"

int test()
{
    std::cout << "hello world" << std::endl;
    return 0;
}

int main(int argc, char *argv[])
{
    corpc::co_go(test);
    corpc::sche_join();

    return 0;
}