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
		std::array<board::Move, 1024> moveList;

	};
}
#endif