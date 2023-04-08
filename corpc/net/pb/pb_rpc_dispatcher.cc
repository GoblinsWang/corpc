#include <google/protobuf/message.h>
#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>

#include "pb_data.h"
#include "pb_rpc_dispatcher.h"
#include "pb_rpc_controller.h"
#include "pb_rpc_closure.h"
#include "pb_codec.h"
#include "../comm/msg_req.h"
#include "../comm/error_code.h"

namespace corpc
{

  class TcpBuffer;

  void PbRpcDispacther::dispatch(AbstractData *data, TcpConnection *conn)
  {
    PbStruct *tmp = dynamic_cast<PbStruct *>(data);

    if (tmp == nullptr)
    {
      LogError("dynamic_cast error");
      return;
    }
    // Coroutine::GetCurrentCoroutine()->getRunTime()->m_msg_no = tmp->msg_req;
    // setCurrentRunTime(Coroutine::GetCurrentCoroutine()->getRunTime());

    LogInfo("begin to dispatch client pb request, msgno=" << tmp->msg_req);

    std::string service_name;
    std::string method_name;

    PbStruct reply_pk;
    reply_pk.service_full_name = tmp->service_full_name;
    reply_pk.msg_req = tmp->msg_req;
    if (reply_pk.msg_req.empty())
    {
      reply_pk.msg_req = MsgReqUtil::genMsgNumber();
    }

    if (!parseServiceFullName(tmp->service_full_name, service_name, method_name))
    {
      LogError(reply_pk.msg_req << "|parse service name " << tmp->service_full_name << "error");

      reply_pk.err_code = ERROR_PARSE_SERVICE_NAME;
      std::stringstream ss;
      ss << "cannot parse service_name:[" << tmp->service_full_name << "]";
      reply_pk.err_info = ss.str();
      conn->getCodec()->encode(conn->getOutBuffer(), dynamic_cast<AbstractData *>(&reply_pk));
      return;
    }

    // Coroutine::GetCurrentCoroutine()->getRunTime()->m_interface_name = tmp->service_full_name;
    auto it = m_service_map.find(service_name);
    if (it == m_service_map.end() || !((*it).second))
    {
      reply_pk.err_code = ERROR_SERVICE_NOT_FOUND;
      std::stringstream ss;
      ss << "not found service_name:[" << service_name << "]";
      LogError(reply_pk.msg_req << "|" << ss.str());
      reply_pk.err_info = ss.str();

      conn->getCodec()->encode(conn->getOutBuffer(), dynamic_cast<AbstractData *>(&reply_pk));

      LogInfo("end dispatch client pb request, msgno=" << tmp->msg_req);
      return;
    }

    service_ptr service = (*it).second;

    const google::protobuf::MethodDescriptor *method = service->GetDescriptor()->FindMethodByName(method_name);
    if (!method)
    {
      reply_pk.err_code = ERROR_METHOD_NOT_FOUND;
      std::stringstream ss;
      ss << "not found method_name:[" << method_name << "]";
      LogError(reply_pk.msg_req << "|" << ss.str());
      reply_pk.err_info = ss.str();
      conn->getCodec()->encode(conn->getOutBuffer(), dynamic_cast<AbstractData *>(&reply_pk));
      return;
    }

    google::protobuf::Message *request = service->GetRequestPrototype(method).New();
    LogDebug(reply_pk.msg_req << "|request.name = " << request->GetDescriptor()->full_name());

    if (!request->ParseFromString(tmp->pb_data))
    {
      reply_pk.err_code = ERROR_FAILED_SERIALIZE;
      std::stringstream ss;
      ss << "faild to parse request data, request.name:[" << request->GetDescriptor()->full_name() << "]";
      reply_pk.err_info = ss.str();
      LogError(reply_pk.msg_req << "|" << ss.str());
      delete request;
      conn->getCodec()->encode(conn->getOutBuffer(), dynamic_cast<AbstractData *>(&reply_pk));
      return;
    }

    LogInfo("============================================================");
    LogInfo(reply_pk.msg_req << "|Get client request data:" << request->ShortDebugString());
    LogInfo("============================================================");

    google::protobuf::Message *response = service->GetResponsePrototype(method).New();

    LogDebug(reply_pk.msg_req << "|response.name = " << response->GetDescriptor()->full_name());

    PbRpcController rpc_controller;
    rpc_controller.SetMsgReq(reply_pk.msg_req);
    rpc_controller.SetMethodName(method_name);
    rpc_controller.SetMethodFullName(tmp->service_full_name);

    std::function<void()> reply_package_func = []() {};

    PbRpcClosure closure(reply_package_func);
    service->CallMethod(method, &rpc_controller, request, response, &closure);

    LogInfo("Call [" << reply_pk.service_full_name << "] succ, now send reply package");

    if (!(response->SerializeToString(&(reply_pk.pb_data))))
    {
      reply_pk.pb_data = "";
      LogError(reply_pk.msg_req << "|reply error! encode reply package error");
      reply_pk.err_code = ERROR_FAILED_SERIALIZE;
      reply_pk.err_info = "failed to serilize relpy data";
    }
    else
    {
      LogInfo("============================================================");
      LogInfo(reply_pk.msg_req << "|Set server response data:" << response->ShortDebugString());
      LogInfo("============================================================");
    }

    delete request;
    delete response;

    conn->getCodec()->encode(conn->getOutBuffer(), dynamic_cast<AbstractData *>(&reply_pk));
  }

  bool PbRpcDispacther::parseServiceFullName(const std::string &full_name, std::string &service_name, std::string &method_name)
  {
    if (full_name.empty())
    {
      LogError("service_full_name empty");
      return false;
    }
    std::size_t i = full_name.find(".");
    if (i == full_name.npos)
    {
      LogError("not found [.]");
      return false;
    }

    service_name = full_name.substr(0, i);
    LogDebug("service_name = " << service_name);
    method_name = full_name.substr(i + 1, full_name.length() - i - 1);
    LogDebug("method_name = " << method_name);

    return true;
  }

  void PbRpcDispacther::registerService(service_ptr service)
  {
    std::string service_name = service->GetDescriptor()->full_name();
    m_service_map[service_name] = service;
    LogInfo("succ register service[" << service_name << "]!");
  }

}