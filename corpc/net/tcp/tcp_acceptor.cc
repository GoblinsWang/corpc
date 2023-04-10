#include "tcp_acceptor.h"

namespace corpc
{
    TcpAcceptor::TcpAcceptor(NetAddress::ptr net_addr)
    {
        m_listener = std::make_shared<NetSocket>(net_addr);
    }

    TcpAcceptor::~TcpAcceptor()
    {
        if (m_listener->getFd() > 0)
        {
            corpc::Scheduler::getScheduler()->getProcessor(0)->getEpoller()->delEvent(m_listener->getFd()); // 0
            ::close(m_listener->getFd());
        }
    }

    NetSocket::ptr TcpAcceptor::toAccept()
    {
        int confd = m_listener->accept();
        auto sock = std::make_shared<NetSocket>(confd, m_listener->getPeerAddr());

        return sock;
    }
} // namespace corpc
