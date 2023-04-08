#ifndef CORPC_NET_TCP_TCP_CLIENT_H
#define CORPC_NET_TCP_TCP_CLIENT_H

#include <memory>
#include <google/protobuf/service.h>
#include "tcp_connection.h"
#include "../abstract_codec.h"
#include "../net_address.h"
#include "../common.h"
#include "../pb/pb_data.h"

namespace corpc
{

    //
    // You should use TcpClient in a coroutine(not main coroutine)
    //
    class TcpClient
    {
    public:
        typedef std::shared_ptr<TcpClient> ptr;

        TcpClient(NetAddress::ptr addr, ProtocalType type = Pb_Protocal);

        ~TcpClient();

        void init();

        void resetFd();

        int sendAndRecvPb(const std::string &msg_no, PbStruct::pb_ptr &res);

        void stop();

        TcpConnection *getConnection();

        void setTimeout(const int v)
        {
            m_max_timeout = v;
        }

        void setTryCounts(const int v)
        {
            m_try_counts = v;
        }

        const std::string &getErrInfo()
        {
            return m_err_info;
        }

        NetAddress::ptr getPeerAddr() const
        {
            return m_peer_addr;
        }

        NetAddress::ptr getLocalAddr() const
        {
            return m_local_addr;
        }

        AbstractCodeC::ptr getCodeC()
        {
            return m_codec;
        }

        Processor *getProcessor()
        {
            return m_processor;
        }

    private:
        int m_family{0};
        int m_fd{-1};
        int m_try_counts{3};      // max try reconnect times
        int m_max_timeout{10000}; // max connect timeout, ms
        bool m_is_stop{false};
        std::string m_err_info; // error info of client

        NetAddress::ptr m_local_addr{nullptr};
        NetAddress::ptr m_peer_addr{nullptr};
        Processor *m_processor{nullptr};
        TcpConnection::ptr m_connection{nullptr};

        AbstractCodeC::ptr m_codec{nullptr};

        bool m_connect_succ{false};
    };

}

#endif