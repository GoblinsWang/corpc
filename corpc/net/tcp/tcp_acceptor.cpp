#include "tcp_acceptor.h"

namespace corpc
{
    TcpAcceptor::TcpAcceptor(NetAddress::ptr net_addr)
    {
        m_listener = std::make_shared<Socket>(net_addr);
    }

    TcpAcceptor::~TcpAcceptor()
    {
    }

    Socket::ptr TcpAcceptor::toAccept()
    {
        int confd = m_listener->accept();
        auto sock = std::make_shared<Socket>(confd, m_listener->getPeerAddr());

        return sock;
    }
} // namespace corpc
