#ifndef CORPC_NET_TCP_TCP_SERVER_H
#define CORPC_NET_TCP_TCP_SERVER_H

#include <map>
#include "tcp_connection.h"
#include "tcp_acceptor.h"
#include "../common.h"
#include "../net_socket.h"
#include "../fd_event.h"
#include "../abstract_codec.h"
#include "../abstract_dispatcher.h"
#include "../http/http_dispatcher.h"
#include "../http/http_servlet.h"

namespace corpc
{

    class TcpServer
    {

    public:
        typedef std::shared_ptr<TcpServer> ptr;

        TcpServer(NetAddress::ptr addr, ProtocalType type = TinyPb_Protocal);

        ~TcpServer();

        void start();

        TcpConnection::ptr addClient(NetSocket::ptr net_sock);

        // bool registerService(std::shared_ptr<google::protobuf::Service> service);

        bool registerHttpServlet(const std::string &url_path, HttpServlet::ptr servlet);

    public:
        AbstractDispatcher::ptr getDispatcher();

        AbstractCodeC::ptr getCodec();

    private:
        void MainAcceptCorFunc();

    private:
        NetAddress::ptr m_addr;

        TcpAcceptor::ptr m_acceptor;

        int m_tcp_counts{0};

        bool m_is_stop_accept{false};

        AbstractDispatcher::ptr m_dispatcher;

        AbstractCodeC::ptr m_codec;

        ProtocalType m_protocal_type{TinyPb_Protocal};

        std::map<int, std::shared_ptr<TcpConnection>> m_clients;
    };

}

#endif
