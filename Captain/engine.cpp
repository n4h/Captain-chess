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

	void Engine::playBestMove(const board::Board& bCopy, std::chrono::time_point<std::chrono::steady_clock> s)
	{
		searchStart = s;
		engineW = bCopy.wMoving;
		board::Move m = 0;
		b = bCopy;
		
		if (b.currMove <= 40)
		{
			auto movesLeft = 40 - b.currMove + 1;

			if (engineW)
			{
				moveTime = std::chrono::duration_cast<std::chrono::milliseconds>(0.7 * settings.winc + settings.wmsec / movesLeft);
				moveTime = std::min(moveTime, std::chrono::duration_cast<std::chrono::milliseconds>(0.95 * settings.wmsec));
			}
			else
			{
				moveTime = std::chrono::duration_cast<std::chrono::milliseconds>(0.7 * settings.binc + settings.bmsec / movesLeft);
				moveTime = std::min(moveTime, std::chrono::duration_cast<std::chrono::milliseconds>(0.95 * settings.bmsec));
			}
		}
		else
		{
			if (engineW)
				moveTime = std::chrono::duration_cast<std::chrono::milliseconds>(settings.winc + 0.1 * settings.wmsec);
			else
				moveTime = std::chrono::duration_cast<std::chrono::milliseconds>(settings.binc + 0.1 * settings.bmsec);
		}

		nodes = 0;
		if (tt != nullptr)
			initialHash();
		else
			hash = 0;
		if (b.wMoving)
		{
			m = rootSearch<true>();
		}
		else
		{
			m = rootSearch<false>();
		}

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
		hash = 0;

		if (b.wMoving)
			hash ^= tt->wToMove;

		if (b.flags & board::Board::wkCastleFlagMask)
			hash ^= tt->castling_first[0];
		if (b.flags & board::Board::wqCastleFlagMask)
			hash ^= tt->castling_first[1];
		if (b.flags & board::Board::bkCastleFlagMask)
			hash ^= tt->castling_first[2];
		if (b.flags & board::Board::bqCastleFlagMask)
			hash ^= tt->castling_first[3];

		if (b.epLoc)
		{
			unsigned long index;
			_BitScanForward64(&index, b.epLoc);
			hash ^= tt->enPassant[aux::file(index)];
		}

		for (std::size_t i = 0; i != 6; ++i)
		{
			board::Bitboard wb = b.wPieces[i];
			board::Bitboard bb = b.bPieces[i];
			unsigned long index;
			while (_BitScanForward64(&index, wb))
			{
				wb ^= aux::setbit(index);
				hash ^= tt->wPieces[i][index];
			}
			while (_BitScanForward64(&index, bb))
			{
				bb ^= aux::setbit(index);
				hash ^= tt->bPieces[i][index];
			}
		}
	}
}