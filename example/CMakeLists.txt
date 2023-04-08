# cmake最低版本限定
cmake_minimum_required(VERSION 3.6)

project(
    corpc_example
    LANGUAGES C CXX        
)

set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -g  -Wall -fPIC -pipe -std=gnu90")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g  -Wall -fPIC -pipe -std=c++11 -fno-exceptions")

# 包含头文件所在的目录
include_directories(${PROJECT_SOURCE_DIR})

# 定义SRC变量，其值为当前目录下所有的源代码文件
aux_source_directory(../corpc/coroutine COR)
aux_source_directory(../corpc/log LOG)
aux_source_directory(../corpc/net NET)
aux_source_directory(../corpc/net/tcp TCP)
aux_source_directory(../corpc/net/http HTTP)
aux_source_directory(../corpc/net/comm COMM)
aux_source_directory(../corpc/net/pb PB)

find_package(Protobuf REQUIRED)
if (${PROTOBUF_FOUND})
    message("protobuf found")
else ()
    message(FATAL_ERROR "Cannot find Protobuf")
endif ()

set(LIBS
    ${Protobuf_LIBRARY}
    pthread
)

# test_acceptor
add_executable(test_acceptor test_acceptor.cc ${COR} ${LOG} ${NET} ${TCP} ${HTTP} ${COMM} ${PB})
target_link_libraries(test_acceptor ${LIBS})

# test_http_server
set(
    TEST_HTTP_SERVER
    test_http_server.cc
    test_pb_server.pb.cc
)
add_executable(test_http_server ${TEST_HTTP_SERVER} ${COR} ${LOG} ${NET} ${TCP} ${HTTP} ${COMM} ${PB})
target_link_libraries(test_http_server ${LIBS})

# test_pb_server
set(
    TEST_PB_SERVER
    test_pb_server.cc
    test_pb_server.pb.cc
)

add_executable(test_pb_server ${TEST_PB_SERVER} ${COR} ${LOG} ${NET} ${TCP} ${HTTP} ${COMM} ${PB})
target_link_libraries(test_pb_server ${LIBS})

# test_pb_server_client
set(
    TEST_PB_SERVER_CLIENT
    test_pb_server_client.cc
    test_pb_server.pb.cc
)

add_executable(test_pb_server_client ${TEST_PB_SERVER_CLIENT} ${COR} ${LOG} ${NET} ${TCP} ${HTTP} ${COMM} ${PB})
target_link_libraries(test_pb_server_client ${LIBS})


