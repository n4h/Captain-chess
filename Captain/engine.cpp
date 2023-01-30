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

#include <immintrin.h>

#pragma intrinsic(_BitScanForward64)

#include <algorithm>
#include <iostream>
#include <chrono>
#include <string>
#include <sstream>
#include <cassert>
#include <ranges>

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

	std::string Engine::move2uciFormat(const board::QBB& b, board::Move m)
	{ // TODO only works for root move
		std::ostringstream oss;
		auto from = board::getMoveInfo<constants::fromMask>(m);
		auto to = board::getMoveInfo<constants::toMask>(m);
		auto fromfile = aux::file(from);
		auto fromrank = b.getColorToPlay() == board::Color::White ? aux::rank(from) + 1 : 7 - aux::rank(from) + 1;
		auto tofile = aux::file(to);
		auto torank = b.getColorToPlay() == board::Color::White ? aux::rank(to) + 1 : 7 - aux::rank(to) + 1;

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

	std::chrono::milliseconds Engine::elapsed() const
	{
		return aux::castms(std::chrono::steady_clock::now() - searchStart);
	}

	bool Engine::threeFoldRep() const
	{
		std::size_t cnt = 0;
		std::uint64_t currHash = prevPos.back();
		for (int i = prevPos.size() - 1; i >= 0; i -= 2)
		{
			if (prevPos[i] == currHash)
				++cnt;
		}
		return cnt >= 3;
	}

	void Engine::uciUpdate()
	{
		if (aux::castsec(std::chrono::steady_clock::now() - lastUpdate).count() >= 2)
		{
			lastUpdate = std::chrono::steady_clock::now();
			auto seconds = aux::castsec(elapsed()).count();
			if (seconds > 0)
			{
				engine_out << "info depth " << currIDdepth << " nodes " << nodes << " nps " << nodes / seconds << std::endl;
				engine_out.emit();
			}
		}
	}

	bool Engine::shouldStop() noexcept
	{
		if (settings.ponder)
			return false;

		bool overtime = !settings.infiniteSearch && (elapsed() > moveTime || elapsed() > settings.maxTime);

		return overtime || (nodes > settings.maxNodes) || (elapsed() > settings.maxTime);
	}
	
	void Engine::rootSearch(const board::QBB& b, std::chrono::time_point<std::chrono::steady_clock> s,
		const MoveHistory& moveHist, const PositionHistory& posHist)
	{
		searchStart = s;
		lastUpdate = s;
		prevMoves = moveHist;
		prevPos = posHist;
		
		engineW = b.isWhiteToPlay();
		currIDdepth = 0;
		nodes = 0;
		if (tt != nullptr)
			hash = prevPos.back();
		else
			hash = 0;
		auto mytime = engineW ? settings.wmsec : settings.bmsec;
		auto myinc = engineW ? settings.winc : settings.binc;
		auto moveNumber = (prevPos.size() + 2) / 2;
		if (settings.movestogo == std::numeric_limits<std::size_t>::max() || settings.movestogo == 0)
		{
			moveTime = aux::castms(moveNumber < 12 ? mytime / 40 : 0.1 * mytime);
		}
		else
		{
			moveTime = aux::castms( ((0.95*mytime) / settings.movestogo) + (myinc / 3));
		}

		movegen::Movelist moves;
		movegen::genMoves(b, moves);
		
		std::array<std::pair<board::Move, Eval>, 218> rootMoves;
		for (std::size_t i = 0; i != moves.size(); ++i)
		{
			rootMoves[i].first = moves[i];
			rootMoves[i].second = negInf;
		}

		Eval worstCase = negInf;

		board::QBB bcopy = b;
		for (unsigned int k = 1; k <= posInf; ++k)
		{
			currIDdepth = k;
			worstCase = negInf;
			Eval score = negInf;

			for (std::size_t i = 0; i != moves.size(); ++i)
			{
				if (!searchFlags::searching.test())
					goto endsearch;
				bcopy.makeMove(rootMoves[i].first);
				prevMoves.push_back(rootMoves[i].first);
				auto oldhash = hash;
				hash ^= tt->incrementalUpdate(rootMoves[i].first, b, bcopy);
				try 
				{
					rootMoves[i].second = -alphaBetaSearch(bcopy, negInf, -worstCase, k - 1, false);
				}
				catch (const Timeout&)
				{
					prevMoves.pop_back();
					goto endsearch;
				}
				score = std::max(score, rootMoves[i].second);
				prevMoves.pop_back();
				bcopy = b;
				hash = oldhash;
				if (score > worstCase)
					worstCase = score;
			}

			std::stable_sort(rootMoves.begin(), rootMoves.begin() + moves.size(), [](const auto& a, const auto& b) {
				return a.second > b.second;
				});
		}
	endsearch:
		searchFlags::searching.clear();
		engine_out << "bestmove " << move2uciFormat(b, rootMoves[0].first) << std::endl;
		engine_out.emit();
	}

	Eval Engine::quiesceSearch(const board::QBB& b, Eval alpha, Eval beta, int depth)
	{
		const Eval checkpos = eval::evaluate(b);
		AddPosition recordNode(prevPos, hash);

		if (threeFoldRep())
			return 0;
		if (b.get50() == 50)
			return 0;
		if (checkpos > beta)
			return checkpos;
		else if (checkpos > alpha)
			alpha = checkpos;
		if (shouldStop())
			searchFlags::searching.clear();
		++nodes;

		if (tt)
		{
			if ((*tt)[hash].key == hash && (*tt)[hash].depth > depth)
			{
				auto nodetype = (*tt)[hash].nodeType;
				auto eval = (*tt)[hash].eval;
				if (nodetype == TTable::PV)
					return eval;
				else if (nodetype == TTable::ALL && eval < alpha)
					return eval;
				else if (nodetype == TTable::CUT && eval > beta)
					return eval;
			}
		}


		movegen::Movelist<movegen::ScoredMove> ml;
		movegen::genMoves<movegen::QSearch>(b, ml);
		auto captureIterations = ml.size();
		bool check = movegen::isInCheck(b);
		if (!check && !ml.size())
		{
			movegen::genMoves<!movegen::QSearch, movegen::Quiets>(b, ml);
			if (!ml.size())
			{
				return 0;
			}
			else
			{
				return checkpos;
			}
		}
		else if (check && !ml.size())
		{
			movegen::genMoves<!movegen::QSearch, movegen::Quiets>(b, ml);
			if (!ml.size())
			{
				return negInf;
			}
		}


		Eval currEval = checkpos;

		board::QBB bcopy = b;

		for (std::size_t i = 0; auto& [move, score] : ml)
		{
			if (i < captureIterations)
				score = eval::mvvlva(b, move);
			++i;
		}

		for (std::size_t i = 0; i != ml.size(); ++i)
		{
			if (i + 1 < captureIterations)
			{
				std::iter_swap(ml.begin() + i, std::max_element(ml.begin() + i, ml.end()));
			}
			if (i < captureIterations)
			{
				if (eval::getCaptureValue(b, ml[i].m) + 200 + checkpos <= alpha)
				{
					if (check && i + 1 == captureIterations)
					{
						movegen::genMoves<!movegen::QSearch, movegen::Quiets>(b, ml);
					}
					continue;
				}
				if (ml[i].score < 0)
				{
					ml[i].score = eval::see(b, ml[i].m);
					if (ml[i].score < 0)
					{
						if (check && i + 1 == captureIterations)
						{
							movegen::genMoves<!movegen::QSearch, movegen::Quiets>(b, ml);
						}
						continue;
					}
				}
			}
			if (!searchFlags::searching.test())
				throw Timeout();
			auto oldhash = hash;
			bcopy.makeMove(ml[i].m);
			prevMoves.push_back(ml[i].m);
			hash ^= tt->incrementalUpdate(ml[i].m, b, bcopy);
			
			currEval = std::max<Eval>(currEval, -quiesceSearch(bcopy, -beta, -alpha, depth - 1));
			prevMoves.pop_back();
			bcopy = b;
			hash = oldhash;
			alpha = std::max(currEval, alpha);
			if (alpha > beta)
				return currEval;
			if (check && i + 1 == captureIterations)
			{
				movegen::genMoves<!movegen::QSearch, movegen::Quiets>(b, ml);
			}
		}
		return currEval;
	}

	Eval Engine::alphaBetaSearch(const board::QBB& b, Eval alpha, Eval beta, int depth, bool nullBranch)
	{
		auto nodeType = TTable::ALL;
		AddPosition recordNode(prevPos, hash);

		if (threeFoldRep())
			return 0;
		if (shouldStop())
			searchFlags::searching.clear();
		uciUpdate();
		if (b.get50() == 50)
			return 0;
		if (depth <= 0)
			return quiesceSearch(b, alpha, beta, depth);
		++nodes;
		
		if (tt)
		{
			if ((*tt)[hash].key == hash && (*tt)[hash].depth > depth)
			{
				auto nodetype = (*tt)[hash].nodeType;
				auto eval = (*tt)[hash].eval;
				if (nodetype == TTable::PV)
					return eval;
				else if (nodetype == TTable::ALL && eval < alpha)
					return eval;
				else if (nodetype == TTable::CUT && eval > beta)
					return eval;
			}
		}

		if (!nullBranch && !movegen::isInCheck(b))
		{
			board::QBB bnull = b;
			auto oldhash = hash;
			hash ^= tt->nullUpdate(bnull);
			bnull.doNullMove();
			prevMoves.push_back(0);
			Eval nulleval = -alphaBetaSearch(bnull, -beta, -alpha, depth - 3, true);
			hash = oldhash;
			prevMoves.pop_back();
			if (nulleval > beta)
				return nulleval;
		}


		board::Move topMove = 0;

		Eval currEval = negInf;
		movegen::MoveOrder moves(tt, b, hash);
		board::Move nextMove = 0;
		board::QBB bcopy = b;
		std::size_t i = 0;
		for (; moves.next(b, nextMove); ++i)
		{
			if (!searchFlags::searching.test())
				throw Timeout();
			auto oldhash = hash;
			bcopy.makeMove(nextMove);
			prevMoves.push_back(nextMove);
			hash ^= tt->incrementalUpdate(nextMove, b, bcopy);
			
			currEval = std::max<Eval>(currEval, -alphaBetaSearch(bcopy, -beta, -alpha, depth - 1, nullBranch));
			prevMoves.pop_back();
			bcopy = b;
			hash = oldhash;
			if (currEval > alpha)
			{
				nodeType = TTable::PV;
				topMove = nextMove;
				alpha = currEval;
			}
			if (alpha > beta)
			{
				nodeType = TTable::CUT;
				if (tt)
					tt->store(hash, depth, currEval, topMove, nodeType);
				return currEval;
			}
		}
		if (i == 0)
		{
			return movegen::isInCheck(b) ? negInf : 0;
		}
		if (tt)
			tt->store(hash, depth, currEval, topMove, nodeType);
		return currEval;
	}
}