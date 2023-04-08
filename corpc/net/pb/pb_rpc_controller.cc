#include <google/protobuf/service.h>
#include <google/protobuf/stubs/callback.h>
#include "pb_rpc_controller.h"

namespace corpc
{

    void PbRpcController::Reset() {}

    bool PbRpcController::Failed() const
    {
        return m_is_failed;
    }

    std::string PbRpcController::ErrorText() const
    {
        return m_error_info;
    }

    void PbRpcController::StartCancel() {}

    void PbRpcController::SetFailed(const std::string &reason)
    {
        m_is_failed = true;
        m_error_info = reason;
    }

    bool PbRpcController::IsCanceled() const
    {
        return false;
    }

    void PbRpcController::NotifyOnCancel(google::protobuf::Closure *callback)
    {
    }

    void PbRpcController::SetErrorCode(const int error_code)
    {
        m_error_code = error_code;
    }

    int PbRpcController::ErrorCode() const
    {
        return m_error_code;
    }

    const std::string &PbRpcController::MsgSeq() const
    {
        return m_msg_req;
    }

    void PbRpcController::SetMsgReq(const std::string &msg_req)
    {
        m_msg_req = msg_req;
    }

    void PbRpcController::SetError(const int err_code, const std::string &err_info)
    {
        SetFailed(err_info);
        SetErrorCode(err_code);
    }

    void PbRpcController::SetPeerAddr(NetAddress::ptr addr)
    {
        m_peer_addr = addr;
    }

    void PbRpcController::SetLocalAddr(NetAddress::ptr addr)
    {
        m_local_addr = addr;
    }
    NetAddress::ptr PbRpcController::PeerAddr()
    {
        return m_peer_addr;
    }

    NetAddress::ptr PbRpcController::LocalAddr()
    {
        return m_local_addr;
    }

    void PbRpcController::SetTimeout(const int timeout)
    {
        m_timeout = timeout;
    }
    int PbRpcController::Timeout() const
    {
        return m_timeout;
    }

    void PbRpcController::SetMethodName(const std::string &name)
    {
        m_method_name = name;
    }

    std::string PbRpcController::GetMethodName()
    {
        return m_method_name;
    }

    void PbRpcController::SetMethodFullName(const std::string &name)
    {
        m_full_name = name;
    }

    std::string PbRpcController::GetMethodFullName()
    {
        return m_full_name;
    }

}