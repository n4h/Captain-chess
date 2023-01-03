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
	std::string prettyPrintMove(board::Move m, board::Color stm);

	std::size_t perftDivide(const board::QBB& b, board::ExtraBoardInfo ebi, std::size_t t);
}
#endif