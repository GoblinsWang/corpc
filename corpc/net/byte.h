#ifndef CORPC_NET_BYTE_H
#define CORPC_NET_BYTE_H

#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>

namespace corpc
{

  int32_t getInt32FromNetByte(const char *buf)
  {
    int32_t tmp;
    memcpy(&tmp, buf, sizeof(tmp));
    return ntohl(tmp);
  }

}

#endif
