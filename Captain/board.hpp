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

#ifndef BOARD_H
#define BOARD_H

#include <string>
#include <cstdint>
#include <concepts>
#include <tuple>
#include <iostream>
#include <cassert>
#include <vector>
#include <array>

#include "constants.hpp"
#include "auxiliary.hpp"

namespace board
{
	using namespace aux;
	using namespace constants;

	using std::unsigned_integral;

	using Bitboard = std::uint64_t;

	/*Lowest 6 bits: index of from square
	  Next 6 bits: index of to square
	  Last 4: move type (8 possible move types)
	*/
	using Move = std::uint32_t;

	template<std::uint32_t mask>
	constexpr std::uint32_t getMoveInfo(Move m)
	{
		return m & mask;
	}
	template<>
	constexpr std::uint32_t getMoveInfo<toMask>(Move m)
	{
		return (m & toMask) >> 6;
	}
	template<>
	constexpr std::uint32_t getMoveInfo<moveTypeMask>(Move m)
	{
		return (m & moveTypeMask) >> 12;
	}

	enum pieceType : unsigned int
	{
		pawns, knights, bishops, rooks, queens, king, none
	};

	constexpr pieceType char2pieceType(const char i) noexcept
	{
		switch (i)
		{
		case 'K':
		case 'k':
			return king;
		case 'Q':
		case 'q':
			return queens;
		case 'R':
		case 'r':
			return rooks;
		case 'B':
		case 'b':
			return bishops;
		case 'N':
		case 'n':
			return knights;
		case 'P':
		case 'p':
			return pawns;
		default:
			return none;
		}
	}

	constexpr unsigned int getPieceValue(pieceType p)
	{
		return pieceValues[p];
	}

	constexpr unsigned int getPromoPiece(Move m)
	{
		switch (getMoveInfo<moveTypeMask>(m))
		{
		case queenPromo:
			return queens;
		case rookPromo:
			return rooks;
		case bishopPromo:
			return bishops;
		case knightPromo:
			return knights;
		default:
			return king;
		}
	}

	constexpr char promoFlag2char(Move m)
	{
		switch (getMoveInfo<moveTypeMask>(m))
		{
		case queenPromo:
			return 'q';
		case rookPromo:
			return 'r';
		case bishopPromo:
			return 'b';
		case knightPromo:
			return 'n';
		default:
			// to notify GUI of any bug
			return 'X';
		}
	}

	constexpr std::uint32_t getPromoType(pieceType promoPiece)
	{
		// TODO make a switch statement
		if (promoPiece == queens) return queenPromo;
		if (promoPiece == rooks) return rookPromo;
		if (promoPiece == bishops) return bishopPromo;
		if (promoPiece == knights) return knightPromo;
		return QMove;
	}

	std::tuple<bool, unsigned int> makeSquare(const char i);

	enum square : unsigned int
	{
		a1, b1, c1, d1, e1, f1, g1, h1,
		a2, b2, c2, d2, e2, f2, g2, h2,
		a3, b3, c3, d3, e3, f3, g3, h3,
		a4, b4, c4, d4, e4, f4, g4, h4,
		a5, b5, c5, d5, e5, f5, g5, h5,
		a6, b6, c6, d6, e6, f6, g6, h6,
		a7, b7, c7, d7, e7, f7, g7, h7,
		a8, b8, c8, d8, e8, f8, g8, h8,
	};

	constexpr Bitboard wkCastleSquares = setbit(f1) | setbit(g1);
	constexpr Bitboard wqCastleSquares = setbit(b1) | setbit(c1) | setbit(d1);
	constexpr Bitboard bkCastleSquares = setbit(f8) | setbit(g8);
	constexpr Bitboard bqCastleSquares = setbit(b8) | setbit(c8) | setbit(d8);

