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
	
	std::int32_t Engine::quiesceSearch(const board::QBB& b, std::int32_t alpha, std::int32_t beta, int depth)
	{
		++nodes;
		const std::int32_t checkpos = eval::evaluate(b);
		if (checkpos > beta)
			return checkpos;
		else if (checkpos > alpha)
			alpha = checkpos;
		if (shouldStop())
			searchFlags::searching.clear();


		movegen::Movelist<218> ml;
		movegen::genMoves(b, ml); // TODO sort moves in Q search

		std::int32_t currEval = checkpos;

		board::QBB bcopy = b;
		for (std::size_t i = 0; i != ml.size(); ++i)
		{
			if (!searchFlags::searching.test())
				return alpha;
			bcopy.makeMove(ml[i]);
			currEval = std::max(currEval, -quiesceSearch(bcopy, -beta, -alpha, depth - 1));
			bcopy = b;
			alpha = std::max(currEval, alpha);
			if (alpha > beta)
				return currEval;
		}
		return currEval;
	}

	std::int32_t Engine::alphaBetaSearch(const board::QBB& b, std::int32_t alpha, std::int32_t beta, int depth, bool prevNull)
	{
		const auto oldAlpha = alpha;
		if (shouldStop())
			searchFlags::searching.clear();
		if (b.get50() == 50)
			return 0;
		if (depth <= 0)
			return quiesceSearch(b, alpha, beta, depth);
		++nodes;
		// TODO TT check

		movegen::Movelist<218> ml;
		movegen::genMoves(b, ml); // TODO sort moves

		std::int32_t currEval = negInf;

		// TODO nullMove
		board::QBB bcopy = b;
		for (std::size_t i = 0; i != ml.size(); ++i)
		{
			if (!searchFlags::searching.test())
				return eval::evaluate(b);
			bcopy.makeMove(ml[i]);
			currEval = std::max(currEval, -alphaBetaSearch(bcopy, -beta, -alpha, depth - 1, !prevNull));
			bcopy = b;
			alpha = std::max(currEval, alpha);
			if (alpha > beta)
				return currEval;
			// TODO TT store
		}
		// TODO TT store at end
		return currEval;
	}
}