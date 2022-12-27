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

	using AttackMap = board::Bitboard;
	using board::Bitboard;

	// move generation is based on "Hyperbola Quintessence" algorithm
	// https://www.chessprogramming.org/Hyperbola_Quintessence


	template<typename T>
	constexpr AttackMap hypqDiag(Bitboard o, T idx)
	{
		if constexpr (std::is_same_v<T, board::square>)
		{
			o &= board::diagMask(idx);
			Bitboard r = aux::setbit(idx);
			Bitboard orev = _byteswap_uint64(o);
			Bitboard rrev = _byteswap_uint64(r);
			return board::diagMask(idx) & ((o - 2 * r) ^ _byteswap_uint64(orev - 2 * rrev));
		}
		else if constexpr (std::is_same_v<T, Bitboard>)
		{
			o &= board::multiDiagMask(idx);
			Bitboard orev = _byteswap_uint64(o);
			Bitboard rrev = _byteswap_uint64(idx);
			return board::multiDiagMask(idx) & ((o - 2 * idx) ^ _byteswap_uint64(orev - 2 * rrev));
		}
		else
		{
			return 0ULL;
		}
	}

	template<typename T>
	constexpr AttackMap hypqAntiDiag(Bitboard o, T idx)
	{
		if constexpr (std::is_same_v<T, board::square>)
		{
			o &= board::antiDiagMask(idx);
			Bitboard r = aux::setbit(idx);
			Bitboard orev = _byteswap_uint64(o);
			Bitboard rrev = _byteswap_uint64(r);
			return board::antiDiagMask(idx) & ((o - 2 * r) ^ _byteswap_uint64(orev - 2 * rrev));
		}
		else if constexpr (std::is_same_v<T, Bitboard>)
		{
			o &= board::multiAntiDiagMask(idx);
			Bitboard orev = _byteswap_uint64(o);
			Bitboard rrev = _byteswap_uint64(idx);
			return board::multiAntiDiagMask(idx) & ((o - 2 * idx) ^ _byteswap_uint64(orev - 2 * rrev));
		}
		else
		{
			return 0ULL;
		}
	}

	template<typename T>
	constexpr AttackMap hypqFile(Bitboard o, T idx)
	{
		if constexpr (std::is_same_v<T, board::square>)
		{
			o &= board::fileMask(idx);
			Bitboard r = aux::setbit(idx);
			Bitboard orev = _byteswap_uint64(o);
			Bitboard rrev = _byteswap_uint64(r);
			return board::fileMask(idx) & ((o - 2 * r) ^ _byteswap_uint64(orev - 2 * rrev));
		}
		else if constexpr (std::is_same_v<T, Bitboard>)
		{
			o &= board::multiFileMask(idx);
			Bitboard orev = _byteswap_uint64(o);
			Bitboard rrev = _byteswap_uint64(idx);
			return board::multiFileMask(idx) & ((o - 2 * idx) ^ _byteswap_uint64(orev - 2 * rrev));
		}
		else
		{
			return 0ULL;
		}
	}
	
	template<typename T>
	AttackMap hypqRank(Bitboard o, T idx)
	{
		// No bit reversal: map to file 0 and calculate file attacks
		// before converting back to rank attacks
		if constexpr (std::is_same_v<T, board::square>)
		{
			Bitboard vertical = _pext_u64(o, board::rankMask(idx));
			vertical = _pdep_u64(vertical, board::fileMask(board::a1));
			Bitboard attacks = hypqFile(vertical, static_cast<board::square>(8 * aux::file(idx)));
			attacks = _pext_u64(attacks, board::fileMask(board::a1));
			return _pdep_u64(attacks, board::rankMask(idx));
		}
		else if constexpr (std::is_same_v<T, Bitboard>)
		{
			Bitboard vertical = _pext_u64(o, board::multiRankMask(idx));
			vertical = _pdep_u64(vertical, board::fileMask(board::a1));
			Bitboard attacks = hypqFile(vertical, static_cast<board::square>(8 * aux::file(_tzcnt_u64(idx))));
			attacks = _pext_u64(attacks, board::fileMask(board::a1));
			return _pdep_u64(attacks, board::multiRankMask(idx));
		}
		else
		{
			return 0ULL;
		}
	}

	template<typename T>
	constexpr AttackMap kingAttacks(T idx)
	{
		Bitboard king;
		if constexpr (std::is_same_v<T, board::square>)
		{
			king = aux::setbit(idx);
		}
		else if constexpr (std::is_same_v<T, Bitboard>)
		{
			king = idx;
		}
		else
		{
			return 0ULL;
		}

		Bitboard n = (king << 8);
		Bitboard s = (king >> 8);
		Bitboard w = (king >> 1) & ~board::fileMask(board::h1);
		Bitboard e = (king << 1) & ~board::fileMask(board::a1);

		Bitboard nw = (king << 7) & ~board::fileMask(board::h1);
		Bitboard ne = (king << 9) & ~board::fileMask(board::a1);
		Bitboard sw = (king >> 9) & ~board::fileMask(board::h1);
		Bitboard se = (king >> 7) & ~board::fileMask(board::a1);
		return n | s | e | w | nw | ne | sw | se;
	}

	template<typename T>
	constexpr AttackMap knightAttacks(T idx)
	{
		Bitboard knight;
		if constexpr (std::is_same_v<T, board::square>)
		{
			knight = aux::setbit(idx);
		}
		else if constexpr (std::is_same_v<T, Bitboard>)
		{
			knight = idx;
		}
		else
		{
			return 0ULL;
		}
		// n = north, s = south, etc.
		Bitboard nnw = (knight << 15) & ~board::fileMask(board::h1);
		Bitboard nne = (knight << 17) & ~board::fileMask(board::a1);
		Bitboard nww = (knight << 6) & ~(board::fileMask(board::g1) | board::fileMask(board::h1));
		Bitboard nee = (knight << 10) & ~(board::fileMask(board::b1) | board::fileMask(board::a1));

		Bitboard ssw = (knight >> 17) & ~board::fileMask(board::h1);
		Bitboard sse = (knight >> 15) & ~board::fileMask(board::a1);
		Bitboard sww = (knight >> 10) & ~(board::fileMask(board::g1) | board::fileMask(board::h1));
		Bitboard see = (knight >> 6) & ~(board::fileMask(board::b1) | board::fileMask(board::a1));
		return nnw | nne | nww | nee | ssw | sse | sww | see;
	}

	// pawn moves/attacks are generated setwise
	constexpr AttackMap pawnAttacksLeft(Bitboard pawns)
	{
		return (pawns << 7) & ~board::fileMask(board::h1);
	}

	constexpr AttackMap pawnAttacksRight(Bitboard pawns)
	{
		return (pawns << 9) & ~board::fileMask(board::a1);
	}

	constexpr AttackMap enemyPawnAttacksLeft(Bitboard pawns)
	{
		return (pawns >> 9) & ~board::fileMask(board::h1);
	}

	constexpr AttackMap enemyPawnAttacksRight(Bitboard pawns)
	{
		return (pawns >> 7) & ~board::fileMask(board::a1);
	}

	constexpr Bitboard pawnMovesUp(Bitboard pawns)
	{
		return (pawns << 8);
	}

	constexpr Bitboard pawn2MovesUp(Bitboard pawns, Bitboard occ)
	{
		Bitboard empty = ~occ;
		pawns &= board::rankMask(board::a2);
		Bitboard upOnce = (pawns << 8) & empty;
		return (upOnce << 8) & empty;
	}

	// generate attacks given a bitboard (as opposed to a square)
	constexpr AttackMap genDiagAttackSet(Bitboard occ, Bitboard diag)
	{
		AttackMap attacks = 0;
		Bitboard lsb = 0;
		do
		{
			lsb = _blsi_u64(diag);
			attacks |= hypqDiag(occ, lsb) | hypqAntiDiag(occ, lsb);
		} while (diag ^= lsb);
		return attacks;
	}

	constexpr AttackMap genOrthAttackSet(Bitboard occ, Bitboard orth)
	{
		AttackMap attacks = 0;
		Bitboard lsb = 0;
		do
		{
			lsb = _blsi_u64(orth);
			attacks |= hypqFile(occ, lsb) | hypqRank(occ, lsb);
		} while (orth ^= lsb);
		return attacks;
	}

	constexpr Bitboard getVertPinnedPieces(Bitboard occ, Bitboard king, Bitboard orth)
	{
		AttackMap fileAttacks = 0;
		Bitboard lsb = 0;
		do
		{
			lsb = _blsi_u64(orth);
			fileAttacks |= hypqFile(occ, lsb);
		} while (orth ^= lsb);
		return hypqFile(occ, king) & fileAttacks & occ;
	}

	constexpr Bitboard getHorPinnedPieces(Bitboard occ, Bitboard king, Bitboard orth)
	{
		AttackMap rankAttacks = 0;
		Bitboard lsb = 0;
		do
		{
			lsb = _blsi_u64(orth);
			rankAttacks |= hypqRank(occ, lsb);
		} while (orth ^= lsb);
		return hypqRank(occ, king) & rankAttacks & occ;
	}

	constexpr Bitboard getDiagPinnedPieces(Bitboard occ, Bitboard king, Bitboard diag)
	{
		Bitboard lsb = 0;
		AttackMap diagAttacks = 0;
		do
		{
			lsb = _blsi_u64(diag);
			diagAttacks |= hypqDiag(occ, lsb);
		} while (diag ^= lsb);
		return hypqDiag(occ, king) & diagAttacks & occ;
	}

	constexpr Bitboard getAntiDiagPinnedPieces(Bitboard occ, Bitboard king, Bitboard diag)
	{
		Bitboard lsb = 0;
		AttackMap antiDiagAttacks = 0;
		do
		{
			lsb = _blsi_u64(diag);
			antiDiagAttacks |= hypqDiag(occ, lsb);
		} while (diag ^= lsb);
		return hypqAntiDiag(occ, king) & antiDiagAttacks & occ;
	}

	constexpr Bitboard getAllPinnedPieces(Bitboard occ, Bitboard king, Bitboard diag, Bitboard orth)
	{
		Bitboard pinned = 0;
		AttackMap diagAttacks = 0;
		AttackMap antiDiagAttacks = 0;
		AttackMap fileAttacks = 0;
		AttackMap rankAttacks = 0;
		Bitboard lsb = 0;
		do
		{
			lsb = _blsi_u64(diag);
			diagAttacks |= hypqDiag(occ, lsb);
			antiDiagAttacks |= hypqAntiDiag(occ, lsb);
		} while (diag ^= lsb);
		pinned |= hypqDiag(occ, king) & diagAttacks & occ;
		pinned |= hypqAntiDiag(occ, king) & antiDiagAttacks & occ;
		do
		{
			lsb = _blsi_u64(orth);
			fileAttacks |= hypqFile(occ, lsb);
			rankAttacks |= hypqRank(occ, lsb);
		} while (orth ^= lsb);
		pinned |= hypqFile(occ, king) & fileAttacks & occ;
		pinned |= hypqRank(occ, king) & rankAttacks & occ;
		return pinned;
	}

	constexpr AttackMap genEnemyAttacks(Bitboard occ, const board::QBB& b)
	{
		Bitboard attacks = genDiagAttackSet(occ, b.their(b.getDiagSliders()));
		attacks |= genOrthAttackSet(occ, b.their(b.getOrthSliders()));
		attacks |= enemyPawnAttacksLeft(b.their(b.getPawns()));
		attacks |= enemyPawnAttacksRight(b.their(b.getPawns()));
		attacks |= knightAttacks(b.their(b.getKnights()));
		attacks |= kingAttacks(b.their(b.getKings()));
		return attacks;
	}

	constexpr Bitboard getBetweenChecks(const board::QBB& b, Bitboard checkers) 
	{
		const Bitboard myKing = b.my(b.getKings());
		Bitboard occ = b.getOccupancy();

		checkers &= b.getDiagSliders() | b.getOrthSliders();
		Bitboard between = hypqDiag(occ, myKing) & hypqDiag(occ, checkers);
		between |= hypqAntiDiag(occ, myKing) & hypqAntiDiag(occ, checkers);
		between |= hypqFile(occ, myKing) & hypqFile(occ, checkers);
		between |= hypqRank(occ, myKing) & hypqRank(occ, checkers);

		return between;
	}

	constexpr Bitboard isInCheck(const board::QBB& b)
	{
		Bitboard myKing = b.my(b.getKings());
		Bitboard occ = b.getOccupancy();

		Bitboard checkers = b.their(b.getDiagSliders()) & (hypqDiag(occ, myKing) | hypqAntiDiag(occ, myKing));
		checkers |= b.their(b.getOrthSliders()) & (hypqFile(occ, myKing) | hypqRank(occ, myKing));
		checkers |= b.their(b.getKnights()) & knightAttacks(myKing);
		checkers |= b.their(b.getPawns()) & (pawnAttacksLeft(myKing) | pawnAttacksRight(myKing));

		return checkers;
	}

	// macros refer to the following variables:
	// unsigned long index
	// board::Move m
	// Bitboard pieces
