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

#ifndef MOVEGEN_H
#define MOVEGEN_H

#include <immintrin.h>
#include <intrin.h>

#pragma intrinsic(__popcnt64)
#pragma intrinsic(_BitScanForward64)
#pragma intrinsic(_pdep_u64)
#pragma intrinsic(_pext_u64)


#include <cstdint>
#include <cstddef>
#include <type_traits>
#include <cassert>
#include <array>

#include "board.hpp"
#include "auxiliary.hpp"
#include "constants.hpp"

namespace movegen
{
	// the idea of templatizing the move generation function
	// by the color-to-move is due to the Gigantua move generator
	// https://github.com/Gigantua/Gigantua

	using attackMap = std::uint64_t;

	constexpr bool kingSideCastle = true;
	constexpr bool queenSideCastle = false;

	struct MoveList
	{
		std::array<board::Move, 218> moves;
		unsigned char mvCnt = 0;
		board::Move& operator[](unsigned char k)
		{
			return moves[k];
		}
	};

	board::Bitboard dumbFill(board::Bitboard loc, board::Bitboard occ, int r, int f);
	board::Bitboard filterEdgesRook(board::Bitboard rookMoves, std::size_t pos);
	void initRookBishopAttacks();
	void initAttacks();

	template<bool wToMove, std::size_t N, bool qSearch = false>
	std::size_t genMoves(board::QBB& b, std::array<board::Move, N>& ml, std::size_t i)
	{
		board::Move heading = b.getHeading();


		genPawnMoves<wToMove>(b, heading, ml, i);

		genMovesForPiece<wToMove, board::king>(b, heading, ml, i);

		return i;
	}
}
#endif