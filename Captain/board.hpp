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
	using BoardFlags = std::uint16_t;


	/*Lowest 6 bits: index of from square
	  Next 6 bits: index of to square
	  Next 6: half move count
	  Next 4: castling flags
	  Next 4: flags for en passant
	  Next 5: move type flag
	  using 31 bits total*/
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
	constexpr std::uint32_t getMoveInfo<halfMoveCntMask>(Move m)
	{
		return (m & halfMoveCntMask) >> 12;
	}
	template<>
	constexpr std::uint32_t getMoveInfo<enPFileMask>(Move m)
	{
		return (m & enPFileMask) >> 22;
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

	constexpr std::uint32_t getCapType(pieceType p)
	{
		switch (p)
		{
		case pawns:
			return capP;
		case knights:
			return capN;
		case bishops:
			return capB;
		case rooks:
			return capR;
		case queens:
			return capQ;
		case king:
		case none:
		default:
			return QMove;
		}
	}

	constexpr unsigned int getPromoPiece(Move m)
	{
		switch (getMoveInfo<moveTypeMask>(m))
		{
		case queenPromoCapQ:
		case queenPromoCapR:
		case queenPromoCapB:
		case queenPromoCapN:
		case queenPromo:
			return queens;
		case rookPromoCapQ:
		case rookPromoCapR:
		case rookPromoCapB:
		case rookPromoCapN:
		case rookPromo:
			return rooks;
		case bishopPromoCapQ:
		case bishopPromoCapR:
		case bishopPromoCapB:
		case bishopPromoCapN:
		case bishopPromo:
			return bishops;
		case knightPromoCapQ:
		case knightPromoCapR:
		case knightPromoCapB:
		case knightPromoCapN:
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
		case queenPromoCapQ:
		case queenPromoCapR:
		case queenPromoCapB:
		case queenPromoCapN:
		case queenPromo:
			return 'q';
		case rookPromoCapQ:
		case rookPromoCapR:
		case rookPromoCapB:
		case rookPromoCapN:
		case rookPromo:
			return 'r';
		case bishopPromoCapQ:
		case bishopPromoCapR:
		case bishopPromoCapB:
		case bishopPromoCapN:
		case bishopPromo:
			return 'b';
		case knightPromoCapQ:
		case knightPromoCapR:
		case knightPromoCapB:
		case knightPromoCapN:
		case knightPromo:
			return 'n';
		default:
			// to notify GUI of any bug
			return 'X';
		}
	}

	constexpr pieceType moveType2capPieceType(board::Move m)
	{
		switch (board::getMoveInfo<constants::moveTypeMask>(m))
		{
		case enPCap:
		case capP:
			return pawns;
		case knightPromoCapN:
		case bishopPromoCapN:
		case rookPromoCapN:
		case queenPromoCapN:
		case capN:
			return knights;
		case knightPromoCapB:
		case bishopPromoCapB:
		case rookPromoCapB:
		case queenPromoCapB:
		case capB:
			return bishops;
		case knightPromoCapR:
		case bishopPromoCapR:
		case rookPromoCapR:
		case queenPromoCapR:
		case capR:
			return rooks;
		case knightPromoCapQ:
		case bishopPromoCapQ:
		case rookPromoCapQ:
		case queenPromoCapQ:
		case capQ:
			return queens;
		default:
			return none;
		}
	}

	constexpr std::uint32_t getPromoType(pieceType promoPiece, pieceType capType)
	{
		if (promoPiece == queens && capType == none) return queenPromo;
		if (promoPiece == queens && capType == knights) return queenPromoCapN;
		if (promoPiece == queens && capType == bishops) return queenPromoCapB;
		if (promoPiece == queens && capType == rooks) return queenPromoCapR;
		if (promoPiece == queens && capType == queens) return queenPromoCapQ;
		if (promoPiece == rooks && capType == none) return rookPromo;
		if (promoPiece == rooks && capType == knights) return rookPromoCapN;
		if (promoPiece == rooks && capType == bishops) return rookPromoCapB;
		if (promoPiece == rooks && capType == rooks) return rookPromoCapR;
		if (promoPiece == rooks && capType == queens) return rookPromoCapQ;
		if (promoPiece == bishops && capType == none) return bishopPromo;
		if (promoPiece == bishops && capType == knights) return bishopPromoCapN;
		if (promoPiece == bishops && capType == bishops) return bishopPromoCapB;
		if (promoPiece == bishops && capType == rooks) return bishopPromoCapR;
		if (promoPiece == bishops && capType == queens) return bishopPromoCapQ;
		if (promoPiece == knights && capType == none) return knightPromo;
		if (promoPiece == knights && capType == knights) return knightPromoCapN;
		if (promoPiece == knights && capType == bishops) return knightPromoCapB;
		if (promoPiece == knights && capType == rooks) return knightPromoCapR;
		if (promoPiece == knights && capType == queens) return knightPromoCapQ;
		return QMove;
	}

	constexpr bool isCapture(Move m)
	{
		const auto x = getMoveInfo<moveTypeMask>(m);
		return (x >= enPCap && x <= capQ) || (x >= knightPromoCapN && x <= rookPromoCapB) || (x >= rookPromoCapR && x <= queenPromoCapQ);
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

	class Board
	{
	public:

		// bitboards by piece type
		Bitboard wPieces[6] = { 0, 0, 0, 0, 0, 0 };
		Bitboard bPieces[6] = { 0, 0, 0, 0, 0, 0 };

		// bitboards for all white and all black pieces only
		Bitboard wAll = 0;
		Bitboard bAll = 0;
		
		// bitboard for all pieces
		Bitboard all = 0;

		Bitboard epLoc = 0;

		/* Lowest 6 bits: 50 move rule counter
		   Next 4: castling status indicator (white king, white queen, black king, black queen)
		   */
		BoardFlags flags = 0;
		static constexpr std::uint16_t ply50FlagMask = 0b111111U;
		static constexpr std::uint16_t wkCastleFlagMask = 0b1000000U;
		static constexpr std::uint16_t wqCastleFlagMask = 0b10000000U;
		static constexpr std::uint16_t bkCastleFlagMask = 0b100000000U;
		static constexpr std::uint16_t bqCastleFlagMask = 0b1000000000U;

		std::size_t currMove = 0;
		bool wMoving = true;

		Board(std::string fen);
		Board() {};
	
	private:
		template <bool wToMove>
		void movePiece(const std::uint64_t fromBit, const std::uint64_t toBit, unsigned int pieceType, unsigned int toType)
		{
			all ^= fromBit;
			all |= toBit;
			if constexpr (wToMove)
			{
				wAll ^= (fromBit ^ toBit);
				bAll &= ~toBit;
				wPieces[pieceType] &= ~fromBit;
				wPieces[pieceType] ^= toBit;
				if (toType != none) bPieces[toType] &= ~toBit;
			}
			else
			{
				bAll ^= (fromBit ^ toBit);
				wAll &= ~toBit;
				bPieces[pieceType] &= ~fromBit;
				bPieces[pieceType] ^= toBit;
				if (toType != none) wPieces[toType] &= ~toBit;
			}
		}

		// used in unmake move to move a piece from toBit to fromBit
		template <bool wToMove>
		constexpr void moveBack(std::uint64_t fromBit, std::uint64_t toBit, unsigned int pieceType)
		{
			all |= fromBit;
			all &= ~toBit;
			if constexpr (wToMove)
			{
				wAll ^= (fromBit ^ toBit);
				wPieces[pieceType] ^= (fromBit ^ toBit);
			}
			else
			{
				bAll ^= (fromBit ^ toBit);
				bPieces[pieceType] ^= (fromBit ^ toBit);
			}
		}

		template<bool w>
		constexpr void restorePiece(Bitboard at, pieceType type)
		{
			all ^= at;
			if constexpr (w)
			{
				wAll ^= at;
				wPieces[type] ^= at;
			}
			else
			{
				bAll ^= at;
				bPieces[type] ^= at;
			}
		}

		void clearPosition(std::uint64_t pos) noexcept;

	public:
		constexpr Move getHeading() const
		{
			board::Move heading = 0;

			heading |= ((std::uint32_t)flags) << constants::flagsOffset;

			if (epLoc)
			{
				heading |= constants::enPExistsMask;
				unsigned long index;
				_BitScanForward64(&index, epLoc);
				heading |= aux::file(index) << constants::enPMaskOffset;
			}
			return heading;
		}

		pieceType getPieceType(Bitboard at) const noexcept;

		void playMoves(const std::vector<Move>& v);
		
		template <bool wToMove>
		constexpr bool canCastleK() const
		{
			if constexpr (wToMove) return flags & wkCastleFlagMask;
			else return flags & bkCastleFlagMask;
		}

		template <bool wToMove>
		constexpr bool canCastleQ() const
		{
			if constexpr (wToMove) return flags & wqCastleFlagMask;
			else return flags & bqCastleFlagMask;
		}

		friend std::ostream& operator<<(std::ostream& os, const Board& b);

		template<bool wToMove, bool nullMove = false>
		void makeMove(const Move m)
		{
			if constexpr (nullMove)
			{
				epLoc = 0;
				wMoving = !wMoving;
				if constexpr (!wToMove)
					++currMove;
				return;
			}
			else { // the rest of the function
			const auto fromBit = setbit(getMoveInfo<fromMask>(m));
			const auto toBit = setbit(getMoveInfo<toMask>(m));
			const bool pawnMove = ((wPieces[pawns] | bPieces[pawns]) & fromBit) != 0;

			epLoc = 0;

			if constexpr (wToMove)
			{
				if (fromBit == setbit(wk_start) || fromBit == setbit(wkr_start)) flags &= ~wkCastleFlagMask;
				if (fromBit == setbit(wk_start) || fromBit == setbit(wqr_start)) flags &= ~wqCastleFlagMask;
				if (toBit == setbit(bk_start) || toBit == setbit(bkr_start)) flags &= ~bkCastleFlagMask;
				if (toBit == setbit(bk_start) || toBit == setbit(bqr_start)) flags &= ~bqCastleFlagMask;
			}
			else
			{
				if (toBit == setbit(wk_start) || toBit == setbit(wkr_start)) flags &= ~wkCastleFlagMask;
				if (toBit == setbit(wk_start) || toBit == setbit(wqr_start)) flags &= ~wqCastleFlagMask;
				if (fromBit == setbit(bk_start) || fromBit == setbit(bkr_start)) flags &= ~bkCastleFlagMask;
				if (fromBit == setbit(bk_start) || fromBit == setbit(bqr_start)) flags &= ~bqCastleFlagMask;
			}

			movePiece<wToMove>(fromBit, toBit, getPieceType(fromBit), getPieceType(toBit));

			const auto moveType = getMoveInfo<moveTypeMask>(m);
			switch (moveType)
			{
			case dblPawnMove:
				if constexpr (wToMove)
					epLoc = toBit >> 8;
				else
					epLoc = toBit << 8;
				break;
			case QSCastle:
				if constexpr (wToMove)
					movePiece<wToMove>(setbit(wqr_start), setbit(wqr_start + 3), rooks, none);
				else
					movePiece<wToMove>(setbit(bqr_start), setbit(bqr_start + 3), rooks, none);
				break;
			case KSCastle:
				if constexpr (wToMove)
					movePiece<wToMove>(setbit(wkr_start), setbit(wkr_start - 2), rooks, none);
				else
					movePiece<wToMove>(setbit(bkr_start), setbit(bkr_start - 2), rooks, none);
				break;
			case enPCap:
				if constexpr (wToMove)
					clearPosition(toBit >> 8);
				else
					clearPosition(toBit << 8);
				break;
			}

			if (getPromoPiece(m) != king) // the moveType is a promo
			{
				if constexpr (wToMove)
				{
					wPieces[pawns] ^= toBit;
					wPieces[getPromoPiece(m)] ^= toBit;
				}
				else
				{
					bPieces[pawns] ^= toBit;
					bPieces[getPromoPiece(m)] ^= toBit;
				}
			}

			if (isCapture(m) || pawnMove)
				flags &= (std::uint16_t)0b1111000000U;
			else
				flags += 1;

			if constexpr (!wToMove)
				++currMove;
			wMoving = !wMoving;
			} // for the else block when if constexpr (nullMove) is false
		}

		template <bool wToMove, bool nullMove = false> // wToMove means we are unmaking a move played by w
		void unmakeMove(const Move m)
		{
			if constexpr (nullMove)
			{
				if constexpr (wToMove)
				{
					if (getMoveInfo<enPExistsMask>(m)) epLoc = setbit(5U, getMoveInfo<enPFileMask>(m));
					else epLoc = 0;
				}
				else
				{
					if (getMoveInfo<enPExistsMask>(m)) epLoc = setbit(2U, getMoveInfo<enPFileMask>(m));
					else epLoc = 0;
				}
				wMoving = !wMoving;
				if constexpr (!wToMove)
					--currMove;
				return;
			}
			else { 
			const auto fromBit = setbit(getMoveInfo<fromMask>(m));
			const auto toBit = setbit(getMoveInfo<toMask>(m));

			const auto moveType = getMoveInfo<moveTypeMask>(m);
			// change the promoted piece back into a pawn
			// before handling anything else, to ensure moveBack
			// works correctly
			if (moveType >= knightPromo)
			{
				if constexpr (wToMove)
				{
					wPieces[pawns] ^= toBit;
					wPieces[getPromoPiece(m)] ^= toBit;
				}
				else
				{
					bPieces[pawns] ^= toBit;
					bPieces[getPromoPiece(m)] ^= toBit;
				}
			}

			moveBack<wToMove>(fromBit, toBit, getPieceType(toBit));

			switch (moveType)
			{
			case QSCastle:
				if constexpr (wToMove)
					moveBack<wToMove>(setbit(wqr_start), setbit(wqr_start + 3), rooks);
				else
					moveBack<wToMove>(setbit(bqr_start), setbit(bqr_start + 3), rooks);
				break;
			case KSCastle:
				if constexpr (wToMove)
					moveBack<wToMove>(setbit(wkr_start), setbit(wkr_start - 2), rooks);
				else
					moveBack<wToMove>(setbit(bkr_start), setbit(bkr_start - 2), rooks);
				break;
			case enPCap:
				if constexpr (wToMove)
				{
					bPieces[pawns] |= (toBit >> 8);
					bAll |= (toBit >> 8);
					all |= (toBit >> 8);
				}
				else
				{
					wPieces[pawns] |= (toBit << 8);
					wAll |= (toBit << 8);
					all |= (toBit << 8);
				}
				break;
			case capP:
				restorePiece<!wToMove>(toBit, pawns);
				break;
			case knightPromoCapN:
			case bishopPromoCapN:
			case rookPromoCapN:
			case queenPromoCapN:
			case capN:
				restorePiece<!wToMove>(toBit, knights);
				break;
			case knightPromoCapB:
			case bishopPromoCapB:
			case rookPromoCapB:
			case queenPromoCapB:
			case capB:
				restorePiece<!wToMove>(toBit, bishops);
				break;
			case knightPromoCapR:
			case bishopPromoCapR:
			case rookPromoCapR:
			case queenPromoCapR:
			case capR:
				restorePiece<!wToMove>(toBit, rooks);
				break;
			case knightPromoCapQ:
			case bishopPromoCapQ:
			case rookPromoCapQ:
			case queenPromoCapQ:
			case capQ:
				restorePiece<!wToMove>(toBit, queens);
				break;
			}

			flags = static_cast<std::uint16_t>(getMoveInfo<halfMoveCntMask>(m) | (getMoveInfo<castleFlagMask>(m) >> flagsOffset));
			if constexpr (wToMove)
			{
				if (getMoveInfo<enPExistsMask>(m)) epLoc = setbit(5U, getMoveInfo<enPFileMask>(m));
				else epLoc = 0;
			}
			else
			{
				if (getMoveInfo<enPExistsMask>(m)) epLoc = setbit(2U, getMoveInfo<enPFileMask>(m));
				else epLoc = 0;
			}
			if constexpr (!wToMove)
				--currMove;
			wMoving = !wMoving;
			} // for the else block when if constexpr (nullMove) is false
		}
	};
	bool operator==(const Board& l, const Board& r) noexcept;
#include <xmmintrin.h>
	struct QBB
	{
		__m256i qbb;
		// epc contains the en passant location and castling rights
		// castling rights are encoded by setting the kings' and rooks'
		// starting square's bit if and only if the respective piece
		// has never moved
		std::uint64_t epc = 0;
		QBB(const Board&);
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
		using QBBDelta = std::array<std::uint64_t, 4>;
		
		static constexpr std::array<QBBDelta, 64*6*8> genMakeMoveArray()
		{
			constexpr auto index = [](std::size_t sq, std::size_t pt, std::size_t mt) -> std::size_t {
				return 64 * sq + 6 * pt + mt;
			};


			std::array<QBBDelta, 64 * 6 * 8> val = {};

			for (std::size_t i = 0; i != 64; ++i)
				for (std::size_t j = 0; j != 6; ++j)
					for (std::size_t k = 0; k != 8; ++k)
					{
						val[index(i, j, k)][0] = ;
						val[index(i, j, k)][1] = ;
						val[index(i, j, k)][2] = ;
						val[index(i, j, k)][3] = ;
					}
			return val;
		}

		static constexpr std::array<QBBDelta, 64 * 6 * 8> moveDelta = genMakeMoveArray();
	};
}
#endif