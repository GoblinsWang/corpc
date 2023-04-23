#ifndef CORPC_NET_NET_ADDRESS_H
#define CORPC_NET_NET_ADDRESS_H

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <unistd.h>
#include <memory>
#include <string>

namespace corpc
{

    class NetAddress
    {

    public:
        using ptr = std::shared_ptr<NetAddress>;

        virtual sockaddr *getSockAddr() = 0;

        virtual int getFamily() const = 0;

        virtual std::string toString() const = 0;

        virtual socklen_t getSockLen() const = 0;
    };

    class IPAddress : public NetAddress
    {

    public:
        explicit IPAddress(const std::string &ip = "", uint16_t port = -1);

        explicit IPAddress(const std::string &addr);

        explicit IPAddress(uint16_t port);

        explicit IPAddress(sockaddr_in addr);

        sockaddr *getSockAddr();

        int getFamily() const;

        std::string toString() const;

        socklen_t getSockLen() const;

    public:
        inline std::string getIP() const
        {
            return m_ip;
        }

        inline int getPort() const
        {
            return m_port;
        }

        static bool CheckValidIPAddr(const std::string &addr);

    private:
        std::string m_ip;
        uint16_t m_port;
        sockaddr_in m_addr;
    };

    class UnixDomainAddress : public NetAddress
    {

    public:
        UnixDomainAddress(std::string &path);

        UnixDomainAddress(sockaddr_un addr);

        sockaddr *getSockAddr();

        int getFamily() const;

        socklen_t getSockLen() const;

        std::string toString() const;

    public:
        inline std::string getPath() const
        {
            return m_path;
        }

    private:
        std::string m_path = "";
        sockaddr_un m_addr;
    };

}

#endif
