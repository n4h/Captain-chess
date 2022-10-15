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

#include <immintrin.h>
#include <intrin.h>

#pragma intrinsic(__popcnt64)
#pragma intrinsic(_BitScanForward64)
#pragma intrinsic(_pdep_u64)
#pragma intrinsic(_pext_u64)

#include <cstddef>

#include "movegen.hpp"
#include "board.hpp"
#include "auxiliary.hpp"

namespace movegen
{
	using aux::getDiag;
	using aux::getAntiDiag;
	using aux::setbit;

	attackMap bishopAttacks[bishopAttacksSize()];
	board::Bitboard bishopMasks[64];
	std::size_t bishopOffsets[64];

	attackMap rookAttacks[rookAttacksSize()];
	board::Bitboard rookMasks[64];
	std::size_t rookOffsets[64];

	attackMap kingAttacks[64];
	attackMap knightAttacks[64];
	attackMap wpawnAttacks[64];
	attackMap bpawnAttacks[64];


	board::Bitboard dumbFill(board::Bitboard loc, board::Bitboard occ, int r, int f)
	{
		unsigned long index = 0;
		_BitScanForward64(&index, loc);

		auto initRank = aux::rank(index);
		auto initFile = aux::file(index);

		board::Bitboard fill = 0;
		for (int i = 1; (initRank + i*r >= 0) && (initRank + i*r <= 7) && (initFile + i*f >= 0) && (initFile + i*f <= 7); ++i)
		{
			board::Bitboard bitToCheck = setbit(initRank + i * r, initFile + i * f);
			if (bitToCheck & occ)
			{
				fill |= bitToCheck;
				return fill;
			}
			else
			{
				fill |= bitToCheck;
			}
		}
		return fill;
	}

	board::Bitboard filterEdgesRook(board::Bitboard rookMoves, std::size_t pos)
	{
		auto leftEdge = board::fileMask[0];
		auto rightEdge = board::fileMask[7];
		auto topEdge = board::rankMask[7];
		auto botEdge = board::rankMask[0];
		auto edge = leftEdge | rightEdge | topEdge | botEdge;

		rookMoves &= ~setbit(pos);

		if (board::isInterior(pos))
		{
			rookMoves &= ~edge;
			return rookMoves;
		}
		switch (pos)
		{
		case board::a1:
			rookMoves &= ~(topEdge | rightEdge);
			return rookMoves;
		case board::h1:
			rookMoves &= ~(topEdge | leftEdge);
			return rookMoves;
		case board::a8:
			rookMoves &= ~(botEdge | rightEdge);
			return rookMoves;
		case board::h8:
			rookMoves &= ~(botEdge | leftEdge);
			return rookMoves;
		default:
			if (board::isLeftEdge(pos)) rookMoves &= ~(topEdge | botEdge | rightEdge);
			if (board::isRightEdge(pos)) rookMoves &= ~(topEdge | botEdge | leftEdge);
			if (board::isTopEdge(pos)) rookMoves &= ~(rightEdge | botEdge | leftEdge);
			if (board::isBottomEdge(pos)) rookMoves &= ~(rightEdge | topEdge | leftEdge);
			return rookMoves;
		}
	}

	void initRookBishopAttacks()
	{
		auto edge = board::fileMask[0] | board::fileMask[7] | board::rankMask[7] | board::rankMask[0];
		std::size_t offsetBishop = 0;
		std::size_t offsetRook = 0;

		for (std::size_t i = 0; i != 64; ++i)
		{
			bishopOffsets[i] = offsetBishop;
			rookOffsets[i] = offsetRook;

			auto diagonals = board::diagMask[getDiag(i)] | board::antidiagMask[getAntiDiag(i)];
			bishopMasks[i] = diagonals & ~setbit(i) & ~edge;
			std::size_t numToReserve = 1ULL << __popcnt64(bishopMasks[i]);
			for (int j = 0; j != numToReserve; ++j)
			{
				bishopAttacks[bishopOffsets[i] + j] = 0;
				auto occ = _pdep_u64(j, bishopMasks[i]);
				bishopAttacks[bishopOffsets[i] + j] |= dumbFill(setbit(i), occ, 1, -1);
				bishopAttacks[bishopOffsets[i] + j] |= dumbFill(setbit(i), occ, 1, 1);
				bishopAttacks[bishopOffsets[i] + j] |= dumbFill(setbit(i), occ, -1, 1);
				bishopAttacks[bishopOffsets[i] + j] |= dumbFill(setbit(i), occ, -1, -1);
			}
			offsetBishop += numToReserve;

			auto rookMoves = board::rankMask[aux::rank(i)] | board::fileMask[aux::file(i)];
			rookMasks[i] = filterEdgesRook(rookMoves, i);

			numToReserve = 1ULL << __popcnt64(rookMasks[i]);
			for (int j = 0; j != numToReserve; ++j)
			{
				rookAttacks[rookOffsets[i] + j] = 0;
				auto occ = _pdep_u64(j, rookMasks[i]);
				rookAttacks[rookOffsets[i] + j] |= dumbFill(setbit(i), occ, 1, 0);
				rookAttacks[rookOffsets[i] + j] |= dumbFill(setbit(i), occ, -1, 0);
				rookAttacks[rookOffsets[i] + j] |= dumbFill(setbit(i), occ, 0, 1);
				rookAttacks[rookOffsets[i] + j] |= dumbFill(setbit(i), occ, 0, -1);
			}
			offsetRook += numToReserve;
		}
	}