#define MOVEGEN_LOOP_ATTACKS(attackExpression) while (_BitScanForward64(&index, pieces))\
{\
	AttackMap pieceAttacks = attackExpression;\
	pieces ^= _blsi_u64(pieces);\
	m = index;\
	while (_BitScanForward64(&index, pieceAttacks))\
	{\
		pieceAttacks ^= _blsi_u64(pieceAttacks);\
		m |= index << constants::toMaskOffset;\
		ml[i++] = m;\
		m &= constants::fromMask;\
	}\
}

#define MOVEGEN_LOOP_PAWN_MOVES(dest, offset) while (_BitScanForward64(&index, dest))\
{\
	m = index << constants::toMaskOffset;\
	m |= index - offset;\
	ml[i++] = m;\
}

#define MOVEGEN_LOOP_PAWN_PROMOS(dest, offset) while (_BitScanForward64(&index, dest))\
{\
	m = index << constants::toMaskOffset;\
	m |= index - offset;\
	m |= constants::queenPromo << constants::moveTypeOffset;\
	ml[i++] = m;\
	m &= constants::fromMask | constants::toMask;\
	m |= constants::knightPromo << constants::moveTypeOffset;\
	ml[i++] = m;\
	m &= constants::fromMask | constants::toMask;\
	m |= constants::rookPromo << constants::moveTypeOffset;\
	ml[i++] = m;\
	m &= constants::fromMask | constants::toMask;\
	m |= constants::bishopPromo << constants::moveTypeOffset;\
	ml[i++] = m;\
}

	template<std::size_t N, bool qSearch = false>
	std::size_t genMoves(const board::QBB& b, std::array<board::Move, N>& ml)
	{
		std::size_t i = 0;

		Bitboard checkers = isInCheck(b);

		if (!checkers)
		{
			Bitboard occ = b.getOccupancy();
			Bitboard myKing = b.my(b.getKings());
			Bitboard orth = b.my(b.getOrthSliders());
			Bitboard diag = b.my(b.getDiagSliders());
			Bitboard knights = b.my(b.getKnights());
			Bitboard pawns = b.my(b.getPawns());
			AttackMap enemyAttacks = genEnemyAttacks(occ, b);

			Bitboard horPinned = getHorPinnedPieces(occ, myKing, b.their(b.getOrthSliders())) & b.side;
			Bitboard vertPinned = getVertPinnedPieces(occ, myKing, b.their(b.getOrthSliders())) & b.side;
			Bitboard diagPinned = getDiagPinnedPieces(occ, myKing, b.their(b.getDiagSliders())) & b.side;
			Bitboard antiDiagPinned = getAntiDiagPinnedPieces(occ, myKing, b.their(b.getDiagSliders())) & b.side;
			Bitboard vertPinnedPawns = vertPinned & pawns;
			Bitboard diagPinnedPawns = diagPinned & pawns;
			Bitboard antiDiagPinnedPawns = antiDiagPinned & pawns;

			pawns &= ~(horPinned | antiDiagPinnedPawns | antiDiagPinnedPawns | vertPinnedPawns);
			knights &= ~(horPinned | vertPinned | diagPinned | antiDiagPinned);
			orth &= ~(diagPinned | antiDiagPinned);
			diag &= ~(horPinned | vertPinned);

			horPinned &= orth;
			vertPinned &= orth;
			diagPinned &= diag;
			antiDiagPinned &= diag;

			unsigned long index = 0;
			board::Move m = 0;

			Bitboard pieces = horPinned;
			MOVEGEN_LOOP_ATTACKS(hypqRank(occ, index));
			pieces = vertPinned;
			MOVEGEN_LOOP_ATTACKS(hypqFile(occ, index));
			pieces = diagPinned;
			MOVEGEN_LOOP_ATTACKS(hypqDiag(occ, index));
			pieces = antiDiagPinned;
			MOVEGEN_LOOP_ATTACKS(hypqAntiDiag(occ, index));
			pieces = antiDiagPinnedPawns;
			AttackMap attackPinner = pawnAttacksRight(pieces) & occ;
			AttackMap attackPinnerEP = pawnAttacksRight(pieces) & b.getEp();
			AttackMap attackPinner8 = attackPinner & board::rankMask(board::a8);
			attackPinner &= ~attackPinner8;
			while (_BitScanForward64(&index, attackPinnerEP))
			{
				m = index << constants::toMaskOffset;
				m |= constants::enPCap << constants::moveTypeOffset;
				m |= index - 9; 
				ml[i++] = m; 
			}
			MOVEGEN_LOOP_PAWN_MOVES(attackPinner, 9);
			MOVEGEN_LOOP_PAWN_PROMOS(attackPinner8, 9);
			pieces = diagPinnedPawns;
			attackPinner = pawnAttacksLeft(pieces) & occ;
			attackPinnerEP = pawnAttacksRight(pieces) & b.getEp();
			AttackMap attackPinner8 = attackPinner & board::rankMask(board::a8);
			attackPinner &= ~attackPinner8;
			while (_BitScanForward64(&index, attackPinnerEP))
			{
				m = index << constants::toMaskOffset;
				m |= constants::enPCap << constants::moveTypeOffset;
				m |= index - 7;
				ml[i++] = m;
			}
			MOVEGEN_LOOP_PAWN_MOVES(attackPinner, 7);
			MOVEGEN_LOOP_PAWN_PROMOS(attackPinner8, 7);
			pieces = vertPinnedPawns;
			Bitboard movesUp = pawnMovesUp(pieces) & ~occ;
			Bitboard twoMovesUp = pawn2MovesUp(pieces, occ);
			MOVEGEN_LOOP_PAWN_MOVES(movesUp, 8);
			MOVEGEN_LOOP_PAWN_MOVES(twoMovesUp, 16);

			pieces = knights;
			MOVEGEN_LOOP_ATTACKS(knightAttacks(index) & ~b.side);
			pieces = orth;
			MOVEGEN_LOOP_ATTACKS((hypqRank(occ, index) | hypqFile(occ, index)) & ~b.side);
			pieces = diag;
			MOVEGEN_LOOP_ATTACKS((hypqAntiDiag(occ, index) | hypqDiag(occ, index)) & ~b.side);
			pieces = myKing;
			MOVEGEN_LOOP_ATTACKS(kingAttacks(myKing) & ~enemyAttacks & ~b.side);
			pieces = pawns;
			Bitboard enpAttacks = enemyPawnAttacksLeft(b.getEp()) | enemyPawnAttacksRight(b.getEp());
			enpAttacks &= pawns;
			AttackMap leftAttacks = pawnAttacksLeft(pieces) & b.their(occ);
			AttackMap leftAttacks8 = leftAttacks & board::rankMask(a8);
			leftAttacks &= ~leftAttacks8;
			AttackMap rightAttacks = enemyPawnAttacksRight(pieces) & b.their(occ);
			AttackMap rightAttacks8 = rightAttacks & board::rankMask(board::a8);
			rightAttacks &= ~rightAttacks8;
			Bitboard movesUp = pawnMovesUp(pieces) & ~occ;
			AttackMap movesUp8 = movesUp & board::rankMask(board::a8);
			movesUp &= ~movesUp8;
			Bitboard twoMovesUp = pawn2MovesUp(pieces, occ);
			MOVEGEN_LOOP_PAWN_MOVES(twoMovesUp, 16);
			MOVEGEN_LOOP_PAWN_MOVES(movesUp, 8);
			MOVEGEN_LOOP_PAWN_MOVES(leftAttacks, 7);
			MOVEGEN_LOOP_PAWN_MOVES(rightAttacks, 9);
			MOVEGEN_LOOP_PAWN_PROMOS(leftAttacks8, 7);
			MOVEGEN_LOOP_PAWN_PROMOS(rightAttacks8, 9);
			MOVEGEN_LOOP_PAWN_PROMOS(movesUp8, 8);
			unsigned enpsq = _tzcnt_u64(b.getEp());
			while (_BitScanForward64(&index, enpAttacks))
			{
				m = index;
				m |= enpsq << constants::toMaskOffset;
				m |= constants::enPCap << constants::moveTypeOffset;
				ml[i++] = m;
			}
			if (b.canCastleShort() && (enemyAttacks & aux::setbit(board::f1)) && (enemyAttacks & aux::setbit(board::g1)))
			{
				m = board::e1;
				m |= board::g1 << constants::toMaskOffset;
				m |= constants::KSCastle << constants::moveTypeOffset;
				ml[i++] = m;
			}
			if (b.canCastleLong() && (enemyAttacks & aux::setbit(board::d1)) && (enemyAttacks & aux::setbit(board::c1)) && (enemyAttacks & aux::setbit(board::b1)))
			{
				m = board::e1;
				m |= board::c1 << constants::toMaskOffset;
				m |= constants::QSCastle << constants::moveTypeOffset;
				ml[i++] = m;
			}
		}
		else if (__popcnt64(checkers) == 1)
		{
			Bitboard myKing = b.my(b.getKings());
			Bitboard occ = b.getOccupancy();
			Bitboard enemyDiag = b.their(b.getDiagSliders());
			Bitboard enemyOrth = b.their(b.getOrthSliders());
			Bitboard pinned = getAllPinnedPieces(occ, myKing, enemyDiag, enemyOrth);

			// rare: checker can be captured by en passant
			Bitboard enpChecker = (checkers << 8) & b.getEp();
			
			checkers |= getBetweenChecks(b, checkers);
			
			Bitboard attacks = genEnemyAttacks(occ & ~myKing, b);

			AttackMap dest = kingAttacks(myKing) & ~attacks & ~b.side;
			board::Move m = _tzcnt_u64(myKing);
			unsigned long index = 0;
			while (_BitScanForward64(&index, dest))
			{
				dest ^= _blsi_u64(dest);
				m |= index << constants::toMaskOffset;
				ml[i++] = m;
				m &= constants::fromMask;
			}

			Bitboard pieces = b.my(b.getKnights()) & ~pinned;
			MOVEGEN_LOOP_ATTACKS(knightAttacks(index) & checkers);

			pieces = b.my(b.getDiagSliders()) & ~pinned;
			MOVEGEN_LOOP_ATTACKS((hypqDiag(occ, index) | hypqAntiDiag(occ, index)) & checkers);

			pieces = b.my(b.getOrthSliders()) & ~pinned;
			MOVEGEN_LOOP_ATTACKS((hypqFile(occ, index) | hypqRank(occ, index)) & checkers);

			pieces = b.my(b.getPawns()) & ~pinned;
			AttackMap leftAttacks = pawnAttacksLeft(pieces) & checkers & occ;
			AttackMap leftAttacks8 = leftAttacks & board::rankMask(a8);
			leftAttacks &= ~leftAttacks8;
			AttackMap rightAttacks = enemyPawnAttacksRight(pieces) & checkers & occ;
			AttackMap rightAttacks8 = rightAttacks & board::rankMask(board::a8);
			rightAttacks &= ~rightAttacks8;
			Bitboard movesUp = pawnMovesUp(pieces) & checkers & ~occ;
			AttackMap movesUp8 = movesUp & board::rankMask(board::a8);
			movesUp &= ~movesUp8;
			Bitboard twoMovesUp = pawn2MovesUp(pieces, occ) & checkers;
			MOVEGEN_LOOP_PAWN_MOVES(twoMovesUp, 16);
			MOVEGEN_LOOP_PAWN_MOVES(movesUp, 8);
			MOVEGEN_LOOP_PAWN_MOVES(leftAttacks, 7);
			MOVEGEN_LOOP_PAWN_MOVES(rightAttacks, 9);
			MOVEGEN_LOOP_PAWN_PROMOS(leftAttacks8, 7);
			MOVEGEN_LOOP_PAWN_PROMOS(rightAttacks8, 9);
			MOVEGEN_LOOP_PAWN_PROMOS(movesUp8, 8);

			AttackMap enpCheckerAttacks = enemyPawnAttacksLeft(enpChecker) | enemyPawnAttacksRight(enpChecker);
			enpCheckerAttacks &= b.my(b.getPawns());
			unsigned enpsq = _tzcnt_u64(enpChecker);
			while (_BitScanForward64(&index, enpCheckerAttacks))
			{
				m = index;
				m |= enpsq << constants::toMaskOffset;
				m |= constants::enPCap << constants::moveTypeOffset;
				ml[i++] = m;
			}
		}
		else // >1 checkers (double check)
		{
			Bitboard myKing = b.my(b.getKings());
			Bitboard occ = b.getOccupancy();
			occ &= ~myKing; // remove king to generate X rays through king
			
			AttackMap attacks = genEnemyAttacks(occ, b);

			AttackMap dest = kingAttacks(myKing) & ~attacks & ~b.side;
			board::Move m = _tzcnt_u64(myKing);
			unsigned long index = 0;
			while (_BitScanForward64(&index, dest))
			{
				dest ^= _blsi_u64(dest);
				m |= index << 6;
				ml[i++] = m;
				m &= constants::fromMask;
			}
		}

		return i;
	}

#undef MOVEGEN_LOOP_ATTACKS
}
#endif