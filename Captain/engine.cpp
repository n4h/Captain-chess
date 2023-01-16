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
	{ // TODO only works for root move
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
	void Engine::initialHash(const board::QBB& b)
	{
		hash = 0;
		if (!tt) return;
		for (std::size_t i = 0; i != 64; ++i)
		{
			auto piecetype = b.getPieceType(static_cast<board::square>(i));
			if (b.isWhiteToPlay())
			{
				if (piecetype)
				{
					if (piecetype & 1)
						hash ^= tt->whitePSQT[(piecetype >> 1) - 1][i];
					else
						hash ^= tt->blackPSQT[(piecetype >> 1) - 1][i];
				}
			}
			else
			{
				if (piecetype)
				{
					if (piecetype & 1)
						hash ^= tt->blackPSQT[(piecetype >> 1) - 1][aux::flip(i)];
					else
						hash ^= tt->whitePSQT[(piecetype >> 1) - 1][aux::flip(i)];
				}
			}
		}
		
		if (b.isWhiteToPlay())
			hash ^= tt->wToMove;

		if (b.isWhiteToPlay())
		{
			hash ^= b.canCastleLong() ? tt->castling_first[0] : 0;
			hash ^= b.canCastleShort() ? tt->castling_first[1] : 0;
			hash ^= b.oppCanCastleLong() ? tt->castling_first[2] : 0;
			hash ^= b.oppCanCastleShort() ? tt->castling_first[3] : 0;
		}
		else
		{
			hash ^= b.oppCanCastleLong() ? tt->castling_first[0] : 0;
			hash ^= b.oppCanCastleShort() ? tt->castling_first[1] : 0;
			hash ^= b.canCastleLong() ? tt->castling_first[3] : 0;
			hash ^= b.canCastleShort() ? tt->castling_first[4] : 0;
		}

		if (b.enpExists())
			hash ^= tt->enPassant[b.getEnpFile()];
	}
	
	void Engine::rootSearch(const board::QBB& b, std::chrono::time_point<std::chrono::steady_clock> s, board::ExtraBoardInfo e)
	{
		searchStart = s;
		ebi = e;
		engineW = ebi.initialMover == board::Color::White;
		currIDdepth = 0;
		nodes = 0;
		if (tt != nullptr)
			initialHash(b);
		else
			hash = 0;
		auto mytime = engineW ? settings.wmsec : settings.bmsec;
		auto myinc = engineW ? settings.winc : settings.binc;
		moveTime = e.moveNumber < 40 ? std::min(aux::castms(mytime * 0.95), aux::castms((mytime / (40 - e.moveNumber)) + myinc/3)) : aux::castms(mytime / 10);

		movegen::Movelist<218> moves;
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
			sync_cout << "info string iterative deepening " << k << sync_endl;
			currIDdepth = k;
			worstCase = negInf;
			Eval score = negInf;

			for (std::size_t i = 0; i != moves.size(); ++i)
			{
				if (!searchFlags::searching.test())
					goto endsearch;
				bcopy.makeMove(rootMoves[i].first);
				auto oldhash = hash;
				hash ^= tt->incrementalUpdate(rootMoves[i].first, b, bcopy);

				sync_cout << "info currmove " << move2uciFormat(rootMoves[i].first) << sync_endl;
				sync_cout << "info nodes " << nodes << sync_endl;
				try 
				{
					rootMoves[i].second = -alphaBetaSearch(bcopy, negInf, -worstCase, k - 1, false);
				}
				catch (const Timeout&)
				{
					goto endsearch;
				}
				score = std::max(score, rootMoves[i].second);
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
		sync_cout << "bestmove " << move2uciFormat(rootMoves[0].first) << sync_endl;
	}

	Eval Engine::quiesceSearch(const board::QBB& b, Eval alpha, Eval beta, int depth)
	{
		const Eval checkpos = eval::evaluate(b);
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


		movegen::Movelist<218> ml;
		movegen::genMoves<movegen::QSearch>(b, ml); // TODO sort moves in Q search

		if (!ml.size())
		{
			if (movegen::isInCheck(b))
				return negInf;
			else
			{
				if (movegen::genMoves(b, ml); ml.size())
					return checkpos;
				else
					return 0;
			}
		}

		Eval currEval = checkpos;

		board::QBB bcopy = b;
		for (std::size_t i = 0; i != ml.size(); ++i)
		{
			if (!searchFlags::searching.test())
				throw Timeout();
			auto oldhash = hash;
			bcopy.makeMove(ml[i]);
			hash ^= tt->incrementalUpdate(ml[i], b, bcopy);
			currEval = std::max(currEval, (Eval)-quiesceSearch(bcopy, -beta, -alpha, depth - 1));
			bcopy = b;
			hash = oldhash;
			alpha = std::max(currEval, alpha);
			if (alpha > beta)
				return currEval;
		}
		return currEval;
	}

	Eval Engine::alphaBetaSearch(const board::QBB& b, Eval alpha, Eval beta, int depth, bool nullBranch)
	{
		const auto oldAlpha = alpha;
		if (shouldStop())
			searchFlags::searching.clear();
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
			auto nulleval = -alphaBetaSearch(bnull, -beta, -alpha, depth - 3, true);
			hash = oldhash;
			if (nulleval > beta)
				return nulleval;
			else if (nulleval > alpha)
				alpha = nulleval;
		}

		movegen::Movelist<218> ml;
		movegen::genMoves(b, ml); // TODO sort moves

		if (!ml.size())
		{
			if (movegen::isInCheck(b))
				return negInf;
			else
				return 0;
		}

		Eval currEval = negInf;

		
		board::QBB bcopy = b;
		for (std::size_t i = 0; i != ml.size(); ++i)
		{
			if (!searchFlags::searching.test())
				throw Timeout();
			auto oldhash = hash;
			bcopy.makeMove(ml[i]);
			hash ^= tt->incrementalUpdate(ml[i], b, bcopy);
			currEval = std::max(currEval, (Eval) -alphaBetaSearch(bcopy, -beta, -alpha, depth - 1, nullBranch));
			bcopy = b;
			hash = oldhash;
			alpha = std::max(currEval, alpha);
			if (alpha > beta)
			{
				if (tt)
					tt->store(hash, depth, currEval, TTable::CUT);
				return currEval;
			}
		}
		if (tt)
			tt->store(hash, depth, currEval, alpha > oldAlpha ? TTable::PV : TTable::ALL);
		return currEval;
	}
}