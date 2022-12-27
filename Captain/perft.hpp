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
		void perft(const board::QBB& b, std::size_t t);
		std::size_t getResult();
		void reset();
		Perft();
		Perft(board::QBB& b, std::size_t t);
	private:
		std::size_t perftResult = 0;

	};
}
#endif