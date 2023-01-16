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

#include <immintrin.h>
#include <intrin.h>

#pragma intrinsic(__popcnt64)
#pragma intrinsic(_BitScanForward64)

#include <cstddef>

#include "movegen.hpp"
#include "board.hpp"
#include "auxiliary.hpp"

namespace movegen
{
	// generate attacks given a bitboard (as opposed to a square)
	AttackMap genDiagAttackSet(Bitboard occ, Bitboard diag)
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
	AttackMap genOrthAttackSet(Bitboard occ, Bitboard orth)
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
	Bitboard getVertPinnedPieces(Bitboard occ, Bitboard king, Bitboard orth)
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
	Bitboard getHorPinnedPieces(Bitboard occ, Bitboard king, Bitboard orth)
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
	Bitboard getDiagPinnedPieces(Bitboard occ, Bitboard king, Bitboard diag)
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
	Bitboard getAntiDiagPinnedPieces(Bitboard occ, Bitboard king, Bitboard diag)
	{
		Bitboard lsb = 0;
		AttackMap antiDiagAttacks = 0;
		do
		{
			lsb = _blsi_u64(diag);
			antiDiagAttacks |= hypqAntiDiag(occ, lsb);
		} while (diag ^= lsb);
		return hypqAntiDiag(occ, king) & antiDiagAttacks & occ;
	}
	Bitboard getAllPinnedPieces(Bitboard occ, Bitboard king, Bitboard diag, Bitboard orth)
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
	AttackMap genEnemyAttacks(Bitboard occ, const board::QBB& b)
	{
		Bitboard attacks = genDiagAttackSet(occ, b.their(b.getDiagSliders()));
		attacks |= genOrthAttackSet(occ, b.their(b.getOrthSliders()));
		attacks |= enemyPawnAttacksLeft(b.their(b.getPawns()));
		attacks |= enemyPawnAttacksRight(b.their(b.getPawns()));
		attacks |= knightAttacks(b.their(b.getKnights()));
		attacks |= kingAttacks(b.their(b.getKings()));
		return attacks;
	}
	Bitboard getBetweenChecks(const board::QBB& b, Bitboard checkers)
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
	Bitboard isInCheck(const board::QBB& b)
	{
		Bitboard myKing = b.my(b.getKings());
		Bitboard occ = b.getOccupancy();

		Bitboard checkers = b.their(b.getDiagSliders()) & (hypqDiag(occ, myKing) | hypqAntiDiag(occ, myKing));
		checkers |= b.their(b.getOrthSliders()) & (hypqFile(occ, myKing) | hypqRank(occ, myKing));
		checkers |= b.their(b.getKnights()) & knightAttacks(myKing);
		checkers |= b.their(b.getPawns()) & (pawnAttacksLeft(myKing) | pawnAttacksRight(myKing));

		return checkers;
	}
	Bitboard getSqAttackers(const board::QBB& b, board::square s)
	{
		AttackMap orth = hypqAllOrth(b.getOccupancy(), s) & b.getOrthSliders();
		AttackMap diag = hypqAllDiag(b.getOccupancy(), s) & b.getDiagSliders();
		AttackMap knight = knightAttacks(s) & b.getKnights();
		AttackMap kings = kingAttacks(s) & b.getKings();
		AttackMap pawns = pawnAttacks(s) & b.their(b.getPawns());
		pawns |= enemyPawnAttacks(s) & b.my(b.getPawns());
		return orth | diag | knight | kings | pawns;
	}
	Bitboard getSliderAttackers(Bitboard occ, board::square s, Bitboard diag, Bitboard orth)
	{
		orth &= hypqAllOrth(occ, s);
		diag &= hypqAllDiag(occ, s);
		return orth | diag;
	}
}