	constexpr Bitboard wkCastleChecks = setbit(e1) | setbit(f1) | setbit(g1);
	constexpr Bitboard wqCastleChecks = setbit(c1) | setbit(d1) | setbit(e1);
	constexpr Bitboard bkCastleChecks = setbit(e8) | setbit(f8) | setbit(g8);
	constexpr Bitboard bqCastleChecks = setbit(c8) | setbit(d8) | setbit(e8);

	constexpr Bitboard fastCastleWK = setbit(wkr_start) | setbit(wk_start) | setbit(g1) | setbit(f1);
	constexpr Bitboard fastCastleBK = setbit(bkr_start) | setbit(bk_start) | setbit(g8) | setbit(f8);
	constexpr Bitboard fastCastleWQ = setbit(wqr_start) | setbit(wk_start) | setbit(c1) | setbit(d1);
	constexpr Bitboard fastCastleBQ = setbit(bqr_start) | setbit(bk_start) | setbit(c8) | setbit(d8);

	constexpr Bitboard rankMask[8] =
	{
		setbit(a1) | setbit(b1) | setbit(c1) | setbit(d1) | setbit(e1) | setbit(f1) | setbit(g1) | setbit(h1),
		setbit(a2) | setbit(b2) | setbit(c2) | setbit(d2) | setbit(e2) | setbit(f2) | setbit(g2) | setbit(h2),
		setbit(a3) | setbit(b3) | setbit(c3) | setbit(d3) | setbit(e3) | setbit(f3) | setbit(g3) | setbit(h3),
		setbit(a4) | setbit(b4) | setbit(c4) | setbit(d4) | setbit(e4) | setbit(f4) | setbit(g4) | setbit(h4),
		setbit(a5) | setbit(b5) | setbit(c5) | setbit(d5) | setbit(e5) | setbit(f5) | setbit(g5) | setbit(h5),
		setbit(a6) | setbit(b6) | setbit(c6) | setbit(d6) | setbit(e6) | setbit(f6) | setbit(g6) | setbit(h6),
		setbit(a7) | setbit(b7) | setbit(c7) | setbit(d7) | setbit(e7) | setbit(f7) | setbit(g7) | setbit(h7),
		setbit(a8) | setbit(b8) | setbit(c8) | setbit(d8) | setbit(e8) | setbit(f8) | setbit(g8) | setbit(h8)
	};

	constexpr Bitboard fileMask[8] =
	{
		setbit(a1) | setbit(a2) | setbit(a3) | setbit(a4) | setbit(a5) | setbit(a6) | setbit(a7) | setbit(a8),
		setbit(b1) | setbit(b2) | setbit(b3) | setbit(b4) | setbit(b5) | setbit(b6) | setbit(b7) | setbit(b8),
		setbit(c1) | setbit(c2) | setbit(c3) | setbit(c4) | setbit(c5) | setbit(c6) | setbit(c7) | setbit(c8),
		setbit(d1) | setbit(d2) | setbit(d3) | setbit(d4) | setbit(d5) | setbit(d6) | setbit(d7) | setbit(d8),
		setbit(e1) | setbit(e2) | setbit(e3) | setbit(e4) | setbit(e5) | setbit(e6) | setbit(e7) | setbit(e8),
		setbit(f1) | setbit(f2) | setbit(f3) | setbit(f4) | setbit(f5) | setbit(f6) | setbit(f7) | setbit(f8),
		setbit(g1) | setbit(g2) | setbit(g3) | setbit(g4) | setbit(g5) | setbit(g6) | setbit(g7) | setbit(g8),
		setbit(h1) | setbit(h2) | setbit(h3) | setbit(h4) | setbit(h5) | setbit(h6) | setbit(h7) | setbit(h8)
	};

