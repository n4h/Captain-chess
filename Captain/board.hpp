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

#ifndef BOARD_H
#define BOARD_H

#include <string>
#include <cstdint>
#include <concepts>
#include <tuple>
#include <vector>
#include <array>
#include <climits>

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
	using Move = std::uint16_t;

	constexpr void addMoveInfo(Move& m, std::unsigned_integral auto info)
	{
		static_assert(sizeof(info) * CHAR_BIT > 16);
		m |= (info << 16);
	}

	template<std::uint32_t mask>
	constexpr std::uint32_t getMoveInfo(Move m)
	{
		return m & mask;
	}
	template<>
	constexpr std::uint32_t getMoveInfo<toMask>(Move m)
	{
		return (m & toMask) >> toMaskOffset;
	}
	template<>
	constexpr std::uint32_t getMoveInfo<moveTypeMask>(Move m)
	{
		return (m & moveTypeMask) >> moveTypeOffset;
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

	constexpr bool isPromo(Move m)
	{
		return getMoveInfo<moveTypeMask>(m) >= knightPromo;
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
			return none;
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
			// to stand out in logs
			return 'X';
		}
	}

	constexpr std::uint32_t getPromoType(pieceType promoPiece)
	{
		switch (promoPiece)
		{
		case queens:
			return queenPromo;
		case rooks:
			return rookPromo;
		case bishops:
			return bishopPromo;
		case knights:
			return knightPromo;
		default:
			return QMove;
		}
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

	constexpr square castSq(std::unsigned_integral auto x)
	{
		return static_cast<square>(x);
	}

	constexpr square getMoveFromSq(Move m)
	{
		return static_cast<square>(getMoveInfo<fromMask>(m));
	}

	constexpr square getMoveToSq(Move m)
	{
		return static_cast<square>(getMoveInfo<toMask>(m));
	}

	namespace masks {
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
	}

	constexpr Bitboard fileMask(square s)
	{
		Bitboard mask = setbit(s);
		mask |= mask >> 8;
		mask |= mask << 8;
		mask |= mask >> 16;
		mask |= mask << 16;
		mask |= mask >> 32;
		mask |= mask << 32;
		return mask;
	}

	constexpr Bitboard multiFileMask(Bitboard b)
	{
		b |= b >> 8;
		b |= b << 8;
		b |= b >> 16;
		b |= b << 16;
		b |= b >> 32;
		b |= b << 32;
		return b;
	}

	constexpr Bitboard rankMask(square s)
	{
		Bitboard mask = setbit(s);
		mask |= (mask >> 1) & ~0x8080808080808080U;
		mask |= (mask << 1) & ~0x101010101010101U;
		mask |= (mask >> 2) & ~0xC0C0C0C0C0C0C0C0U;
		mask |= (mask << 2) & ~0x303030303030303U;
		mask |= (mask >> 4) & ~0xF0F0F0F0F0F0F0F0U;
		mask |= (mask << 4) & ~0xF0F0F0F0F0F0F0FU;
		return mask;
	}

	constexpr Bitboard multiRankMask(Bitboard b)
	{
		b |= (b >> 1) & ~0x8080808080808080U;
		b |= (b << 1) & ~0x101010101010101U;
		b |= (b >> 2) & ~0xC0C0C0C0C0C0C0C0U;
		b |= (b << 2) & ~0x303030303030303U;
		b |= (b >> 4) & ~0xF0F0F0F0F0F0F0F0U;
		b |= (b << 4) & ~0xF0F0F0F0F0F0F0FU;
		return b;
	}

	constexpr Bitboard diagMask(square s)
	{
		Bitboard mask = setbit(s);
		mask |= (mask >> 7) & ~0x101010101010101U;
		mask |= (mask << 7) & ~0x8080808080808080U;
		mask |= (mask << 14) & ~0xC0C0C0C0C0C0C0C0U;
		mask |= (mask >> 14) & ~0x303030303030303U;
		mask |= (mask << 28) & ~0xF0F0F0F0F0F0F0F0U;
		mask |= (mask >> 28) & ~0xF0F0F0F0F0F0F0FU;
		return mask;
	}

	constexpr Bitboard multiDiagMask(Bitboard b)
	{
		b |= (b << 7) & ~0x8080808080808080U;
		b |= (b >> 7) & ~0x101010101010101U;
		b |= (b << 14) & ~0xC0C0C0C0C0C0C0C0U;
		b |= (b >> 14) & ~0x303030303030303U;
		b |= (b << 28) & ~0xF0F0F0F0F0F0F0F0U;
		b |= (b >> 28) & ~0xF0F0F0F0F0F0F0FU;
		return b;
	}

	constexpr Bitboard antiDiagMask(square s)
	{
		Bitboard mask = setbit(s);
		mask |= (mask >> 9) & ~0x8080808080808080U;
		mask |= (mask << 9) & ~0x101010101010101U;
		mask |= (mask >> 18) & ~0xC0C0C0C0C0C0C0C0U;
		mask |= (mask << 18) & ~0x303030303030303U;
		mask |= (mask >> 36) & ~0xF0F0F0F0F0F0F0F0U;
		mask |= (mask << 36) & ~0xF0F0F0F0F0F0F0FU;
		return mask;
	}

	constexpr Bitboard multiAntiDiagMask(Bitboard b)
	{
		b |= (b >> 9) & ~0x8080808080808080U;
		b |= (b << 9) & ~0x101010101010101U;
		b |= (b >> 18) & ~0xC0C0C0C0C0C0C0C0U;
		b |= (b << 18) & ~0x303030303030303U;
		b |= (b >> 36) & ~0xF0F0F0F0F0F0F0F0U;
		b |= (b << 36) & ~0xF0F0F0F0F0F0F0FU;
		return b;
	}

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

	enum class Color{Black = -1, White = 1};
	constexpr Color operator*(board::Color c, int)
	{
		return static_cast<Color>(static_cast<int>(c) * -1);
	}

#include <xmmintrin.h>
	using QBBDelta = std::array<std::uint64_t, 4>;
	struct QBB
	{
		std::uint64_t side = 0;
		std::uint64_t pbq = 0;
		std::uint64_t nbk = 0;
		std::uint64_t rqk = 0;
		// epc contains the en passant location and castling rights
		// castling rights are encoded by setting the kings' and rooks'
		// starting square's bit if and only if the respective piece
		// has never moved
		std::uint64_t epc = 0;
		QBB() {}
		QBB(const std::string&);

		void makeMove(const Move);

		void doNullMove();

		unsigned getPieceType(square) const noexcept;

		void flipQBB();

		constexpr bool isCapture(Move m) const noexcept
		{
			auto movetype = getMoveInfo<moveTypeMask>(m);
			auto toBB = setbit(getMoveToSq(m));
			toBB &= getOccupancy();
			return toBB || movetype == enPCap;
		}

		constexpr bool canCastleShort() const noexcept
		{
			return (epc & 0xF0U) == 0x90U;
		}

		constexpr bool canCastleLong() const noexcept
		{
			return (epc & 0x1FU) == 0x11U;
		}

		constexpr bool oppCanCastleShort() const noexcept
		{
			return (epc & 0xF000000000000000ULL) == 0x9000000000000000ULL;
		}

		constexpr bool oppCanCastleLong() const noexcept
		{
			return (epc & 0x1F00000000000000ULL) == 0x1100000000000000ULL;
		}

		constexpr bool enpExists() const noexcept
		{
			return epc & rankMask(a6);
		}

		unsigned getEnpFile() const noexcept;

		constexpr Bitboard getOccupancy() const noexcept
		{
			return rqk | nbk | pbq;
		}

		constexpr Bitboard getPawns() const noexcept
		{
			return pbq & ~nbk & ~rqk;
		}

		constexpr Bitboard getKnights() const noexcept
		{
			return nbk & ~pbq & ~rqk;
		}

		constexpr Bitboard getBishops() const noexcept
		{
			return nbk & pbq;
		}

		constexpr Bitboard getRooks() const noexcept
		{
			return rqk & ~pbq & ~nbk;
		}

		constexpr Bitboard getQueens() const noexcept
		{
			return pbq & rqk;
		}

		constexpr Bitboard getKings() const noexcept
		{
			return rqk & nbk;
		}

		constexpr Bitboard my(Bitboard b) const noexcept
		{
			return b & side;
		}

		constexpr Bitboard their(Bitboard b) const noexcept
		{
			return b & ~side;
		}

		constexpr Bitboard getDiagSliders() const noexcept
		{
			return pbq & ~getPawns();
		}

		constexpr Bitboard getOrthSliders() const noexcept
		{
			return rqk & ~getKings();
		}

		constexpr Bitboard getEp() const noexcept
		{
			return epc & rankMask(a6);
		}

		constexpr auto get50() const noexcept
		{
			return _bextr_u64(epc, 24, 6);
		}

		constexpr Bitboard getCastling() const noexcept
		{
			return epc & (rankMask(a1) | rankMask(a8));
		}

		constexpr bool isWhiteToPlay() const noexcept
		{
			return epc & 0x80'80'00'00'00U;
		}
		// TODO refactor program to use getColorToPlay where appropriate
		constexpr Color getColorToPlay() const noexcept
		{
			return epc & 0x80'80'00'00'00U ? Color::White : Color::Black;
		}
	};

	board::Bitboard getCastlingDiff(const board::QBB&, const board::QBB&);
}
#endif