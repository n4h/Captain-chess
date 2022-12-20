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

#include <cstddef>

#include "perft.hpp"
#include "board.hpp"


namespace perft
{
	Perft::Perft(){}

	Perft::Perft(board::QBB& b, std::size_t t)
	{
		perft(b, t, (std::size_t)0);
	}

	void Perft::perft(const board::QBB& b, std::size_t t, std::size_t firstMove)
	{
		if (t-- == 0)
		{
			++perftResult;
			return;
		}

		const std::size_t j = movegen::genMoves(b, moveList, firstMove);

		board::QBB bcopy = b;

		for (std::size_t k = firstMove; k != j; ++k)
		{
			bcopy.makeMove(moveList[k]);

			perft(bcopy, t, j);
			bcopy = b;
		}
	}

	std::size_t Perft::getResult()
	{
		return perftResult;
	}

	void Perft::reset()
	{
		perftResult = 0;
	}
}