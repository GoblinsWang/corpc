#include <sys/socket.h>
#include <arpa/inet.h>
#include "tcp_client.h"
#include "../comm/error_code.h"
#include "../http/http_codec.h"
#include "../pb/pb_codec.h"

namespace corpc
{

    TcpClient::TcpClient(NetAddress::ptr addr, ProtocalType type /*= Pb_Protocal*/) : m_peer_addr(addr)
    {

        m_family = m_peer_addr->getFamily();
        m_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (m_fd == -1)
        {
            LogError("call socket error, fd=-1, sys error=" << strerror(errno));
        }
        LogDebug("TcpClient() create fd = " << m_fd);
        m_local_addr = std::make_shared<corpc::IPAddress>("127.0.0.1", 0);
        m_processor = corpc::Scheduler::getScheduler()->getProcessor(threadIdx); // set processor

        if (type == Http_Protocal)
        {
            m_codec = std::make_shared<HttpCodeC>();
        }
        else
        {
            m_codec = std::make_shared<PbCodeC>();
        }

        m_connection = std::make_shared<TcpConnection>(this, m_fd, 128, m_peer_addr);
    }

    TcpClient::~TcpClient()
    {
        if (m_fd > 0)
        {
            ::close(m_fd);
            LogDebug("~TcpClient() close fd = " << m_fd);
        }
    }

    TcpConnection *TcpClient::getConnection()
    {
        if (!m_connection.get())
        {
            m_connection = std::make_shared<TcpConnection>(this, m_fd, 128, m_peer_addr);
        }
        return m_connection.get();
    }

    void TcpClient::resetFd()
    {
        m_processor->GetEpoller()->delEvent(m_fd); // 注销
        ::close(m_fd);
        m_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (m_fd == -1)
        {
            LogError("call socket error, fd=-1, sys error=" << strerror(errno));
        }
    }

    int TcpClient::sendAndRecvPb(const std::string &msg_no, PbStruct::pb_ptr &res)
    {
        bool is_timeout = false;

        Coroutine *cur_cor = m_processor->getCurRunningCo();
        Processor *pro = m_processor;
        auto timer_cb = [this, &is_timeout, cur_cor, pro]()
        {
            LogInfo("TcpClient timer out event occur");
            is_timeout = true;
            this->m_connection->setOverTimeFlag(true);
            pro->resume(cur_cor);
        };
        TimerEvent::ptr event = std::make_shared<TimerEvent>(m_max_timeout, false, timer_cb);
        m_processor->getTimer()->addTimerEvent(event);

        LogDebug("add corpc timer event, timeout on " << event->m_arrive_time);

        while (!is_timeout)
        {
            LogDebug("begin to connect");
            if (m_connection->getState() != Connected)
            {
                int rt = m_connection->toConnect();
                if (rt == 0)
                {
                    LogDebug("connect [" << m_peer_addr->toString() << "] succ!");
                    m_connection->setUpClient();
                    break;
                }
                resetFd();
                if (is_timeout)
                {
                    LogInfo("connect timeout, break");
                    goto err_deal;
                }
                if (errno == ECONNREFUSED)
                {
                    std::stringstream ss;
                    ss << "connect error, peer[ " << m_peer_addr->toString() << " ] closed.";
                    m_err_info = ss.str();
                    LogError("cancle overtime event, err info=" << m_err_info);
                    m_processor->getTimer()->delTimerEvent(event);
                    return ERROR_PEER_CLOSED;
                }
                if (errno == EAFNOSUPPORT)
                {
                    std::stringstream ss;
                    ss << "connect cur sys ror, errinfo is " << std::string(strerror(errno)) << " ] closed.";
                    m_err_info = ss.str();
                    LogError("cancle overtime event, err info=" << m_err_info);
                    m_processor->getTimer()->delTimerEvent(event);
                    return ERROR_CONNECT_SYS_ERR;
                }
            }
            else
            {
                break;
            }
        }

        if (m_connection->getState() != Connected)
        {
            std::stringstream ss;
            ss << "connect peer addr[" << m_peer_addr->toString() << "] error. sys error=" << strerror(errno);
            m_err_info = ss.str();
            m_processor->getTimer()->delTimerEvent(event);
            return ERROR_FAILED_CONNECT;
        }

        m_connection->setUpClient();
        m_connection->output();
        if (m_connection->getOverTimerFlag())
        {
            LogInfo("send data over time");
            is_timeout = true;
            goto err_deal;
        }

        while (!m_connection->getResPackageData(msg_no, res))
        {
            LogDebug("redo getResPackageData");
            m_connection->input();

            if (m_connection->getOverTimerFlag())
            {
                LogInfo("read data over time");
                is_timeout = true;
                goto err_deal;
            }
            if (m_connection->getState() == Closed)
            {
                LogInfo("peer close");
                goto err_deal;
            }

            m_connection->execute();
        }

        m_processor->getTimer()->delTimerEvent(event);
        m_err_info = "";
        return 0;

    err_deal:
        // connect error should close fd and reopen new one
        m_processor->GetEpoller()->delEvent(m_fd); // 注销
        ::close(m_fd);

        m_fd = socket(AF_INET, SOCK_STREAM, 0);
        std::stringstream ss;
        if (is_timeout)
        {
            ss << "call corpc falied, over " << m_max_timeout << " ms";
            m_err_info = ss.str();

            m_connection->setOverTimeFlag(false);
            return ERROR_RPC_CALL_TIMEOUT;
        }
        else
        {
            ss << "call corpc falied, peer closed [" << m_peer_addr->toString() << "]";
            m_err_info = ss.str();
            return ERROR_PEER_CLOSED;
        }
    }

    void TcpClient::stop()
    {
        if (!m_is_stop)
        {
            m_is_stop = true;
            m_processor->stop(); //
        }
    }

} // namespace corpc
