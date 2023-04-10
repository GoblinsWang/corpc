/***
	@author: Wangzhiming
	@date: 2022-10-29
***/
#include "processor_selector.h"
#include "processor.h"

namespace corpc
{

	Processor *ProcessorSelector::next()
	{
		int n = static_cast<int>(m_processors.size());
		if (!n)
		{
			return nullptr;
		}

		int minCoProIdx = 0;
		size_t minCoCnt = m_processors.front()->getCoCnt();
		switch (m_strategy)
		{
		case MIN_EVENT_FIRST:
			for (int i = 1; i < n; ++i)
			{
				size_t coCnt = m_processors[i]->getCoCnt();
				if (coCnt < minCoCnt)
				{
					minCoCnt = coCnt;
					minCoProIdx = i;
				}
			}
			m_curpro_id = minCoProIdx;
			break;

		case ROUND_ROBIN:
		default:
			++m_curpro_id;
			if (m_curpro_id >= n)
			{
				m_curpro_id = 1;
			}
			break;
		}

		return m_processors[m_curpro_id];
	};
}