/*
Copyright 2022-2023, Narbeh Mouradian

Captain is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Captain is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <https://www.gnu.org/licenses/>.

*/

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