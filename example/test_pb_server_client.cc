#include <iostream>
#include <google/protobuf/service.h>
#include "../corpc/coroutine/co_api.h"
#include "../corpc/net/pb/pb_rpc_channel.h"
// #include "../corpc/net/pb/pb_rpc_async_channel.h"
#include "../corpc/net/pb/pb_rpc_controller.h"
#include "../corpc/net/pb/pb_rpc_closure.h"
#include "../corpc/net/net_address.h"
#include "test_pb_server.pb.h"

void test_client()
{

  corpc::IPAddress::ptr addr = std::make_shared<corpc::IPAddress>("127.0.0.1", 39999);

  corpc::PbRpcChannel channel(addr);
  QueryService_Stub stub(&channel);

  corpc::PbRpcController rpc_controller;
  rpc_controller.SetTimeout(5000);

  queryAgeReq rpc_req;
  queryAgeRes rpc_res;

  std::cout << "Send to rpc server " << addr->toString() << ", requeset body: " << rpc_req.ShortDebugString() << std::endl;
  stub.query_age(&rpc_controller, &rpc_req, &rpc_res, nullptr);

  if (rpc_controller.ErrorCode() != 0)
  {
    std::cout << "Failed to call rpc server, error code: " << rpc_controller.ErrorCode() << ", error info: " << rpc_controller.ErrorText() << std::endl;
    return;
  }

  std::cout << "Success get response frrom rpc server " << addr->toString() << ", response body: " << rpc_res.ShortDebugString() << std::endl;
}

int main(int argc, char *argv[])
{
  corpc::co_go(test_client);
  corpc::sche_join();

  return 0;
}
