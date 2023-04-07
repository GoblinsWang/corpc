#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include "tcp_connection.h"
#include "tcp_server.h"

namespace corpc
{

    TcpConnection::TcpConnection(corpc::TcpServer *tcp_svr, NetSocket::ptr net_sock, int buff_size)
        : m_tcp_svr(tcp_svr), m_fd(net_sock->getFd()), m_netsock(net_sock), m_state(Connected), m_connection_type(ServerConnection)
    {

        m_codec = m_tcp_svr->getCodec();
        m_fd_event = FdEventContainer::GetFdContainer()->getFdEvent(m_fd);
        initBuffer(buff_size);

        LogDebug("succ create tcp connection[" << m_state << "], fd=" << m_fd);
    }

    // TcpConnection::TcpConnection(corpc::TcpClient *tcp_cli, corpc::Reactor *reactor, int fd, int buff_size, NetAddress::ptr peer_addr)
    //     : m_fd(fd), m_state(NotConnected), m_connection_type(ClientConnection), m_peer_addr(peer_addr)
    // {
    //     m_reactor = reactor;

    //     m_tcp_cli = tcp_cli;

    //     m_codec = m_tcp_cli->getCodeC();

    //     m_fd_event = FdEventContainer::GetFdContainer()->getFdEvent(fd);
    //     m_fd_event->setReactor(m_reactor);
    //     initBuffer(buff_size);

    //     LogDebug("succ create tcp connection[NotConnected]");
    // }

    void TcpConnection::initServer()
    {
        registerToTimeWheel();
        corpc::co_go(std::bind(&TcpConnection::MainServerLoopCorFunc, this));
    }

    void TcpConnection::registerToTimeWheel()
    {
        auto cb = [](TcpConnection::ptr conn)
        {
            conn->shutdownConnection();
        };
        TcpTimeWheel::TcpConnectionSlot::ptr tmp = std::make_shared<AbstractSlot<TcpConnection>>(shared_from_this(), cb);
        m_weak_slot = tmp;
        m_tcp_svr->freshTcpConnection(tmp);
    }

    void TcpConnection::setUpClient()
    {
        setState(Connected);
    }

    TcpConnection::~TcpConnection()
    {
        // LogError("~TcpConnection, fd = " << m_netsock->getFd());
    }

    void TcpConnection::initBuffer(int size)
    {
        // init two buffer
        m_write_buffer = std::make_shared<TcpBuffer>(size);
        m_read_buffer = std::make_shared<TcpBuffer>(size);
    }

    void TcpConnection::MainServerLoopCorFunc()
    {

        while (!m_stop)
        {
            input();

            execute();

            output();
        }
        // TODO: clear this conn is a task of tcp_server
        // LogError("this connection has already end loop");
    }

    void TcpConnection::input()
    {
        if (m_is_over_time)
        {
            LogInfo("over timer, skip input progress");
            return;
        }
        TcpConnectionState state = getState();
        if (state == Closed || state == NotConnected)
        {
            return;
        }
        bool read_all = false;
        bool close_flag = false;
        int count = 0;
        while (!read_all)
        {

            if (m_read_buffer->writeAble() == 0)
            {
                m_read_buffer->resizeBuffer(2 * m_read_buffer->getSize());
            }

            int read_count = m_read_buffer->writeAble();
            int write_index = m_read_buffer->writeIndex();

            LogDebug("m_read_buffer size = " << m_read_buffer->getBufferVector().size() << " rd = " << m_read_buffer->readIndex() << " wd = " << m_read_buffer->writeIndex());

            int rt = m_netsock->read(&(m_read_buffer->m_buffer[write_index]), read_count);
            if (rt > 0)
            {
                m_read_buffer->recycleWrite(rt);
            }
            LogDebug("m_read_buffer size = " << m_read_buffer->getBufferVector().size() << ", rd = " << m_read_buffer->readIndex() << ", wd = " << m_read_buffer->writeIndex());

            LogDebug("read data back, fd = " << m_netsock->getFd());
            count += rt;
            if (m_is_over_time)
            {
                LogInfo("over timer, now break read function");
                break;
            }
            if (rt <= 0)
            {
                LogDebug("rt <= 0");
                LogDebug("read empty while occur read event, because of peer close, fd= " << m_netsock->getFd() << ", sys error=" << strerror(errno) << ", now to clear tcp connection");
                // this cor can destroy
                close_flag = true;
                break;
            }
            else
            {
                if (rt == read_count)
                {
                    LogDebug("read_count == rt");
                    // is is possible read more data, should continue read
                    continue;
                }
                else if (rt < read_count)
                {
                    LogDebug("read_count > rt");
                    // read all data in socket buffer, skip out loop
                    read_all = true;
                    break;
                }
            }
        }

        if (close_flag)
        {
            // set close status
            clearClient();
            return;
        }

        if (m_is_over_time)
        {
            return;
        }

        if (!read_all)
        {
            LogError("not read all data in socket buffer");
        }

        LogInfo("recv [" << count << "] bytes data from [" << m_netsock->getLocalAddr()->toString() << "], fd [" << m_netsock->getFd() << "]");

        if (m_connection_type == ServerConnection)
        {
            int64_t now = getNowMs();
            // 超过时间轮间隔，才fresh
            if (now - getLastActiveTime() >= m_tcp_svr->getTimeWheel()->getInterval())
            {
                TcpTimeWheel::TcpConnectionSlot::ptr tmp = m_weak_slot.lock();
                if (tmp)
                {
                    m_tcp_svr->freshTcpConnection(tmp);
                }
            }
            updateLastActiveTime();
        }
    }

