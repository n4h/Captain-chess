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

#ifndef MOVEGEN_H
#define MOVEGEN_H

#include <immintrin.h>
#include <intrin.h>

#pragma intrinsic(__popcnt64)
#pragma intrinsic(_BitScanForward64)


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

	template<std::size_t N>
	struct Movelist
	{
	private:
		std::array<board::Move, N> ml;
		std::size_t i = 0;
	public:
		void push_back(board::Move m)
		{
			ml[i++] = m;
		}
		board::Move& operator[](std::size_t k)
		{
			return ml[k];
		}
		auto begin()
		{
			return ml.begin();
		}
		auto end()
		{
			return ml.begin() + i;
		}
		constexpr std::size_t size() const noexcept
		{
			return i;
		}
		constexpr std::size_t max_size() const noexcept
		{
			return N;
		}
	};

	// move generation is based on "Hyperbola Quintessence" algorithm
	// https://www.chessprogramming.org/Hyperbola_Quintessence


	template<typename T>
	AttackMap hypqDiag(Bitboard o, T idx)
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
	AttackMap hypqAntiDiag(Bitboard o, T idx)
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
	AttackMap hypqFile(Bitboard o, T idx)
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
	AttackMap hypqAllOrth(Bitboard o, T idx)
	{
		return hypqFile(o, idx) | hypqRank(o, idx);
	}

	template<typename T>
	AttackMap hypqAllDiag(Bitboard o, T idx)
	{
		return hypqDiag(o, idx) | hypqAntiDiag(o, idx);
	}

	template<typename T>
	AttackMap kingAttacks(T idx)
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
	AttackMap knightAttacks(T idx)
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
	AttackMap genDiagAttackSet(Bitboard occ, Bitboard diag);

	AttackMap genOrthAttackSet(Bitboard occ, Bitboard orth);

	Bitboard getVertPinnedPieces(Bitboard occ, Bitboard king, Bitboard orth);

	Bitboard getHorPinnedPieces(Bitboard occ, Bitboard king, Bitboard orth);

	Bitboard getDiagPinnedPieces(Bitboard occ, Bitboard king, Bitboard diag);

	Bitboard getAntiDiagPinnedPieces(Bitboard occ, Bitboard king, Bitboard diag);

	Bitboard getAllPinnedPieces(Bitboard occ, Bitboard king, Bitboard diag, Bitboard orth);

	AttackMap genEnemyAttacks(Bitboard occ, const board::QBB& b);

	Bitboard getBetweenChecks(const board::QBB& b, Bitboard checkers);

	Bitboard isInCheck(const board::QBB& b);

	template<typename Attacks, std::size_t N>
	void addMoves(Bitboard pieces, Movelist<N>& ml, Attacks dest)
	{
		unsigned long index;
		while (_BitScanForward64(&index, pieces))
		{
			AttackMap pieceAttacks = dest(static_cast<board::square>(index));
			pieces = _blsr_u64(pieces);
			board::Move m = index;
			while (_BitScanForward64(&index, pieceAttacks))
			{
				pieceAttacks = _blsr_u64(pieceAttacks);
				m |= index << constants::toMaskOffset;
				ml.push_back(m);
				m &= constants::fromMask;
			}
		}
	}

	template<bool promos, std::size_t offset, typename Attacks, std::size_t N>
	void addPawnMoves(Bitboard pawns, Movelist<N>& ml, Attacks dest)
	{
		AttackMap attacks = dest(pawns);
		unsigned long index;
		AttackMap not8 = attacks;
		if constexpr (promos)
		{
			not8 &= ~board::rankMask(board::a8);
		}
		while (_BitScanForward64(&index, not8))
		{
			board::Move m = index - offset;
			not8 = _blsr_u64(not8);
			m |= index << constants::toMaskOffset;
			ml.push_back(m);
		}
		if constexpr (promos)
		{
			AttackMap on8 = attacks & board::rankMask(board::a8);
			while (_BitScanForward64(&index, on8))
			{
				board::Move m = index - offset;
				on8 = _blsr_u64(not8);
				m |= index << constants::toMaskOffset;
				m |= constants::queenPromo << constants::moveTypeOffset;
				ml.push_back(m);
				m &= 07777U;
				m |= constants::knightPromo << constants::moveTypeOffset;
				ml.push_back(m);
				m &= 07777U;
				m |= constants::rookPromo << constants::moveTypeOffset;
				ml.push_back(m);
				m &= 07777U;
				m |= constants::bishopPromo << constants::moveTypeOffset;
				ml.push_back(m);
			}
		}
	}