	constexpr Bitboard diagMask[15] =
	{
		setbit(a1),
		setbit(a2) | setbit(b1),
		setbit(a3) | setbit(b2) | setbit(c1),
		setbit(a4) | setbit(b3) | setbit(c2) | setbit(d1),
		setbit(a5) | setbit(b4) | setbit(c3) | setbit(d2) | setbit(e1),
		setbit(a6) | setbit(b5) | setbit(c4) | setbit(d3) | setbit(e2) | setbit(f1),
		setbit(a7) | setbit(b6) | setbit(c5) | setbit(d4) | setbit(e3) | setbit(f2) | setbit(g1),
		setbit(a8) | setbit(b7) | setbit(c6) | setbit(d5) | setbit(e4) | setbit(f3) | setbit(g2) | setbit(h1),
		setbit(b8) | setbit(c7) | setbit(d6) | setbit(e5) | setbit(f4) | setbit(g3) | setbit(h2),
		setbit(c8) | setbit(d7) | setbit(e6) | setbit(f5) | setbit(g4) | setbit(h3),
		setbit(d8) | setbit(e7) | setbit(f6) | setbit(g5) | setbit(h4),
		setbit(e8) | setbit(f7) | setbit(g6) | setbit(h5),
		setbit(f8) | setbit(g7) | setbit(h6),
		setbit(g8) | setbit(h7),
		setbit(h8)
	};

	constexpr Bitboard antidiagMask[15] =
	{
		setbit(h1),
		setbit(g1) | setbit(h2),
		setbit(f1) | setbit(g2) | setbit(h3),
		setbit(e1) | setbit(f2) | setbit(g3) | setbit(h4),
		setbit(d1) | setbit(e2) | setbit(f3) | setbit(g4) | setbit(h5),
		setbit(c1) | setbit(d2) | setbit(e3) | setbit(f4) | setbit(g5) | setbit(h6),
		setbit(b1) | setbit(c2) | setbit(d3) | setbit(e4) | setbit(f5) | setbit(g6) | setbit(h7),
		setbit(a1) | setbit(b2) | setbit(c3) | setbit(d4) | setbit(e5) | setbit(f6) | setbit(g7) | setbit(h8),
		setbit(a2) | setbit(b3) | setbit(c4) | setbit(d5) | setbit(e6) | setbit(f7) | setbit(g8),
		setbit(a3) | setbit(b4) | setbit(c5) | setbit(d6) | setbit(e7) | setbit(f8),
		setbit(a4) | setbit(b5) | setbit(c6) | setbit(d7) | setbit(e8),
		setbit(a5) | setbit(b6) | setbit(c7) | setbit(d8),
		setbit(a6) | setbit(b7) | setbit(c8),
		setbit(a7) | setbit(b8),
		setbit(a8)
	};

	constexpr bool isRightEdge(unsigned_integral auto i)
	{
		return (i + 1) % 8 == 0;
	}

	constexpr bool isLeftEdge(unsigned_integral auto i)
	{
		return i % 8 == 0;
	}

	constexpr bool isBottomEdge(unsigned_integral auto i)
	{
		return i <= 7;
	}

	constexpr bool isTopEdge(unsigned_integral auto i)
	{
		return i >= 56 && i <= 63;
	}

	constexpr bool isEdge(unsigned_integral auto i)
	{
		return isLeftEdge(i) || isBottomEdge(i) || isTopEdge(i) || isRightEdge(i);
	}

	constexpr bool isInterior(unsigned_integral auto i)
	{
		return !isEdge(i);
	}

	constexpr bool isCorner(unsigned_integral auto i)
	{
		return i == a1 || i == h1 || i == a8 || i == h8;
	}
#include <xmmintrin.h>
	using QBBDelta = std::array<std::uint64_t, 4>;
	struct QBB
	{
		__m256i qbb;
		// epc contains the en passant location and castling rights
		// castling rights are encoded by setting the kings' and rooks'
		// starting square's bit if and only if the respective piece
		// has never moved
		std::uint64_t epc = 0;
		QBB(const std::string&);
		
		
		void makeMove(Move);
		void unmakeMove(Move);
		unsigned getPieceType(square) const;

		constexpr bool canCastleShort() const
		{
			return epc & 0x90U;
		}
		constexpr bool canCastleLong() const
		{
			return epc & 0x11U;
		}
	private:
		void flipQBB();

	};

