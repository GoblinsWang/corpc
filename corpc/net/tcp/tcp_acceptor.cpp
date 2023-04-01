#include "tcp_acceptor.h"

namespace corpc
{
    TcpAcceptor::TcpAcceptor(NetAddress::ptr net_addr)
    {
        m_listener = std::make_shared<NetSocket>(net_addr);
    }

    TcpAcceptor::~TcpAcceptor()
    {
    }

    NetSocket::ptr TcpAcceptor::toAccept()
    {
        int confd = m_listener->accept();
        auto sock = std::make_shared<NetSocket>(confd, m_listener->getPeerAddr());

        return sock;
    }
} // namespace corpc
