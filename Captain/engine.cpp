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
		auto from = board::getMoveInfo<constants::fromMask>(m);
		auto to = board::getMoveInfo<constants::toMask>(m);
		auto fromfile = aux::file(from);
		auto fromrank = ebi.initialMover == board::Color::White ? aux::rank(from) + 1 : 7 - aux::rank(from) + 1;
		auto tofile = aux::file(to);
		auto torank = ebi.initialMover == board::Color::White ? aux::rank(to) + 1 : 7 - aux::rank(to) + 1;

		oss << aux::file2char(fromfile);
		oss << fromrank;
		oss << aux::file2char(tofile);
		oss << torank;
		if (board::getPromoPiece(m) != board::none)
		{
			oss << board::promoFlag2char(m);
		}
		return oss.str();
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
	
	void Engine::rootSearch(const board::QBB& b, std::chrono::time_point<std::chrono::steady_clock> s, board::ExtraBoardInfo e)
	{
		// TODO time management
		searchStart = s;
		ebi = e;
		engineW = ebi.initialMover == board::Color::White;
		currIDdepth = 0;
		nodes = 0;
		if (tt != nullptr)
			initialHash();
		else
			hash = 0;

		movegen::Movelist<218> moves;
		movegen::genMoves(b, moves);
		board::Move bestmove = moves[0];


		// TODO set move scores to negative infinity

		std::int32_t worstCase = negInf;

		board::QBB bcopy = b;
		for (unsigned int k = 1; k <= posInf; ++k)
		{
			sync_cout << "info string iterative deepening " << k << sync_endl;
			currIDdepth = k;
			worstCase = negInf;
			std::int32_t score = negInf;

			for (std::size_t i = 0; i != moves.size(); ++i)
			{
				if (!searchFlags::searching.test())
					goto endsearch;
				bcopy.makeMove(rootMoves[i].first);

				sync_cout << "info currmove " << move2uciFormat(rootMoves[i].first) << sync_endl;
				sync_cout << "info nodes " << nodes << sync_endl;
				rootMoves[i].second =  -alphaBetaSearch(bcopy, negInf, -1 * worstCase, k - 1, false);
				score = std::max(score, rootMoves[i].second);
				bcopy = b;
				if (score > worstCase)
					worstCase = score;
			}

			std::stable_sort(rootMoves.begin(), rootMoves.begin() + j, [](const auto& a, const auto& b) {
				return a.second > b.second;
				});
		}
	endsearch:
		// TODO sort all moves one last time
		searchFlags::searching.clear();
		sync_cout << "bestmove " << move2uciFormat(bestmove) << sync_endl;
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