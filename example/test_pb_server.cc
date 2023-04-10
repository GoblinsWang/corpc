#include <google/protobuf/service.h>
#include <sstream>
#include <mutex>
#include <atomic>

#include "../corpc/net/tcp/tcp_server.h"
#include "../corpc/net/net_address.h"
#include "../corpc/coroutine/rw_mutex.h"
#include "../corpc/net/pb/pb_rpc_dispatcher.h"
#include "../corpc/log/logger.h"
#include "test_pb_server.pb.h"
using namespace corpc;

static int i = 0;
corpc::RWMutex g_cor_mutex;

class QueryServiceImpl : public QueryService
{
public:
	QueryServiceImpl() {}
	~QueryServiceImpl() {}

	void query_name(google::protobuf::RpcController *controller,
					const ::queryNameReq *request,
					::queryNameRes *response,
					::google::protobuf::Closure *done)
	{

		LogDebug("QueryServiceImpl.query_name, req={" << request->ShortDebugString() << "}");

		response->set_id(request->id());
		response->set_name("goblin");

		LogDebug("QueryServiceImpl.query_name, req={" << request->ShortDebugString() << "}, res={" << response->ShortDebugString() << "}");

		if (done)
		{
			done->Run();
		}
	}

	void query_age(google::protobuf::RpcController *controller,
				   const ::queryAgeReq *request,
				   ::queryAgeRes *response,
				   ::google::protobuf::Closure *done)
	{

		LogDebug("QueryServiceImpl.query_age, req={" << request->ShortDebugString() << "}");

		response->set_ret_code(0);
		response->set_res_info("OK");
		response->set_req_no(request->req_no());
		response->set_id(request->id());
		response->set_age(100100111);

		g_cor_mutex.wlock();
		LogDebug("begin i = " << i);
		// co_sleep(1000);
		i++;
		LogDebug("end i = " << i);
		g_cor_mutex.wunlock();

		if (done)
		{
			done->Run();
		}
		// printf("response = %s\n", response->ShortDebugString().c_str());

		LogDebug("QueryServiceImpl.query_age, res={" << response->ShortDebugString() << "}");
	}
};

int main(int argc, char *argv[])
{
	auto net_addr = std::make_shared<corpc::IPAddress>(39999);
	auto ser = std::make_shared<corpc::TcpServer>(net_addr, Pb_Protocal);
	ser->start();
	ser->registerService(std::make_shared<QueryServiceImpl>());
	corpc::sche_join();
	return 0;
}
