#ifndef CORPC_NET_HTTP_HTTP_RESPONSE_H
#define CORPC_NET_HTTP_HTTP_RESPONSE_H

#include <string>
#include <memory>

#include "../abstract_data.h"
#include "http_define.h"

namespace corpc
{

    class HttpResponse : public AbstractData
    {
    public:
        using ptr = std::shared_ptr<HttpResponse>;

    public:
        std::string m_response_version;
        int m_response_code;
        std::string m_response_info;

        HttpResponseHeader m_response_header;
        std::string m_response_body;
    };

}

#endif
