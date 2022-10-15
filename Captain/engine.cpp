/*
Copyright 2022, Narbeh Mouradian

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

#include <algorithm>
#include <iostream>
#include <chrono>

#include "engine.hpp"
#include "board.hpp"
#include "movegen.hpp"
#include "eval.hpp"
#include "auxiliary.hpp"
#include "uci.hpp"

namespace engine
{

	double Engine::getEval()
	{
		return eval;
	}

	void Engine::playBestMove(board::Board b, std::chrono::time_point<std::chrono::steady_clock> s)
	{
		searchStart = s;
		engineW = b.wMoving;
		board::Move m = 0;
		if (b.wMoving)
		{
			m = rootSearch<true>(b);
		}
		else
		{
			m = rootSearch<false>(b);
		}
		std::ostringstream oss;
		oss << aux::file2char(aux::file(board::getMoveInfo<constants::fromMask>(m)));
		oss << aux::rank(board::getMoveInfo<constants::fromMask>(m)) + 1;
		oss << aux::file2char(aux::file(board::getMoveInfo<constants::toMask>(m)));
		oss << aux::rank(board::getMoveInfo<constants::toMask>(m)) + 1;
		sync_cout << "bestmove " << oss.str() << sync_endl;
		searchFlags::searching.clear();
	}

	bool Engine::shouldStop() noexcept
	{
		if (settings.infiniteSearch || settings.ponder)
			return false;

		auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - searchStart);
		bool overtime = false;
		if (engineW)
			overtime = (elapsed > settings.winc + 0.1 * settings.wmsec) || (elapsed > settings.maxTime);
		else
			overtime = (elapsed > settings.binc + 0.1 * settings.bmsec) || (elapsed > settings.maxTime);

		return overtime || (nodes > settings.maxNodes);
	}
}