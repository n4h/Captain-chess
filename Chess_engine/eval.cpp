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

#include "eval.hpp"
#include "board.hpp"
#include "auxiliary.hpp"
#include "constants.hpp"

namespace eval
{
	using namespace aux;
	using namespace constants;

	unsigned int getCaptureValue(board::Move m)
	{
		switch (board::getMoveInfo<constants::moveTypeMask>(m))
		{
		case capP:
			return pawn_val;
		case knightPromoCapN:
		case bishopPromoCapN:
		case rookPromoCapN:
		case queenPromoCapN:
		case capN:
			return knight_val;
		case knightPromoCapB:
		case bishopPromoCapB:
		case rookPromoCapB:
		case queenPromoCapB:
		case capB:
			return bishop_val;
		case knightPromoCapR:
		case bishopPromoCapR:
		case rookPromoCapR:
		case queenPromoCapR:
		case capR:
			return rook_val;
		case knightPromoCapQ:
		case bishopPromoCapQ:
		case rookPromoCapQ:
		case queenPromoCapQ:
		case capQ:
			return queen_val;
		default:
			return 0U;
		}
	}
}