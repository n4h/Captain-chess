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

#ifndef DIVIDE_H
#define DIVIDE_H

#include <cstddef>
#include <iostream>
#include <string>
#include <cassert>
#include <array>

#include "board.hpp"
#include "movegen.hpp"
#include "constants.hpp"
#include "auxiliary.hpp"
#include "perft.hpp"

namespace divide
{
	std::string prettyPrintMove(board::Move m);

	template<bool wToMove>
	std::size_t perftDivide(board::Board& b, std::size_t t, std::size_t firstMove)
	{
		std::array<board::Move, 256> moves;
		std::size_t lastMove = movegen::genMoves<wToMove>(b, moves, firstMove);
		std::size_t total = 0;

		for (std::size_t i = firstMove; i != lastMove; ++i)
		{
			b.makeMove<wToMove>(moves[i]);
			if (!movegen::isInCheck<wToMove>(b))
			{
				b.unmakeMove<wToMove>(b);
				continue;
			}
			perft::Perft p{ b, t - 1, !wToMove };
			total += p.getResult();
			std::cout << prettyPrintMove(moves[i]) << ": " << p.getResult() << std::endl;
			p.reset();
			b.unmakeMove<wToMove>(moves[i]);
		}
		std::cout << "total: " << total << std::endl;
		return total;
	}
}
#endif