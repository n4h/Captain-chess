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

	std::size_t perftDivide(const board::QBB& b, std::size_t t)
	{
		std::array<board::Move, 256> moves;
		std::size_t lastMove = movegen::genMoves(b, moves);
		std::size_t total = 0;

		board::QBB bcopy = b;
		for (std::size_t i = 0; i != lastMove; ++i)
		{
			bcopy.makeMove(moves[i]);
			perft::Perft p{ bcopy, t - 1};
			total += p.getResult();
			std::cout << prettyPrintMove(moves[i]) << ": " << p.getResult() << std::endl;
			p.reset();
			bcopy = b;
		}
		std::cout << "total: " << total << std::endl;
		return total;
	}
}
#endif