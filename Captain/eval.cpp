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

#include <intrin.h>

#pragma intrinsic(_BitScanForward64)

#include "eval.hpp"
#include "board.hpp"
#include "auxiliary.hpp"
#include "constants.hpp"

namespace eval
{
	using namespace aux;
	using namespace constants;

	std::int32_t computeMaterialValue(board::Bitboard bb, const std::array<std::int32_t, 64>& PSQT)
	{
		std::int32_t mval = 0;

		unsigned long index;

		while (_BitScanForward64(&index, bb))
		{
			bb ^= setbit(index);

			mval += PSQT[index];
		}
		return mval;
	}
	std::int32_t evaluate(const board::QBB& b)
	{
		std::int32_t totalW = 0;

		totalW += computeMaterialValue(b.my(b.getPawns()), PSQTpawnw);
		totalW += computeMaterialValue(b.my(b.getKnights()), PSQTknight);
		totalW += computeMaterialValue(b.my(b.getBishops()), PSQTbishop);
		totalW += computeMaterialValue(b.my(b.getRooks()), PSQTrookw);
		totalW += computeMaterialValue(b.my(b.getQueens()), PSQTqueen);

		std::int32_t totalB = 0;

		totalB += computeMaterialValue(b.their(b.getPawns()), PSQTpawnb);
		totalB += computeMaterialValue(b.their(b.getKnights()), PSQTknight);
		totalB += computeMaterialValue(b.their(b.getBishops()), PSQTbishop);
		totalB += computeMaterialValue(b.their(b.getRooks()), PSQTrookb);
		totalB += computeMaterialValue(b.their(b.getQueens()), PSQTqueen);

		if (totalW + totalB > 3000) // not in endgame
		{
			totalW += computeMaterialValue(b.my(b.getKings()), PSQTking);
			totalB += computeMaterialValue(b.their(b.getKings()), PSQTking);
		}

		std::int32_t eval = totalW - totalB;

		return eval;
	}
}