#define addLeftPMoves movegen::addPawnMoves<true, 7>
#define addRightPMoves movegen::addPawnMoves<true, 9>
#define addUpPMoves movegen::addPawnMoves<true, 8>
#define addPinUpPMove movegen::addPawnMoves<false, 8>
#define addUp2PMoves movegen::addPawnMoves<false, 16>

	template<std::size_t N>
	void addEPMoves(Movelist<N>& ml, Bitboard pawns, Bitboard enp)
	{
		AttackMap enpAttacksL = enemyPawnAttacksLeft(enp) & pawns;
		AttackMap enpAttacksR = enemyPawnAttacksRight(enp) & pawns;
		unsigned long index;
		if (_BitScanForward64(&index, enpAttacksL))
		{
			board::Move m = index;
			m |= (index + 9) << constants::toMaskOffset;
			m |= constants::enPCap << constants::moveTypeOffset;
			ml.push_back(m);
		}
		if (_BitScanForward64(&index, enpAttacksR))
		{
			board::Move m = index;
			m |= (index + 7) << constants::toMaskOffset;
			m |= constants::enPCap << constants::moveTypeOffset;
			ml.push_back(m);
		}
	}
	constexpr bool QSearch = true;
	template<bool qSearch = false, std::size_t N>
	void genMoves(const board::QBB& b, Movelist<N>& ml)
	{
		Bitboard checkers = isInCheck(b);

		if (!checkers)
		{
			Bitboard occ = b.getOccupancy();
			Bitboard mine = b.side;
			Bitboard theirs = occ & ~mine;
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
			vertPinned -= vertPinnedPawns;
			Bitboard diagPinnedPawns = diagPinned & pawns;
			diagPinned -= diagPinnedPawns;
			Bitboard antiDiagPinnedPawns = antiDiagPinned & pawns;
			antiDiagPinned -= antiDiagPinnedPawns;

			pawns &= ~(horPinned | antiDiagPinnedPawns | diagPinnedPawns | vertPinnedPawns);
			knights &= ~(horPinned | vertPinned | diagPinned | antiDiagPinned);
			orth &= ~(diagPinned | antiDiagPinned);
			diag &= ~(horPinned | vertPinned);

			horPinned &= orth;
			orth -= horPinned;
			vertPinned &= orth;
			orth -= vertPinned;
			diagPinned &= diag;
			diag -= diagPinned;
			antiDiagPinned &= diag;
			diag -= antiDiagPinned;

			if constexpr (qSearch)
				mine = ~theirs;
			addMoves(horPinned, ml, [occ, mine](board::square idx) {
				return hypqRank(occ, idx) & ~mine; });

			addMoves(vertPinned, ml, [occ, mine](board::square idx) {
				return hypqFile(occ, idx) & ~mine; });

			addMoves(diagPinned, ml, [occ, mine](board::square idx) {
				return hypqDiag(occ, idx) & ~mine; });

			addMoves(antiDiagPinned, ml, [occ, mine](board::square idx) {
				return hypqAntiDiag(occ, idx) & ~mine; });

			addLeftPMoves(diagPinnedPawns, ml, [theirs](Bitboard pawns) 
				{return pawnAttacksLeft(pawns) & theirs; });

			addRightPMoves(antiDiagPinnedPawns, ml, [theirs](Bitboard pawns) {
				return pawnAttacksRight(pawns) & theirs; });

			Bitboard pinnedPawnEP = (enemyPawnAttacksLeft(b.getEp()) & antiDiagPinnedPawns) | (enemyPawnAttacksRight(b.getEp()) & diagPinnedPawns);
			addEPMoves(ml, pinnedPawnEP, b.getEp());
			
			if constexpr (!qSearch)
			{
				addPinUpPMove(vertPinnedPawns, ml, [occ](Bitboard pawns) {
					return pawnMovesUp(pawns) & ~occ; });

				addUp2PMoves(vertPinnedPawns, ml, [occ](Bitboard pawns) {
					return pawn2MovesUp(pawns, occ); });
			}

			addMoves(knights, ml, [mine](board::square idx) {
				return knightAttacks(idx) & ~mine; });

			addMoves(orth, ml, [occ, mine](board::square idx) {
				return hypqAllOrth(occ, idx) & ~mine; });

			addMoves(diag, ml, [occ, mine](board::square idx) {
				return hypqAllDiag(occ, idx) & ~mine; });

			addMoves(myKing, ml, [mine, enemyAttacks](board::square idx) {
				return kingAttacks(idx) & ~mine & ~enemyAttacks; });
			
			addLeftPMoves(pawns, ml, [theirs](Bitboard pawns) {
				return pawnAttacksLeft(pawns) & theirs; });

			addRightPMoves(pawns, ml, [theirs](Bitboard pawns) {
				return pawnAttacksRight(pawns) & theirs; });

			if constexpr (!qSearch)
			{
				addUpPMoves(pawns, ml, [occ](Bitboard pawns) {
					return pawnMovesUp(pawns) & ~occ; });

				addUp2PMoves(pawns, ml, [occ](Bitboard pawns) {
					return pawn2MovesUp(pawns, occ); });
			}

			Bitboard myKing5 = myKing & board::rankMask(board::a5);
			Bitboard theirOrth5 = b.their(b.getOrthSliders()) & board::rankMask(board::a5);
			Bitboard EPPin = getHorPinnedPieces(occ & ~(b.getEp() >> 8), myKing5, theirOrth5);
			addEPMoves(ml, pawns & ~EPPin, b.getEp());
			if constexpr (!qSearch)
			{
				constexpr Bitboard betweenKSCastle = aux::setbit(board::f1) | aux::setbit(board::g1);
				if (b.canCastleShort() && !((enemyAttacks | occ) & betweenKSCastle))
				{
					board::Move m = board::e1;
					m |= board::g1 << constants::toMaskOffset;
					m |= constants::KSCastle << constants::moveTypeOffset;
					ml.push_back(m);
				}
				constexpr Bitboard betweenQSCastle = aux::setbit(board::d1) | aux::setbit(board::c1) | aux::setbit(board::b1);
				constexpr Bitboard QSCastleNoAttack = aux::setbit(board::d1) | aux::setbit(board::c1);
				if (b.canCastleLong() && !(enemyAttacks & QSCastleNoAttack) && !(occ & betweenQSCastle))
				{
					board::Move m = board::e1;
					m |= board::c1 << constants::toMaskOffset;
					m |= constants::QSCastle << constants::moveTypeOffset;
					ml.push_back(m);
				}
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
			
			Bitboard enemyAttacks = genEnemyAttacks(occ & ~myKing, b);

			Bitboard mine = b.side;

			addMoves(myKing, ml, [enemyAttacks, mine](board::square idx) {
				return kingAttacks(idx) & ~enemyAttacks & ~mine; });

			addMoves(b.my(b.getKnights()) & ~pinned, ml, [checkers](board::square idx) {
				return knightAttacks(idx) & checkers; });

			addMoves(b.my(b.getDiagSliders()) & ~pinned, ml, [occ, checkers](board::square idx) {
				return hypqAllDiag(occ, idx) & checkers; });

			addMoves(b.my(b.getOrthSliders()) & ~pinned, ml, [occ, checkers](board::square idx) {
				return hypqAllOrth(occ, idx) & checkers; });

			addLeftPMoves(b.my(b.getPawns()) & ~pinned, ml, [checkers, occ](Bitboard pawns) {
				return pawnAttacksLeft(pawns) & checkers & occ; });

			addRightPMoves(b.my(b.getPawns()) & ~pinned, ml, [checkers, occ](Bitboard pawns) {
				return pawnAttacksRight(pawns) & checkers & occ; });

			addUpPMoves(b.my(b.getPawns()) & ~pinned, ml, [checkers, occ](Bitboard pawns) {
				return pawnMovesUp(pawns) & checkers & ~occ; });

			addUp2PMoves(b.my(b.getPawns()) & ~pinned, ml, [checkers, occ](Bitboard pawns) {
				return pawn2MovesUp(pawns, occ) & checkers; });

			addEPMoves(ml, b.my(b.getPawns()) & ~pinned, enpChecker);
		}
		else // >1 checkers (double check)
		{
			Bitboard myKing = b.my(b.getKings());
			Bitboard occ = b.getOccupancy();
			Bitboard mine = occ & b.side;
			occ &= ~myKing; // remove king to generate X rays through king

			AttackMap attacks = genEnemyAttacks(occ, b);
			addMoves(myKing, ml, [mine, attacks](board::square idx) {return kingAttacks(idx) & ~attacks & ~mine; });
		}
	}
}
#endif