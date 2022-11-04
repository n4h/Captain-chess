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

#ifndef EVALUATION_H
#define EVALUATION_H

#include <cstdint>
#include <cstddef>
#include <array>

#include "auxiliary.hpp"
#include "board.hpp"

namespace eval
{
	using namespace aux;

	constexpr std::array<std::int32_t, 64> PSQTknight = {
		200, 250, 210, 210, 210, 210, 250, 200,
		250, 210, 300, 300, 300, 300, 210, 250,
		250, 300, 360, 360, 360, 360, 300, 250,
		250, 300, 360, 360, 360, 360, 300, 250,
		250, 300, 360, 360, 360, 360, 300, 250,
		250, 300, 360, 360, 360, 360, 300, 250,
		250, 210, 300, 300, 300, 300, 210, 250,
		200, 250, 210, 210, 210, 210, 250, 200
	};

	constexpr std::array<std::int32_t, 64> PSQTbishop = {
		270, 270, 270, 270, 270, 270, 270, 270,
		270, 330, 300, 300, 300, 300, 330, 270,
		270, 300, 330, 330, 330, 330, 300, 270,
		270, 300, 300, 330, 330, 300, 300, 270,
		270, 300, 300, 330, 330, 300, 300, 270,
		270, 300, 330, 330, 330, 330, 300, 270,
		270, 330, 300, 300, 300, 300, 330, 270,
		270, 270, 270, 270, 270, 270, 270, 270
	};

	constexpr std::array<std::int32_t, 64> PSQTrookw = {
		500, 500, 500, 525, 525, 500, 500, 500,
		500, 500, 500, 525, 525, 500, 500, 500,
		500, 500, 500, 525, 525, 500, 500, 500,
		500, 500, 500, 525, 525, 500, 500, 500,
		500, 500, 500, 525, 525, 500, 500, 500,
		500, 500, 500, 525, 525, 500, 500, 500,
		550, 550, 550, 550, 550, 550, 550, 550,
		525, 525, 525, 525, 525, 525, 525, 525
	};

	constexpr std::array<std::int32_t, 64> PSQTrookb = {
		525, 525, 525, 525, 525, 525, 525, 525,
		550, 550, 550, 550, 550, 550, 550, 550,
		500, 500, 500, 525, 525, 500, 500, 500,
		500, 500, 500, 525, 525, 500, 500, 500,
		500, 500, 500, 525, 525, 500, 500, 500,
		500, 500, 500, 525, 525, 500, 500, 500,
		500, 500, 500, 525, 525, 500, 500, 500,
		500, 500, 500, 525, 525, 500, 500, 500
	};

	constexpr std::array<std::int32_t, 64> PSQTpawnw = {
		0, 0, 0, 0, 0, 0, 0, 0,
		100, 100, 100, 80, 80, 100, 100, 100,
		100, 100, 100, 100, 100, 100, 100, 100,
		100, 100, 100, 110, 110, 100, 100, 100,
		130, 130, 130, 130, 130, 130, 130, 130,
		150, 150, 150, 150, 150, 150, 150, 150,
		200, 200, 200, 200, 200, 200, 200, 200,
		0, 0, 0, 0, 0, 0, 0, 0
	};

	constexpr std::array<std::int32_t, 64> PSQTpawnb = {
		0, 0, 0, 0, 0, 0, 0, 0,
		200, 200, 200, 200, 200, 200, 200, 200,
		150, 150, 150, 150, 150, 150, 150, 150,
		130, 130, 130, 130, 130, 130, 130, 130,
		100, 100, 100, 110, 110, 100, 100, 100,
		100, 100, 100, 100, 100, 100, 100, 100,
		100, 100, 100, 80, 80, 100, 100, 100,
		0, 0, 0, 0, 0, 0, 0, 0
	};

	constexpr std::array<std::int32_t, 64> PSQTqueen = {
		810, 810, 810, 810, 810, 810, 810, 810,
		810, 810, 810, 900, 900, 810, 810, 810,
		900, 900, 900, 900, 900, 900, 900, 900,
		900, 900, 900, 900, 900, 900, 900, 900,
		900, 900, 900, 900, 900, 900, 900, 900,
		900, 900, 900, 900, 900, 900, 900, 900,
		810, 810, 810, 900, 900, 810, 810, 810,
		810, 810, 810, 810, 810, 810, 810, 810
	};

	constexpr std::array<std::int32_t, 64> PSQTking = {
		0, 0, 20, 0, 0, 0, 20, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 20, 0, 0, 0, 20, 0
	};


	std::int32_t computeMaterialValue(board::Bitboard, const std::array<std::int32_t, 64>&);

	template<bool wToMove>
	std::int32_t evaluate(const board::Board& b)
	{
		std::int32_t totalW = 0;

		totalW += computeMaterialValue(b.wPieces[board::pawns], PSQTpawnw);
		totalW += computeMaterialValue(b.wPieces[board::knights], PSQTknight);
		totalW += computeMaterialValue(b.wPieces[board::bishops], PSQTbishop);
		totalW += computeMaterialValue(b.wPieces[board::rooks], PSQTrookw);
		totalW += computeMaterialValue(b.wPieces[board::queens], PSQTqueen);

		std::int32_t totalB = 0;

		totalB += computeMaterialValue(b.bPieces[board::pawns], PSQTpawnb);
		totalB += computeMaterialValue(b.bPieces[board::knights], PSQTknight);
		totalB += computeMaterialValue(b.bPieces[board::bishops], PSQTbishop);
		totalB += computeMaterialValue(b.bPieces[board::rooks], PSQTrookb);
		totalB += computeMaterialValue(b.bPieces[board::queens], PSQTqueen);

		if (totalW + totalB > 3000) // not in endgame
		{
			totalW += computeMaterialValue(b.wPieces[board::king], PSQTking);
			totalB += computeMaterialValue(b.bPieces[board::king], PSQTking);
		}

		std::int32_t eval = totalW - totalB;


		if constexpr (wToMove)
		{
			return eval;
		}
		else
		{
			return -1 * eval;
		}
	}

	unsigned int getCaptureValue(board::Move m);
}
#endif