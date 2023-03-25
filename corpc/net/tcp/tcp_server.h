// #ifndef CORPC_NET_TCP_TCP_SERVER_H
// #define CORPC_NET_TCP_TCP_SERVER_H

// #include <map>
// #include "../common.h"
// #include "../fd_event.h"
// #include "../abstract_codec.h"
// #include "tcp_connection.h"

// namespace corpc
// {

//     class TcpServer
//     {

//     public:
//         typedef std::shared_ptr<TcpServer> ptr;

//         TcpServer(NetAddress::ptr addr, ProtocalType type = TinyPb_Protocal);

//         ~TcpServer();

//         void start();

//         void addCoroutine(corpc::Coroutine *cor);

//     public:
//         NetAddress::ptr getPeerAddr();

//         NetAddress::ptr getLocalAddr();

//     private:
//         void MainAcceptCorFunc();

//     private:
//         NetAddress::ptr m_addr;

//         TcpAcceptor::ptr m_acceptor;

//         int m_tcp_counts{0};

//         bool m_is_stop_accept{false};

//         Coroutine::ptr m_accept_cor;

//         AbstractCodeC::ptr m_codec;

//         ProtocalType m_protocal_type{TinyPb_Protocal};

//         std::map<int, std::shared_ptr<TcpConnection>> m_clients;
//     };

// }

// #endif
