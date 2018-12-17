#ifndef __DDR_LNN_LIB_FACTORY_H_INCLUDED__
#define __DDR_LNN_LIB_FACTORY_H_INCLUDED__

#include <vector>
#include <memory>
#include "LNNNodeGroup.h"

namespace DDRLNN {
	inline std::shared_ptr<LNNNodeGroupInterface> CreateNodeGroup() {
		return std::shared_ptr<LNNNodeGroupInterface>(_createLNNNodeGroup());
	}
	inline bool GetDataFromClientNode(LNNNodeGroupInterface *pLNG,
		                              const char *pClientName,
									  std::vector<BYTE> &vec) {
		vec.resize(0);
		BYTE *ptr = nullptr;
		size_t pos = 0;
		while (1) {
			unsigned int remBytes;
			int nRet = pLNG->GetData(pClientName, ptr, vec.size() - pos, &remBytes);
			if (-1 == nRet) { return false; }
			pos += nRet;
			if (remBytes > 0) {
				vec.resize(pos + remBytes);
				ptr = &vec[pos];
			} else {
				vec.resize(pos);
				break;
			}
		}
		return (vec.size() > 0);
	}
	inline bool _stdcall GetDataFromClientNode(std::shared_ptr<LNNNodeGroupInterface> lng,
		                                       const char *pClientName,
											   std::vector<BYTE> &vec) {
		return GetDataFromClientNode(lng.get(), pClientName, vec);
	}
}

#endif // __DDR_LNN_LIB_FACTORY_H_INCLUDED__