    void TcpConnection::execute()
    {
        LogDebug("begin to do execute");

        // it only server do this
        while (m_read_buffer->readAble() > 0)
        {
            std::shared_ptr<AbstractData> data;
            if (m_codec->getProtocalType() == TinyPb_Protocal)
            {
                // data = std::make_shared<TinyPbStruct>();
            }
            else
            {
                data = std::make_shared<HttpRequest>();
            }

            m_codec->decode(m_read_buffer.get(), data.get());

            if (!data->decode_succ)
            {
                // TODO: how to solve error http?
                LogError("it parse request error of fd " << m_netsock->getFd());
                break;
            }
            LogDebug("it parse request success");
            if (m_connection_type == ServerConnection)
            {
                LogDebug("to dispatch this package");
                m_tcp_svr->getDispatcher()->dispatch(data.get(), this);
                LogDebug("contine parse next package");
            }
            // else if (m_connection_type == ClientConnection)
            // {
            //     // TODO:
            //     std::shared_ptr<TinyPbStruct> tmp = std::dynamic_pointer_cast<TinyPbStruct>(data);
            //     if (tmp)
            //     {
            //         m_reply_datas.insert(std::make_pair(tmp->msg_req, tmp));
            //     }
            // }
        }
    }

    void TcpConnection::output()
    {
        if (m_is_over_time)
        {
            LogInfo("over timer, skip output progress");
            return;
        }
        while (true)
        {
            TcpConnectionState state = getState();
            if (state != Connected)
            {
                break;
            }

            if (m_write_buffer->readAble() == 0)
            {
                LogDebug("app buffer of fd[" << m_netsock->getFd() << "] no data to write, to yiled this coroutine");
                break;
            }

            int total_size = m_write_buffer->readAble();
            int read_index = m_write_buffer->readIndex();

            int rt = m_netsock->send(&(m_write_buffer->m_buffer[read_index]), total_size);

            LogDebug("succ write " << rt << " bytes");
            m_write_buffer->recycleRead(rt);
            LogDebug("recycle write index =" << m_write_buffer->writeIndex() << ", read_index =" << m_write_buffer->readIndex() << "readable = " << m_write_buffer->readAble());
            LogInfo("send[" << rt << "] bytes data to [" << m_netsock->getLocalAddr()->toString() << "], fd [" << m_netsock->getFd() << "]");

            if (m_is_over_time)
            {
                LogInfo("over timer, now break write function");
                break;
            }
        }
    }

    void TcpConnection::clearClient()
    {
        if (getState() == Closed)
        {
            LogDebug("this client has closed");
            return;
        }
        // stop read and write cor
        m_stop = true;

        ::close(m_fd_event->getFd());
        setState(Closed);
    }

    void TcpConnection::shutdownConnection()
    {
        TcpConnectionState state = getState();
        if (state == Closed || state == NotConnected)
        {
            LogDebug("this client has closed");
            return;
        }
        setState(HalfClosing);
        LogError("shutdown conn[" << m_netsock->getLocalAddr()->toString() << "], fd=" << m_netsock->getFd());
        // call sys shutdown to send FIN
        // wait client done something, client will send FIN
        // and fd occur read event but byte count is 0
        // then will call clearClient to set CLOSED
        // IOThread::MainLoopTimerFunc will delete CLOSED connection
        shutdown(m_fd_event->getFd(), SHUT_RDWR);
    }

    TcpBuffer *TcpConnection::getInBuffer()
    {
        return m_read_buffer.get();
    }

    TcpBuffer *TcpConnection::getOutBuffer()
    {
        return m_write_buffer.get();
    }

    // bool TcpConnection::getResPackageData(const std::string &msg_req, TinyPbStruct::pb_ptr &pb_struct)
    // {
    //     auto it = m_reply_datas.find(msg_req);
    //     if (it != m_reply_datas.end())
    //     {
    //         LogDebug("return a resdata";
    //         pb_struct = it->second;
    //         m_reply_datas.erase(it);
    //         return true;
    //     }
    //     LogDebug(msg_req << "|reply data not exist";
    //     return false;
    // }

    AbstractCodeC::ptr TcpConnection::getCodec() const
    {
        return m_codec;
    }

    TcpConnectionState TcpConnection::getState()
    {
        TcpConnectionState state;
        m_rwmutex.rlock(); // 读锁
        state = m_state;
        m_rwmutex.runlock();

        return state;
    }

    void TcpConnection::setState(const TcpConnectionState &state)
    {
        m_rwmutex.wlock();
        m_state = state;
        m_rwmutex.wunlock();
    }

    // void TcpConnection::setOverTimeFlag(bool value)
    // {
    //     m_is_over_time = value;
    // }

    // bool TcpConnection::getOverTimerFlag()
    // {
    //     return m_is_over_time;
    // }

    // Coroutine::ptr TcpConnection::getCoroutine()
    // {
    //     return m_loop_cor;
    // }

}
