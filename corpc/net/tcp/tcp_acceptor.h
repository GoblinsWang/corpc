#ifndef CORPC_NET_TCP_TCP_ACCEPTOR_H
#define CORPC_NET_TCP_TCP_ACCEPTOR_H

#include <map>
#include <memory>
#include "../common.h"
#include "../net_address.h"
#include "../socket.h"
namespace corpc
{
    class TcpAcceptor
    {

    public:
        using ptr = std::shared_ptr<TcpAcceptor>;

        TcpAcceptor(NetAddress::ptr net_addr);

        ~TcpAcceptor();

        int toAccept();

    private:
        Socket::ptr m_listener;
    };
}

#endif