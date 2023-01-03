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

#include <cstddef>

#include "perft.hpp"
#include "board.hpp"


namespace perft
{
	Perft::Perft(){}

	Perft::Perft(board::QBB& b, std::size_t t)
	{
		perft(b, t);
	}

	void Perft::perft(const board::QBB& b, std::size_t t)
	{
		if (t-- == 0)
		{
			++perftResult;
			return;
		}
		movegen::Movelist<218> moveList;
		movegen::genMoves(b, moveList);

		board::QBB bcopy = b;

		for (std::size_t k = 0; k != moveList.size(); ++k)
		{
			bcopy.makeMove(moveList[k]);

			perft(bcopy, t);
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