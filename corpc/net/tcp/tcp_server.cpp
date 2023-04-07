#include <sys/socket.h>
#include <assert.h>
#include <fcntl.h>
#include <string.h>
#include "tcp_server.h"
#include "tcp_connection.h"
#include "../net_socket.h"
#include "../common.h"
// #include "../tcp_connection_time_wheel.h"
#include "../http/http_codec.h"
// #include "tinyrpc/net/http/http_dispatcher.h"
// #include "tinyrpc/net/tinypb/tinypb_rpc_dispatcher.h"

namespace corpc
{

    TcpServer::TcpServer(NetAddress::ptr addr, ProtocalType type /*= TinyPb_Protocal*/) : m_addr(addr)
    {
        if (type == Http_Protocal)
        {
            m_dispatcher = std::make_shared<HttpDispacther>();
            m_codec = std::make_shared<HttpCodeC>();
            m_protocal_type = Http_Protocal;
        }
        else
        {
            // m_dispatcher = std::make_shared<TinyPbRpcDispacther>();
            // m_codec = std::make_shared<TinyPbCodeC>();
            // m_protocal_type = TinyPb_Protocal;
        }

        m_time_wheel = std::make_shared<TcpTimeWheel>(10, 6); // 时间轮->10个slot，6秒间隔

        m_clear_clent_timer_event = std::make_shared<TimerEvent>(10000, true, std::bind(&TcpServer::ClearClientTimerFunc, this));
        corpc::Scheduler::getScheduler()->getProcessor(0)->GetTimer()->addTimerEvent(m_clear_clent_timer_event); // 0

        LogInfo("TcpServer setup on [" << m_addr->toString() << "]");
    }

    void TcpServer::start()
    {
        // init a acceptor
        m_acceptor.reset(new TcpAcceptor(m_addr));
        corpc::co_go(std::bind(&TcpServer::MainAcceptCorFunc, this), 0); // 0号协程处理实例
    }

    TcpServer::~TcpServer()
    {
        LogDebug("~TcpServer");
    }

    void TcpServer::MainAcceptCorFunc()
    {

        while (!m_is_stop_accept)
        {

            auto sock = m_acceptor->toAccept();
            sock->setTcpNoDelay(true);

            TcpConnection::ptr conn = addClient(sock);
            conn->initServer();
            LogDebug("tcpconnection init succ, and fd is" << sock->getFd());

            m_tcp_counts++;
            LogDebug("current tcp connection count is [" << m_tcp_counts << "]");
        }
    }

    TcpConnection::ptr TcpServer::addClient(NetSocket::ptr net_sock)
    {

        auto it = m_clients.find(net_sock->getFd());
        if (it != m_clients.end())
        {
            it->second.reset();
            // set new Tcpconnection
            LogInfo("fd " << net_sock->getFd() << "have exist, reset it");
            it->second = std::make_shared<TcpConnection>(this, net_sock, 128);
            return it->second;
        }
        else
        {
            LogInfo("fd " << net_sock->getFd() << ", did't exist, new it");
            TcpConnection::ptr conn = std::make_shared<TcpConnection>(this, net_sock, 128);
            m_clients.insert(std::make_pair(net_sock->getFd(), conn));
            return conn;
        }
    }

    // bool TcpServer::registerService(std::shared_ptr<google::protobuf::Service> service)
    // {
    //     if (m_protocal_type == TinyPb_Protocal)
    //     {
    //         if (service)
    //         {
    //             dynamic_cast<TinyPbRpcDispacther *>(m_dispatcher.get())->registerService(service);
    //         }
    //         else
    //         {
    //             ErrorLog << "register service error, service ptr is nullptr";
    //             return false;
    //         }
    //     }
    //     else
    //     {
    //         ErrorLog << "register service error. Just TinyPB protocal server need to resgister Service";
    //         return false;
    //     }
    //     return true;
    // }

    bool TcpServer::registerHttpServlet(const std::string &url_path, HttpServlet::ptr servlet)
    {
        if (m_protocal_type == Http_Protocal)
        {
            if (servlet)
            {
                dynamic_cast<HttpDispacther *>(m_dispatcher.get())->registerServlet(url_path, servlet);
            }
            else
            {
                LogError("register http servlet error, servlet ptr is nullptr");
                return false;
            }
        }
        else
        {
            LogError("register http servlet error. Just Http protocal server need to resgister HttpServlet");
            return false;
        }
        return true;
    }

    void TcpServer::freshTcpConnection(TcpTimeWheel::TcpConnectionSlot::ptr slot)
    {
        auto cofunc = [slot, this]() mutable
        {
            this->getTimeWheel()->fresh(slot);
            slot.reset();
        };
        // 0, avoid crush between some thread
        corpc::co_go(cofunc, 0);
    }

    void TcpServer::ClearClientTimerFunc()
    {
        // delete Closed TcpConnection per loop
        // for free memory
        // LogDebug("m_clients.size=" << m_clients.size());
        for (auto &i : m_clients)
        {
            if (i.second && i.second.use_count() > 0 && i.second->getState() == Closed)
            {
                // need to delete TcpConnection
                // LogError("TcpConection [fd:" << i.first << "] will delete, state = " << i.second->getState());
                (i.second).reset();
            }
        }
    }

    AbstractDispatcher::ptr TcpServer::getDispatcher()
    {
        return m_dispatcher;
    }

    AbstractCodeC::ptr TcpServer::getCodec()
    {
        return m_codec;
    }

    TcpTimeWheel::ptr TcpServer::getTimeWheel()
    {
        return m_time_wheel;
    }

}
