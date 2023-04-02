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

        // m_main_reactor = tinyrpc::Reactor::GetReactor();
        // m_main_reactor->setReactorType(MainReactor);

        // m_time_wheel = std::make_shared<TcpTimeWheel>(m_main_reactor, gRpcConfig->m_timewheel_bucket_num, gRpcConfig->m_timewheel_inteval);

        // m_clear_clent_timer_event = std::make_shared<TimerEvent>(10000, true, std::bind(&TcpServer::ClearClientTimerFunc, this));
        // m_main_reactor->getTimer()->addTimerEvent(m_clear_clent_timer_event);

        LogInfo("TcpServer setup on [" << m_addr->toString() << "]");
    }

    void TcpServer::start()
    {
        // init a acceptor
        m_acceptor.reset(new TcpAcceptor(m_addr));
        corpc::co_go(std::bind(&TcpServer::MainAcceptCorFunc, this));
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
            LogDebug("fd " << net_sock->getFd() << "have exist, reset it");
            it->second = std::make_shared<TcpConnection>(this, net_sock, 128);
            return it->second;
        }
        else
        {
            LogDebug("fd " << net_sock->getFd() << ", did't exist, new it");
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

    // void TcpServer::freshTcpConnection(TcpTimeWheel::TcpConnectionSlot::ptr slot)
    // {
    //     auto cb = [slot, this]() mutable
    //     {
    //         this->getTimeWheel()->fresh(slot);
    //         slot.reset();
    //     };
    //     m_main_reactor->addTask(cb);
    // }

    // void TcpServer::ClearClientTimerFunc()
    // {
    //     // DebugLog << "this IOThread loop timer excute";

    //     // delete Closed TcpConnection per loop
    //     // for free memory
    //     // DebugLog << "m_clients.size=" << m_clients.size();
    //     for (auto &i : m_clients)
    //     {
    //         // TcpConnection::ptr s_conn = i.second;
    //         // DebugLog << "state = " << s_conn->getState();
    //         if (i.second && i.second.use_count() > 0 && i.second->getState() == Closed)
    //         {
    //             // need to delete TcpConnection
    //             DebugLog << "TcpConection [fd:" << i.first << "] will delete, state=" << i.second->getState();
    //             (i.second).reset();
    //             // s_conn.reset();
    //         }
    //     }
    // }

    // TcpTimeWheel::ptr TcpServer::getTimeWheel()
    // {
    //     return m_time_wheel;
    // }

    AbstractDispatcher::ptr TcpServer::getDispatcher()
    {
        return m_dispatcher;
    }

    AbstractCodeC::ptr TcpServer::getCodec()
    {
        return m_codec;
    }

}
