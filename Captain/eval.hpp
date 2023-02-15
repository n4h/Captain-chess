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

#ifndef EVALUATION_H
#define EVALUATION_H

#include <cstdint>
#include <cstddef>
#include <array>

#include "auxiliary.hpp"
#include "board.hpp"
#include "movegen.hpp"

namespace eval
{
	using namespace aux;
	using Eval = std::int16_t;

	constexpr std::array<Eval, 64> PSQTknight = {
		200, 250, 210, 210, 210, 210, 250, 200,
		250, 210, 300, 300, 300, 300, 210, 250,
		250, 300, 360, 360, 360, 360, 300, 250,
		250, 300, 360, 360, 360, 360, 300, 250,
		250, 300, 360, 360, 360, 360, 300, 250,
		250, 300, 360, 360, 360, 360, 300, 250,
		250, 210, 300, 300, 300, 300, 210, 250,
		200, 250, 210, 210, 210, 210, 250, 200
	};

	constexpr std::array<Eval, 64> PSQTbishop = {
		270, 270, 270, 270, 270, 270, 270, 270,
		270, 310, 300, 300, 300, 300, 310, 270,
		270, 300, 330, 330, 330, 330, 300, 270,
		270, 300, 300, 330, 330, 300, 300, 270,
		270, 300, 300, 330, 330, 300, 300, 270,
		270, 300, 330, 330, 330, 330, 300, 270,
		270, 310, 300, 300, 300, 300, 310, 270,
		270, 270, 270, 270, 270, 270, 270, 270
	};

	constexpr std::array<Eval, 64> PSQTrookw = {
		500, 500, 500, 500, 500, 500, 500, 500,
		500, 500, 500, 500, 500, 500, 500, 500,
		500, 500, 500, 500, 500, 500, 500, 500,
		500, 500, 500, 500, 500, 500, 500, 500,
		500, 500, 500, 500, 500, 500, 500, 500,
		500, 500, 500, 500, 500, 500, 500, 500,
		525, 525, 525, 525, 525, 525, 525, 525,
		500, 500, 500, 500, 500, 500, 500, 500
	};

	constexpr std::array<Eval, 64> PSQTrookb = {
		500, 500, 500, 500, 500, 500, 500, 500,
		525, 525, 525, 525, 525, 525, 525, 525,
		500, 500, 500, 500, 500, 500, 500, 500,
		500, 500, 500, 500, 500, 500, 500, 500,
		500, 500, 500, 500, 500, 500, 500, 500,
		500, 500, 500, 500, 500, 500, 500, 500,
		500, 500, 500, 500, 500, 500, 500, 500,
		500, 500, 500, 500, 500, 500, 500, 500
	};

	constexpr std::array<Eval, 64> PSQTpawnw = {
		0, 0, 0, 0, 0, 0, 0, 0,
		100, 100, 100, 80, 80, 100, 100, 100,
		100, 100, 100, 100, 100, 100, 100, 100,
		100, 100, 100, 110, 110, 100, 100, 100,
		130, 130, 130, 130, 130, 130, 130, 130,
		150, 150, 150, 150, 150, 150, 150, 150,
		200, 200, 200, 200, 200, 200, 200, 200,
		0, 0, 0, 0, 0, 0, 0, 0
	};

	constexpr std::array<Eval, 64> PSQTpawnb = {
		0, 0, 0, 0, 0, 0, 0, 0,
		200, 200, 200, 200, 200, 200, 200, 200,
		150, 150, 150, 150, 150, 150, 150, 150,
		130, 130, 130, 130, 130, 130, 130, 130,
		100, 100, 100, 110, 110, 100, 100, 100,
		100, 100, 100, 100, 100, 100, 100, 100,
		100, 100, 100, 80, 80, 100, 100, 100,
		0, 0, 0, 0, 0, 0, 0, 0
	};

	constexpr std::array<Eval, 64> PSQTqueen = {
		810, 810, 810, 810, 810, 810, 810, 810,
		810, 810, 810, 900, 900, 810, 810, 810,
		900, 900, 930, 930, 930, 930, 900, 900,
		900, 900, 930, 930, 930, 930, 900, 900,
		900, 900, 930, 930, 930, 930, 900, 900,
		900, 900, 930, 930, 930, 930, 900, 900,
		810, 810, 810, 900, 900, 810, 810, 810,
		810, 810, 810, 810, 810, 810, 810, 810
	};

	constexpr std::array<Eval, 64> PSQTking = {
		0, 0, 20, 0, 0, 0, 20, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 20, 0, 0, 0, 20, 0
	};

	constexpr std::array<Eval, 64> PSQTkingEnd = {
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 15, 15, 15, 15, 0, 0,
	0, 0, 15, 20, 20, 15, 0, 0,
	0, 0, 15, 20, 20, 15, 0, 0,
	0, 0, 15, 15, 15, 15, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0
	};

	std::uint32_t getLVA(const board::QBB&, board::Bitboard, board::Bitboard&);
	Eval getCaptureValue(const board::QBB&, board::Move);
	Eval mvvlva(const board::QBB&, board::Move);
	Eval see(const board::QBB&, board::Move);
	Eval evalCapture(const board::QBB&, board::Move);

	Eval computeMaterialValue(board::Bitboard, const std::array<Eval, 64>&);

	// pawnCount = number of pawns on same color square as bishop
	constexpr Eval pawnCountBishopPenalty(unsigned pawnCount)
	{
		return (pawnCount >= 6) * 50;
	}

	constexpr Eval bishopOpenDiagonalBonus(bool open)
	{
		return open ? 15 : 0;
	}

	constexpr Eval rookOpenFileBonus(bool open)
	{
		return open ? 25 : 0;
	}

	constexpr Eval bishopPairBonus(bool pair)
	{
		return pair ? 25 : 0;
	}

	constexpr Eval aggressionBonus(board::square psq, board::square enemyKingSq, int closeness, Eval bonus)
	{
		int pRank = rank(psq);
		int pFile = file(psq);
		int kRank = rank(enemyKingSq);
		int kFile = file(enemyKingSq);
		return (l1dist(pRank, pFile, kRank, kFile) <= closeness) * bonus;
	}

	enum class OutpostType{MyOutpost, OppOutpost};
	template<OutpostType t>
	constexpr Eval knightOutpostBonus(board::square knightsq, board::Bitboard myPawns, board::Bitboard enemyPawns)
	{
		if constexpr (t == OutpostType::MyOutpost)
		{
			board::Bitboard myKnight = setbit(knightsq);
			if ((myKnight & constants::topHalf) && (movegen::pawnAttacks(myPawns) & myKnight))
			{
				myKnight |= movegen::KSNorth(0, myKnight);
				myKnight = movegen::pawnAttacks(myKnight);
				if (!(myKnight & enemyPawns))
				{
					return 15;
				}
			}
			return 0;
		}
		else if constexpr (t == OutpostType::OppOutpost)
		{
			board::Bitboard myKnight = setbit(knightsq);
			if ((myKnight & constants::botHalf) && (movegen::enemyPawnAttacks(enemyPawns) & myKnight))
			{
				myKnight |= movegen::KSSouth(0, myKnight);
				myKnight = movegen::enemyPawnAttacks(myKnight);
				if (!(myKnight & myPawns))
				{
					return 15;
				}
			}
			return 0;
		}
	}

	Eval evaluate(const board::QBB& b);

}
#endif