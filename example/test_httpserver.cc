#include <google/protobuf/service.h>
#include <atomic>
#include <future>
#include "../corpc/coroutine/co_api.h"
#include "../corpc/coroutine/parameter.h"
#include "../corpc/net/http/http_request.h"
#include "../corpc/net/http/http_response.h"
#include "../corpc/net/http/http_servlet.h"
// #include "../corpc/net/http/http_define.h"
#include "../corpc/net/tcp/tcp_server.h"
#include "../corpc/net/net_address.h"
#include "../corpc/log/logger.h"
using namespace corpc;

const char *html = "<html><body><h1>Welcome to coRPC, just enjoy it!</h1><p>%s</p></body></html>";

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
    auto ser = std::make_shared<corpc::TcpServer>(net_addr);
    ser->start();
    ser->registerHttpServlet("/qps", std::make_shared<QPSHttpServlet>());
    corpc::sche_join();
    return 0;
}
