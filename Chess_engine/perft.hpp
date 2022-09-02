#ifndef PERFT_H
#define PERFT_H

#include "board.hpp"
#include "movegen.hpp"

#include <algorithm>
#include <cstddef>
#include <cassert>

namespace perft
{
	class Perft
	{
	public:
		template<bool wToMove>
		void perft(board::Board& b, std::size_t t, std::size_t firstMove)
		{
			if (t-- == 0)
			{
				++perftResult;
				return;
			}

			const std::size_t j = movegen::genMoves<wToMove>(b, moveList, firstMove);

			for (std::size_t k = firstMove; k != j; ++k)
			{
				b.makeMove<wToMove>(moveList[k]);
				if (movegen::isInCheck<wToMove>(b))
				{
					b.unmakeMove<wToMove>(moveList[k]);
					continue;
				}
				perft<!wToMove>(b, t, j);
				b.unmakeMove<wToMove>(moveList[k]);
			}
		}
		std::size_t getResult();
		void reset();
		Perft();
		Perft(board::Board& b, std::size_t t, bool wToMove = true);
	private:
		std::size_t perftResult = 0;
		board::Move moveList[2048];

	};
}
#endif