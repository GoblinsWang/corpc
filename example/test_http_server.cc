#include "../corpc/coroutine/co_api.h"
#include "../corpc/coroutine/parameter.h"
#include "../corpc/net/http/http_request.h"
#include "../corpc/net/http/http_response.h"
#include "../corpc/net/http/http_servlet.h"
#include "../corpc/net/tcp/tcp_server.h"
#include "../corpc/net/net_address.h"
#include "../corpc/log/logger.h"
#include "../corpc/net/pb/pb_rpc_channel.h"
#include "../corpc/net/pb/pb_rpc_controller.h"
#include "test_pb_server.pb.h"
using namespace corpc;

const char *html = "<html><body><h1>Welcome to corpc, just enjoy it!</h1><p>%s</p></body></html>";

corpc::IPAddress::ptr addr = std::make_shared<corpc::IPAddress>("127.0.0.1", 39999);

class BlockCallHttpServlet : public corpc::HttpServlet
{
public:
    BlockCallHttpServlet() = default;
    ~BlockCallHttpServlet() = default;

    void handle(corpc::HttpRequest *req, corpc::HttpResponse *res)
    {
        LogDebug("BlockCallHttpServlet get request ");
        LogDebug("BlockCallHttpServlet success recive http request, now to get http response");
        setHttpCode(res, corpc::HTTP_OK);
        setHttpContentType(res, "text/html;charset=utf-8");

        queryAgeReq rpc_req;
        queryAgeRes rpc_res;
        LogDebug("now to call QueryServer coRPC server to query who's id is " << req->m_query_maps["id"]);
        rpc_req.set_id(std::atoi(req->m_query_maps["id"].c_str()));

        corpc::PbRpcChannel channel(addr);
        QueryService_Stub stub(&channel);

        corpc::PbRpcController rpc_controller;
        rpc_controller.SetTimeout(5000);

        LogDebug("BlockCallHttpServlet end to call RPC");
        stub.query_age(&rpc_controller, &rpc_req, &rpc_res, NULL);
        LogDebug("BlockCallHttpServlet end to call RPC");

        if (rpc_controller.ErrorCode() != 0)
        {
            LogDebug("failed to call QueryServer rpc server");
            char buf[512];
            sprintf(buf, html, "failed to call QueryServer rpc server");
            setHttpBody(res, std::string(buf));
            return;
        }

        if (rpc_res.ret_code() != 0)
        {
            std::stringstream ss;
            ss << "QueryServer rpc server return bad result, ret = " << rpc_res.ret_code() << ", and res_info = " << rpc_res.res_info();
            LogDebug(ss.str());
            char buf[512];
            sprintf(buf, html, ss.str().c_str());
            setHttpBody(res, std::string(buf));
            return;
        }

        std::stringstream ss;
        ss << "Success!! Your age is," << rpc_res.age() << " and Your id is " << rpc_res.id();

        char buf[512];
        sprintf(buf, html, ss.str().c_str());
        setHttpBody(res, std::string(buf));
    }

    std::string getServletName()
    {
        return "BlockCallHttpServlet";
    }
};

class QPSHttpServlet : public corpc::HttpServlet
{
public:
    QPSHttpServlet() = default;
    ~QPSHttpServlet() = default;

    void handle(corpc::HttpRequest *req, corpc::HttpResponse *res)
    {
        LogInfo("QPSHttpServlet get request");
        setHttpCode(res, corpc::HTTP_OK);
        setHttpContentType(res, "text/html;charset=utf-8");

        std::stringstream ss;
        ss << "QPSHttpServlet Echo Success!! Your id is," << req->m_query_maps["id"];
        char buf[512];
        sprintf(buf, html, ss.str().c_str());
        setHttpBody(res, std::string(buf));
        LogInfo(ss.str());
    }

    std::string getServletName()
    {
        return "QPSHttpServlet";
    }
};

int main(int argc, char *argv[])
{
    auto net_addr = std::make_shared<corpc::IPAddress>(8888);
    auto ser = std::make_shared<corpc::TcpServer>(net_addr, Http_Protocal);
    ser->start();
    ser->registerHttpServlet("/qps", std::make_shared<QPSHttpServlet>());
    ser->registerHttpServlet("/block", std::make_shared<BlockCallHttpServlet>());
    corpc::sche_join();
    return 0;
}
