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

    int TcpAcceptor::toAccept()
    {
        return m_listener->accept();
    }
} // namespace corpc
