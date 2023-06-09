#ifndef CORPC_NET_TCP_TCP_CONNECTION_H
#define CORPC_NET_TCP_TCP_CONNECTION_H

#include <memory>
#include <vector>
#include <queue>

#include "tcp_buffer.h"
#include "abstract_slot.h"
#include "../common.h"
#include "../net_address.h"
#include "../abstract_codec.h"
#include "../net_socket.h"
#include "../http/http_request.h"
#include "../pb/pb_codec.h"

namespace corpc
{

    class TcpServer;
    class TcpClient;

    enum TcpConnectionState
    {
        NotConnected = 1, // can do io
        Connected = 2,    // can do io
        HalfClosing = 3,  // server call shutdown, write half close. can read,but can't write
        Closed = 4,       // can't do io
    };

    class TcpConnection : public std::enable_shared_from_this<TcpConnection>
    {

    public:
        using ptr = std::shared_ptr<TcpConnection>;

        using weakptr = std::weak_ptr<TcpConnection>;

        TcpConnection(corpc::TcpServer *tcp_svr, NetSocket::ptr net_sock, int buff_size);

        TcpConnection(corpc::TcpClient *tcp_cli, int fd, int buff_size, NetAddress::ptr peer_addr);

        ~TcpConnection();

        void setUpClient();

        void initBuffer(int size);

        enum ConnectionType
        {
            ServerConnection = 1, // owned by tcp_server
            ClientConnection = 2, // owned by tcp_client
        };

    public:
        void shutdownConnection();

        TcpConnectionState getState();

        void setState(const TcpConnectionState &state);

        TcpBuffer *getInBuffer();

        TcpBuffer *getOutBuffer();

        AbstractCodeC::ptr getCodec() const;

        bool getResPackageData(const std::string &msg_req, PbStruct::pb_ptr &pb_struct);

        void registerToTimeWheel();

        Coroutine::ptr getCoroutine();

    public:
        void MainServerLoopCorFunc();

        int toConnect();

        void input();

        void execute();

        void output();

        void setOverTimeFlag(bool value);

        bool getOverTimerFlag();

        void initServer();

        inline int64_t getLastActiveTime() const
        {
            return m_last_active_time;
        };

        inline void updateLastActiveTime()
        {
            m_last_active_time = getNowMs();
        }

    private:
        void clearClient();

    private:
        TcpServer *m_tcp_svr{nullptr};
        TcpClient *m_tcp_cli{nullptr};

        // Processor *m_processor{nullptr};

        int m_fd;

        NetSocket::ptr m_netsock;
        TcpConnectionState m_state{TcpConnectionState::Connected};
        ConnectionType m_connection_type{ServerConnection};

        // NetAddress::ptr m_peer_addr;

        TcpBuffer::ptr m_read_buffer;
        TcpBuffer::ptr m_write_buffer;

        Coroutine::ptr m_loop_cor;

        AbstractCodeC::ptr m_codec;

        FdEvent::ptr m_fd_event;

        bool m_stop{false};

        bool m_is_over_time{false};

        int64_t m_last_active_time;

        std::map<std::string, std::shared_ptr<PbStruct>> m_reply_datas;

        std::weak_ptr<AbstractSlot<TcpConnection>> m_weak_slot;

        RWMutex m_mutex;
    };

}

#endif
