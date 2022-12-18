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

	// move generation is based on "Hyperbola Quintessence" algorithm
	// https://www.chessprogramming.org/Hyperbola_Quintessence
	constexpr board::Bitboard hypqDiag(board::Bitboard o, board::square idx)
	{
		o &= board::diagMask(idx);
		board::Bitboard r = aux::setbit(idx);
		board::Bitboard orev = _byteswap_uint64(o);
		board::Bitboard rrev = _byteswap_uint64(r);

		return board::diagMask(idx) & ((o - 2 * r) ^ _byteswap_uint64(orev - 2 * rrev));
	}

	constexpr board::Bitboard hypqAntDiag(board::Bitboard o, board::square idx)
	{
		o &= board::antiDiagMask(idx);
		board::Bitboard r = aux::setbit(idx);
		board::Bitboard orev = _byteswap_uint64(o);
		board::Bitboard rrev = _byteswap_uint64(r);

		return board::antiDiagMask(idx) & ((o - 2 * r) ^ _byteswap_uint64(orev - 2 * rrev));
	}

	constexpr board::Bitboard hypqFile(board::Bitboard o, board::square idx)
	{
		o &= board::fileMask(idx);
		board::Bitboard r = aux::setbit(idx);
		board::Bitboard orev = _byteswap_uint64(o);
		board::Bitboard rrev = _byteswap_uint64(r);

		return board::fileMask(idx) & ((o - 2 * r) ^ _byteswap_uint64(orev - 2 * rrev));
	}

	board::Bitboard hypqRank(board::Bitboard o, board::square idx);

	constexpr board::Bitboard knightAttacks(board::square idx)
	{
		board::Bitboard knight = aux::setbit(idx);
		board::Bitboard n
	}

	template<bool wToMove, std::size_t N, bool qSearch = false>
	std::size_t genMoves(const board::QBB& b, std::array<board::Move, N>& ml, std::size_t i)
	{
		
		

		genPawnMoves<wToMove>(b, heading, ml, i);

		genMovesForPiece<wToMove, board::king>(b, heading, ml, i);

		return i;
	}
}
#endif