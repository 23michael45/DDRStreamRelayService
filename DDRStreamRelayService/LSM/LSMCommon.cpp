#include "LSMCommon.h"

#pragma comment (lib,"ws2_32.lib")

namespace DDRCommLib { namespace MARS {
	
bool _receive(SOCKET s, std::vector<char> &vec) {
	const int nBytesPerAttempt = 1024;
	const timeval ztv = { 0, 0 };
	fd_set sset;
	while (1) {
		FD_ZERO(&sset);
		FD_SET(s, &sset);
		if (select(0, &sset, nullptr, nullptr, &ztv) == 0) {
			break;
		}
		int pos = (int)vec.size();
		vec.resize(pos + nBytesPerAttempt);
		int n = recv(s, &vec[pos], nBytesPerAttempt, 0);
		if (n <= 0) { return false; }
		if (n < nBytesPerAttempt) {
			vec.resize(pos + n);
			return true;
		}
	}
	return true;
}


}}