	void initPawnAttacks()
	{
		for (std::size_t i = 0; i != 64; ++i)
		{
			wpawnAttacks[i] = (setbit(i) << 9) | (setbit(i) << 7);
			bpawnAttacks[i] = (setbit(i) >> 9) | (setbit(i) >> 7);

			if (board::isLeftEdge(i))
			{
				wpawnAttacks[i] = setbit(i) << 9;
				bpawnAttacks[i] = setbit(i) >> 7;
			}

			if (board::isRightEdge(i))
			{
				wpawnAttacks[i] = setbit(i) << 7;
				bpawnAttacks[i] = setbit(i) >> 9;
			}
			
			if (board::isBottomEdge(i))
			{
				bpawnAttacks[i] = 0;
			}
			if (board::isTopEdge(i))
			{
				wpawnAttacks[i] = 0;
			}
		}
	}

	void initKingAttacks()
	{
		for (std::size_t i = 0; i != 64; ++i)
		{
			switch (i)
			{
			case board::square::a1:
				kingAttacks[i] = setbit(board::square::a2) | setbit(board::square::b1) | setbit(board::square::b2);
				break;
			case board::square::h1:
				kingAttacks[i] = setbit(board::square::h2) | setbit(board::square::g1) | setbit(board::square::g2);
				break;
			case board::square::a8:
				kingAttacks[i] = setbit(board::square::a7) | setbit(board::square::b8) | setbit(board::square::b7);
				break;
			case board::square::h8:
				kingAttacks[i] = setbit(board::square::h7) | setbit(board::square::g8) | setbit(board::square::g7);
				break;
			default:
				auto k = setbit(i);
				if (board::isInterior(i))
					kingAttacks[i] = k << 1 | k >> 1 | k << 8 | k >> 8 | k >> 9 | k << 9 | k << 7 | k >> 7;
				if (board::isBottomEdge(i))
					kingAttacks[i] = k << 1 | k >> 1 | k << 8 | k << 9 | k << 7;
				if (board::isTopEdge(i))
					kingAttacks[i] = k << 1 | k >> 1 | k >> 8 | k >> 9 | k >> 7;
				if (board::isLeftEdge(i))
					kingAttacks[i] = k << 1 | k << 8 | k >> 8 | k << 9 | k >> 7;
				if (board::isRightEdge(i))
					kingAttacks[i] = k >> 1 | k << 8 | k >> 8 | k >> 9 | k << 7;
				break;
			}
		}
	}

	void initKnightAttacks()
	{
		auto checkJump = [](std::size_t i, int r, int f) {
			auto r_i = aux::rank(i);
			auto f_i = aux::file(i);

			return (r_i + r <= 7) && (r_i + r >= 0) && (f_i + f <= 7) && (f_i + f >= 0);
		};

		for (std::size_t i = 0; i != 64; ++i)
		{
			const board::Bitboard loc = setbit(i);

			if (checkJump(i, 2, 1)) knightAttacks[i] |= loc << 17;
			if (checkJump(i, 2, -1)) knightAttacks[i] |= loc << 15;
			if (checkJump(i, 1, 2)) knightAttacks[i] |= loc << 10;
			if (checkJump(i, 1, -2)) knightAttacks[i] |= loc << 6;
			if (checkJump(i, -2, 1)) knightAttacks[i] |= loc >> 15;
			if (checkJump(i, -2, -1)) knightAttacks[i] |= loc >> 17;
			if (checkJump(i, -1, 2)) knightAttacks[i] |= loc >> 6;
			if (checkJump(i, -1, -2)) knightAttacks[i] |= loc >> 10;
		}
	}

	void initAttacks()
	{
		initRookBishopAttacks();
		initPawnAttacks();
		initKingAttacks();
		initKnightAttacks();
	}


}