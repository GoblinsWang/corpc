#ifndef CORPC_NET_PB_PB_CODEC_H
#define CORPC_NET_PB_PB_CODEC_H

#include <stdint.h>
#include "../abstract_codec.h"
#include "../abstract_data.h"
#include "pb_data.h"

namespace corpc
{

	class PbCodeC : public AbstractCodeC
	{
	public:
		// typedef std::shared_ptr<PbCodeC> ptr;

		PbCodeC();

		~PbCodeC();

		// overwrite
		void encode(TcpBuffer *buf, AbstractData *data);

		// overwrite
		void decode(TcpBuffer *buf, AbstractData *data);

		// overwrite
		virtual ProtocalType getProtocalType();

		const char *encodePbData(PbStruct *data, int &len);
	};

}

#endif
