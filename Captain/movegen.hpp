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

	constexpr AttackMap hypqDiag(Bitboard o, board::square idx)
	{
		o &= board::diagMask(idx);
		Bitboard r = aux::setbit(idx);
		Bitboard orev = _byteswap_uint64(o);
		Bitboard rrev = _byteswap_uint64(r);

		return board::diagMask(idx) & ((o - 2 * r) ^ _byteswap_uint64(orev - 2 * rrev));
	}

	constexpr AttackMap hypqAntiDiag(Bitboard o, board::square idx)
	{
		o &= board::antiDiagMask(idx);
		Bitboard r = aux::setbit(idx);
		Bitboard orev = _byteswap_uint64(o);
		Bitboard rrev = _byteswap_uint64(r);

		return board::antiDiagMask(idx) & ((o - 2 * r) ^ _byteswap_uint64(orev - 2 * rrev));
	}

	constexpr AttackMap hypqFile(Bitboard o, board::square idx)
	{
		o &= board::fileMask(idx);
		Bitboard r = aux::setbit(idx);
		Bitboard orev = _byteswap_uint64(o);
		Bitboard rrev = _byteswap_uint64(r);

		return board::fileMask(idx) & ((o - 2 * r) ^ _byteswap_uint64(orev - 2 * rrev));
	}
	
	AttackMap hypqRank(Bitboard o, board::square idx);


	// move generation in a particular direction
	constexpr AttackMap hypqDiagNE(Bitboard o, board::square idx)
	{
		o &= board::antiDiagMask(idx);
		Bitboard r = aux::setbit(idx);
		return board::antiDiagMask(idx) & (o ^ (o - 2 * r));
	}

	template<typename T>
	constexpr AttackMap hypqDiagSW(Bitboard o, T idx)
	{
		if constexpr (std::is_same_v<T, board::square>)
		{
			o &= board::antiDiagMask(idx);
			Bitboard r = aux::setbit(idx);
			Bitboard orev = _byteswap_uint64(o);
			Bitboard rrev = _byteswap_uint64(r);
			return board::antiDiagMask(idx) & (o ^ _byteswap_uint64(orev - 2 * rrev));
		}
		else if constexpr (std::is_same_v<T, Bitboard>)
		{
			o &= board::multiAntiDiagMask(idx);
			Bitboard orev = _byteswap_uint64(o);
			Bitboard rrev = _byteswap_uint64(idx);
			return board::multiAntiDiagMask(idx) & (o ^ _byteswap_uint64(orev - 2 * rrev));
		}
		else
		{
			return 0ULL;
		}
	}

	template<typename T>
	constexpr AttackMap hypqDiagNW(Bitboard o, T idx)
	{
		if constexpr (std::is_same_v<T, board::square>)
		{
			o &= board::diagMask(idx);
			Bitboard r = aux::setbit(idx);
			return board::diagMask(idx) & (o ^ (o - 2 * r));
		}
		else if constexpr (std::is_same_v<T, Bitboard>)
		{
			o &= board::multiDiagMask(idx);
			return board::multiDiagMask(idx) & (o ^ (o - 2 * r));
		}
		else
		{
			return 0ULL;
		}
	}

	constexpr AttackMap hypqDiagSE(Bitboard o, board::square idx)
	{
		o &= board::diagMask(idx);
		Bitboard r = aux::setbit(idx);
		Bitboard orev = _byteswap_uint64(o);
		Bitboard rrev = _byteswap_uint64(r);
		return board::diagMask(idx) & (o ^ _byteswap_uint64(orev - 2 * rrev));
	}

	constexpr AttackMap hypqFileN(Bitboard o, board::square idx)
	{
		o &= board::fileMask(idx);
		Bitboard r = aux::setbit(idx);

		return board::fileMask(idx) & ((o - 2 * r) ^ o);
	}

	constexpr AttackMap hypqFileS(Bitboard o, board::square idx)
	{
		o &= board::fileMask(idx);
		Bitboard r = aux::setbit(idx);
		Bitboard orev = _byteswap_uint64(o);
		Bitboard rrev = _byteswap_uint64(r);
		return board::fileMask(idx) & (o ^ _byteswap_uint64(orev - 2 * rrev));
	}

	constexpr AttackMap hypqRankE(Bitboard o, board::square idx)
	{
		o &= board::fileMask(idx);
		Bitboard r = aux::setbit(idx);

		return board::fileMask(idx) & ((o - 2 * r) ^ o);
	}

	AttackMap hypqRankW(Bitboard o, board::square idx);

	constexpr AttackMap kingAttacks(board::square idx)
	{
		const Bitboard king = aux::setbit(idx);

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

	constexpr AttackMap knightAttacks(board::square sq)
	{
		Bitboard knight = aux::setbit(sq);
		return genKnightAttackSet(knight);
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

	// generate attacks given a set of bitboards (as opposed to a square)
	constexpr AttackMap genDiagAttackSet(Bitboard occ, Bitboard diag)
	{
		unsigned long index;
		AttackMap attacks = 0;
		while (_BitScanForward64(&index, diag))
		{
			auto sq = static_cast<board::square>(index);
			diag ^= _blsi_u64(diag);
			attacks |= hypqDiag(occ, sq) | hypqAntiDiag(occ, sq);
		}
		return attacks;
	}

	constexpr AttackMap genOrthAttackSet(Bitboard occ, Bitboard orth)
	{
		unsigned long index;
		AttackMap attacks = 0;
		while (_BitScanForward64(&index, orth))
		{
			auto sq = static_cast<board::square>(index);
			orth ^= _blsi_u64(orth);
			attacks |= hypqFile(occ, sq) | hypqRank(occ, sq);
		}
		return attacks;
	}

	constexpr AttackMap genKnightAttackSet(Bitboard knight)
	{
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

	constexpr AttackMap kingAttacksSet(Bitboard king)
	{
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

	constexpr Bitboard getAllPinnedPieces(Bitboard occ, Bitboard king, Bitboard diag, Bitboard orth)
	{
		Bitboard pinned = 0;
		pinned |= 
	}

	constexpr Bitboard isInCheck(const board::QBB& b) // UNDONE isInCheck
	{
		const Bitboard myKing = b.my(b.getKings());
		unsigned long index = 0;
		_BitScanForward64(&index, myKing);
		board::square sq = static_cast<board::square>(index);
		const Bitboard theirDiagSliders = b.their(b.getDiagonalSliders());
		const Bitboard theirOrthSliders = b.their(b.getOrthogonalSliders());
		const Bitboard theirPawns = b.their(b.getPawns());
		const Bitboard theirKnights = b.their(b.getKnights());
		const Bitboard occ = b.getOccupancy();

		const AttackMap diagonal = hypqDiag(occ, sq) | hypqAntiDiag(occ, sq);
		const AttackMap orthogonal = hypqFile(occ, sq) | hypqRank(occ, sq);
		const AttackMap knights = knightAttacks(sq);
		const AttackMap pawns = pawnAttacksLeft(myKing) | pawnAttacksRight(myKing);

		return (diagonal & theirDiagSliders)
			| (orthogonal & theirOrthSliders)
			| (theirKnights & knights)
			| (theirPawns & pawns);
	}

	template<std::size_t N, bool qSearch = false>
	std::size_t genMoves(const board::QBB& b, std::array<board::Move, N>& ml)
	{
		std::size_t i = 0;
		Bitboard occ = b.getOccupancy();
		Bitboard mine = b.my(occ);
		Bitboard pawns = b.getPawns();
		Bitboard knights = b.getKnights();
		Bitboard king = b.getKings();
		Bitboard diagonalSliders = b.getDiagonalSliders();
		Bitboard orthSliders = b.getOrthogonalSliders();

		const Bitboard myKing = b.my(b.getKings());
		unsigned long index = 0;
		_BitScanForward64(&index, myKing);
		board::square sq = static_cast<board::square>(index);

		Bitboard DiagChecks = hypqDiag(occ, sq) & b.their(diagonalSliders);
		Bitboard AntiDiagChecks = hypqAntiDiag(occ, sq) & b.their(diagonalSliders);

		Bitboard Nchecks = hypqFileN(occ, sq) & b.their(orthSliders);
		Bitboard Schecks = hypqFileS(occ, sq) & b.their(orthSliders);

		Bitboard PchecksLeft = pawnAttacksLeft(myKing) & b.their(pawns);
		Bitboard PchecksRight = pawnAttacksRight(myKing) & b.their(pawns);
		Bitboard knightChecks = knightAttacks(sq) & b.their(knights);

		Bitboard diagCheckers = DiagChecks | AntiDiagChecks;
		Bitboard orthCheckers = Nchecks | Schecks | Echecks | Wchecks;
		Bitboard pawnCheckers = PchecksLeft | PchecksRight;

		const Bitboard checkers = diagCheckers | orthCheckers | knightChecks | pawnCheckers;

		if (!checkers)
		{
			Bitboard horPinned = hypqRankW(occ, sq) & multi
		}
		else if (__popcnt64(checkers) == 1)
		{

		}
		else // >1 checkers (double check)
		{
			board::Move m = sq;
			AttackMap kingMoves = kingAttacks(sq);
			kingMoves &= ~genDiagAttackSet(b.their(diagonalSliders));
			kingMoves &= ~genOrthAttackSet(b.their(orthSliders));
			kingMoves &= ~kingAttacks(b.their(king));
			kingMoves &= ~genKnightAttackSet(b.their(knights));
			kingMoves &= ~()
		}

		return i;
	}
}
#endif