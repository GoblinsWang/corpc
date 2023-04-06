#include <algorithm>
#include <sstream>
#include "http_codec.h"
#include "../common.h"
#include "../comm/string_util.h"
#include "../abstract_data.h"
#include "../abstract_codec.h"
#include "http_request.h"
#include "http_response.h"

namespace corpc
{

    HttpCodeC::HttpCodeC()
    {
    }

    HttpCodeC::~HttpCodeC()
    {
    }

    void HttpCodeC::encode(TcpBuffer *buf, AbstractData *data)
    {
        LogDebug("------test encode------");
        HttpResponse *response = dynamic_cast<HttpResponse *>(data);
        response->encode_succ = false;

        std::stringstream ss;
        ss << response->m_response_version << " " << response->m_response_code << " "
           << response->m_response_info << "\r\n"
           << response->m_response_header.toHttpString()
           << "\r\n"
           << response->m_response_body;
        std::string http_res = ss.str();
        LogDebug("encode http response is:  " << http_res);

        buf->writeToBuffer(http_res.c_str(), http_res.length());
        LogDebug("succ encode and write to buffer, writeindex=" << buf->writeIndex());
        response->encode_succ = true;
        LogDebug("------test encode end------");
    }

    void HttpCodeC::decode(TcpBuffer *buf, AbstractData *data)
    {
        LogDebug("------test http decode start------");
        std::string strs = "";
        if (!buf || !data)
        {
            LogError("decode error! buf or data nullptr");
            return;
        }
        HttpRequest *request = dynamic_cast<HttpRequest *>(data);
        if (!request)
        {
            LogError("not httprequest type");
            return;
        }

        strs = buf->getBufferString();

        bool is_parse_request_line = false;
        bool is_parse_request_header = false;
        bool is_parse_request_content = false;
        // bool is_parse_succ = false;
        int read_size = 0;
        std::string tmp(strs);
        // LogDebug("pending to parse str:" << tmp << ", total size =" << tmp.size());
        int len = tmp.length();
        while (1)
        {
            if (!is_parse_request_line)
            {
                size_t i = tmp.find(g_CRLF);
                if (i == tmp.npos)
                {
                    LogError("not found CRLF in buffer");
                    return;
                }
                if (i == tmp.length() - 2)
                {
                    LogError("need to read more data");
                    break;
                }
                is_parse_request_line = parseHttpRequestLine(request, tmp.substr(0, i));
                if (!is_parse_request_line)
                {
                    return;
                }
                tmp = tmp.substr(i + 2, len - 2 - i);
                len = tmp.length();
                read_size = read_size + i + 2;
            }

            if (!is_parse_request_header)
            {
                size_t j = tmp.find(g_CRLF_DOUBLE);
                if (j == tmp.npos)
                {
                    LogError("not found CRLF CRLF in buffer");
                    return;
                }

                is_parse_request_header = parseHttpRequestHeader(request, tmp.substr(0, j));
                if (!is_parse_request_header)
                {
                    return;
                }
                tmp = tmp.substr(j + 4, len - 4 - j);
                len = tmp.length();
                read_size = read_size + j + 4;
            }
            if (!is_parse_request_content)
            {
                // 依据头部字段数据来读
                int content_len = std::atoi(request->m_requeset_header.m_maps["Content-Length"].c_str());
                if ((int)strs.length() - read_size < content_len)
                {
                    LogDebug("need to read more data");
                    return;
                }
                if (request->m_request_method == POST && content_len != 0)
                {
                    is_parse_request_content = parseHttpRequestContent(request, tmp.substr(0, content_len));
                    if (!is_parse_request_content)
                    {
                        return;
                    }
                    read_size = read_size + content_len;
                }
                else
                {
                    is_parse_request_content = true;
                }
            }
            if (is_parse_request_line && is_parse_request_header && is_parse_request_header)
            {
                LogDebug("parse http request success, read size is " << read_size << " bytes");
                buf->recycleRead(read_size);
                break;
            }
        }

        request->decode_succ = true;
        data = request;

        LogDebug("------test http decode end------");
    }

    bool HttpCodeC::parseHttpRequestLine(HttpRequest *requset, const std::string &tmp)
    {
        size_t s1 = tmp.find_first_of(" ");
        size_t s2 = tmp.find_last_of(" ");

        if (s1 == tmp.npos || s2 == tmp.npos || s1 == s2)
        {
            LogError("error read Http Requser Line, space is not 2");
            return false;
        }
        std::string method = tmp.substr(0, s1);
        std::transform(method.begin(), method.end(), method.begin(), ::toupper);
        if (method == "GET")
        {
            requset->m_request_method = HttpMethod::GET;
        }
        else if (method == "POST")
        {
            requset->m_request_method = HttpMethod::POST;
        }
        else
        {
            LogError("parse http request request line error, not support http method:" << method);
            return false;
        }

        std::string version = tmp.substr(s2 + 1, tmp.length() - s2 - 1);
        std::transform(version.begin(), version.end(), version.begin(), ::toupper);
        if (version != "HTTP/1.1" && version != "HTTP/1.0")
        {
            LogError("parse http request request line error, not support http version:" << version);
            return false;
        }
        requset->m_request_version = version;

        std::string url = tmp.substr(s1 + 1, s2 - s1 - 1);
        size_t j = url.find("://");

        if (j != url.npos && j + 3 >= url.length())
        {
            LogError("parse http request request line error, bad url:" << url);
            return false;
        }
        int l = 0;
        if (j == url.npos)
        {
            LogDebug("url only have path, url is " << url);
        }
        else
        {
            url = url.substr(j + 3, s2 - s1 - j - 4);
            LogDebug("delete http prefix, url = " << url);
            j = url.find_first_of("/");
            l = url.length();
            if (j == url.npos || j == url.length() - 1)
            {
                LogDebug("http request root path, and query is empty");
                return true;
            }
            url = url.substr(j + 1, l - j - 1);
        }

        l = url.length();
        j = url.find_first_of("?");
        if (j == url.npos)
        {
            requset->m_request_path = url;
            LogDebug("http request path:" << requset->m_request_path << " and query is empty");
            return true;
        }
        requset->m_request_path = url.substr(0, j);
        requset->m_request_query = url.substr(j + 1, l - j - 1);
        LogDebug("http request path:" << requset->m_request_path << ", and query:" << requset->m_request_query);
        StringUtil::SplitStrToMap(requset->m_request_query, "&", "=", requset->m_query_maps);
        return true;
    }

    bool HttpCodeC::parseHttpRequestHeader(HttpRequest *requset, const std::string &str)
    {
        if (str.empty() || str.length() < 4 || str == "\r\n\r\n")
        {
            return true;
        }
        std::string tmp = str;
        StringUtil::SplitStrToMap(tmp, "\r\n", ":", requset->m_requeset_header.m_maps);
        return true;
    }
    bool HttpCodeC::parseHttpRequestContent(HttpRequest *requset, const std::string &str)
    {
        if (str.empty())
        {
            return true;
        }
        requset->m_request_body = str;
        return true;
    }

    ProtocalType HttpCodeC::getProtocalType()
    {
        return Http_Protocal;
    }

}
