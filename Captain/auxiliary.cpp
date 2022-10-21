#include <ostream>
#include <mutex>

#include "auxiliary.hpp"


// from Stockfish
std::ostream& operator<<(std::ostream& o, SyncCout s)
{
	static std::mutex m;
	if (s == ioLock)
		m.lock();
	if (s == ioUnlock)
		m.unlock();
	return o;
}