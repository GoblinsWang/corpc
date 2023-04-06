# cmake最低版本限定
cmake_minimum_required(VERSION 3.6)

project(
    corpc_example
    LANGUAGES C CXX        
)

set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -g -Wall -fPIC -pipe -std=gnu90")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -fPIC -pipe -std=c++11 -fno-exceptions")

# 包含头文件所在的目录
include_directories(${PROJECT_SOURCE_DIR})

# 定义SRC变量，其值为当前目录下所有的源代码文件
aux_source_directory(../corpc/coroutine COR)
aux_source_directory(../corpc/log LOG)
aux_source_directory(../corpc/net NET)
aux_source_directory(../corpc/net/tcp TCP)
aux_source_directory(../corpc/net/http HTTP)
aux_source_directory(../corpc/net/comm COMM)

# test_acceptor
add_executable(test_acceptor test_acceptor.cc ${COR} ${LOG} ${NET} ${TCP} ${HTTP} ${COMM})
target_link_libraries(test_acceptor pthread)

# test_acceptor
add_executable(test_httpserver test_httpserver.cc ${COR} ${LOG} ${NET} ${TCP} ${HTTP} ${COMM})
target_link_libraries(test_httpserver pthread)




