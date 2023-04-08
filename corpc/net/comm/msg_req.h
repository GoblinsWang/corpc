#ifndef CORPC_NET_COMM_MSG_REQ_H
#define CORPC_NET_COMM_MSG_REQ_H

#include <string>

namespace corpc
{

  class MsgReqUtil
  {
  public:
    static std::string genMsgNumber();
  };

}

#endif