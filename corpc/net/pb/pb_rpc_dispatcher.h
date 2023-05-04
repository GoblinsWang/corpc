#ifndef CORPC_NET_PB_PB_RPC_DISPATCHER_H
#define CORPC_NET_PB_PB_RPC_DISPATCHER_H

#include <google/protobuf/message.h>
#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>
#include <map>
#include <memory>

#include "../abstract_dispatcher.h"
#include "../pb/pb_data.h"

namespace corpc
{

	class PbRpcDispacther : public AbstractDispatcher
	{
	public:
		// typedef std::shared_ptr<PbRpcDispacther> ptr;
		typedef std::shared_ptr<google::protobuf::Service> service_ptr;

		PbRpcDispacther() = default;
		~PbRpcDispacther() = default;

		void dispatch(AbstractData *data, TcpConnection *conn);

		bool parseServiceFullName(const std::string &full_name, std::string &service_name, std::string &method_name);

		void registerService(service_ptr service);

	public:
		// all services should be registerd on there before progress start
		// key: service_name
		std::map<std::string, service_ptr> m_service_map;
	};

}

#endif