#ifndef CORPC_NET_ABSTRACT_DATA_H
#define CORPC_NET_ABSTRACT_DATA_H

namespace corpc
{

    class AbstractData
    {
    public:
        AbstractData() = default;
        virtual ~AbstractData(){};

        bool decode_succ{false};
        bool encode_succ{false};
    };

}

#endif