	constexpr std::array<QBBDelta, 64 * 6 * 8> genMakeMoveArray()
	{
		constexpr auto index = [](std::size_t sq, std::uint32_t pt, std::uint32_t mt) -> std::size_t {
			return 64 * sq + 6 * pt + mt;
		};

		constexpr auto bb0 = [](std::size_t sq, std::uint32_t pt, std::uint32_t mt) -> std::uint64_t {
			std::uint64_t ret = setbit(sq);
			if (pt == kingCode && mt == KSCastle)
				ret |= 0xA0U;
			else if (pt == kingCode && mt == QSCastle)
				ret |= 0x9U;
			return ret;
		};

		constexpr auto bb1 = [](std::size_t sq, std::uint32_t pt, std::uint32_t mt) -> std::uint64_t {
			if (pt == pawnCode)
				if (mt == enPCap)
					return setbit(sq) | (setbit(sq) >> 8);
				else if (mt == queenPromo || mt == bishopPromo)
					return setbit(sq);
				else if (mt == knightPromo || mt == rookPromo)
					return 0;
				else
					return setbit(sq);
			else if (pt == queenCode || pt == bishopCode)
				return setbit(sq);
			else
				return 0U;
		};

		constexpr auto bb2 = [](std::size_t sq, std::uint32_t pt, std::uint32_t mt) -> std::uint64_t {
			if (pt == pawnCode)
				if (mt == knightPromo || mt == bishopPromo)
					return setbit(sq);
				else
					return 0U;
			else if (pt == knightCode || pt == bishopCode || pt == kingCode)
				return setbit(sq);
			else
				return 0;
		};

		constexpr auto bb3 = [](std::size_t sq, std::uint32_t pt, std::uint32_t mt) -> std::uint64_t {
			if (pt == pawnCode)
				if (mt == rookPromo || mt == queenPromo)
					return setbit(sq);
				else
					return 0U;
			else if (pt == kingCode)
				if (mt == KSCastle)
					return setbit(sq) | setbit(h1) | setbit(f1);
				else if (mt == QSCastle)
					return setbit(sq) | setbit(a1) | setbit(d1);
				else
					return setbit(sq);
			else if (pt == rookCode || pt == queenCode)
				return setbit(sq);
			else
				return 0;
		};

		std::array<QBBDelta, 64 * 6 * 8> val = {};

		for (std::size_t i = 0; i != 64; ++i)
			for (std::uint32_t j = 0; j != 6; ++j) // TODO iteration range
				for (std::uint32_t k = 0; k != 8; ++k)
				{
					val[index(i, j, k)][0] = bb0(i, j + 1, k);
					val[index(i, j, k)][1] = bb1(i, j + 1, k);
					val[index(i, j, k)][2] = bb2(i, j + 1, k);
					val[index(i, j, k)][3] = bb3(i, j + 1, k);
				}
		return val;
	}

	constexpr std::array<QBBDelta, 64 * 14> genSquareXPieceArray()
	{
		constexpr auto index = [](auto i, auto j) {return 64 * i + 14 * j; };
		std::array<QBBDelta, 64 * 14> val = {};
		for (std::size_t i = 0; i != 64; ++i)
			for (std::uint32_t j = 0; j != 14U; ++j)
			{
				bool bb0 = j & 0b1U;
				bool bb1 = j & 0b10U;
				bool bb2 = j & 0b100U;
				bool bb3 = j & 0b1000U;
				auto bit = setbit(i);
				val[index(i, j)][0] = bb0 ? bit : 0;
				val[index(i, j)][1] = bb1 ? bit : 0;
				val[index(i, j)][2] = bb2 ? bit : 0;
				val[index(i, j)][3] = bb3 ? bit : 0;
			}
		return val;
	}

	constexpr std::array<QBBDelta, 64 * 6 * 8> makeMoveDeltas = genMakeMoveArray();
	constexpr std::array<QBBDelta, 64 * 14> squareXpieceQBB = genSquareXPieceArray();
}
#endif