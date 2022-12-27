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

#include <immintrin.h>

#pragma intrinsic(_BitScanForward64)

#include <algorithm>
#include <iostream>
#include <chrono>
#include <string>
#include <sstream>

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

	void Engine::setTTable(TTable::TTable* x)
	{
		tt = x;
	}

	std::string Engine::move2uciFormat(board::Move m)
	{
		std::ostringstream oss;
		oss << aux::file2char(aux::file(board::getMoveInfo<constants::fromMask>(m)));
		oss << aux::rank(board::getMoveInfo<constants::fromMask>(m)) + 1;
		oss << aux::file2char(aux::file(board::getMoveInfo<constants::toMask>(m)));
		oss << aux::rank(board::getMoveInfo<constants::toMask>(m)) + 1;
		if (board::getPromoPiece(m) != board::king)
		{
			oss << board::promoFlag2char(m);
		}
		return oss.str();
	}

	void Engine::playBestMove(const board::QBB& bCopy, std::chrono::time_point<std::chrono::steady_clock> s)
	{
		// TODO time management
		searchStart = s;
		//engineW = bCopy.wMoving;
		board::Move m = 0;
		currIDdepth = 0;
		b = bCopy;

		nodes = 0;
		if (tt != nullptr)
			initialHash();
		else
			hash = 0;
		// TODO call rootSearch

		sync_cout << "bestmove " << move2uciFormat(m) << sync_endl;
		searchFlags::searching.clear();
	}

	bool Engine::shouldStop() noexcept
	{
		if (settings.ponder)
			return false;

		auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - searchStart);

		bool overtime = !settings.infiniteSearch && (elapsed > moveTime || elapsed > settings.maxTime);

		return overtime || (nodes > settings.maxNodes) || (elapsed > settings.maxTime);
	}
	void Engine::initialHash()
	{
		
	}

	// TODO rewrite alpha beta search

	std::int32_t Engine::alphaBetaSearch(std::int32_t alpha, std::int32_t beta, int depth)
	{
		return alpha + beta + depth;
	}
}