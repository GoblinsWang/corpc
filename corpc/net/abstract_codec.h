#ifndef CORPC_NET_ABSTRACT_CODEC_H
#define CORPC_NET_ABSTRACT_CODEC_H

#include <string>
#include <memory>
#include "tcp/tcp_buffer.h"
#include "abstract_data.h"

namespace corpc
{

    enum ProtocalType
    {
        Pb_Protocal = 1,
        Http_Protocal = 2
    };

    class AbstractCodeC
    {

    public:
        typedef std::shared_ptr<AbstractCodeC> ptr;

        AbstractCodeC() {}

        virtual ~AbstractCodeC() {}

        virtual void encode(TcpBuffer *buf, AbstractData *data) = 0;

        virtual void decode(TcpBuffer *buf, AbstractData *data) = 0;

        virtual ProtocalType getProtocalType() = 0;
    };

}

#endif
