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
	bool isLegalMove(const board::QBB& b, board::Move m)
	{
		const auto moveType = board::getMoveInfo<constants::moveTypeMask>(m);
		if (moveType > constants::queenPromo)
			return false;

		board::square fromSq = board::getMoveFromSq(m);
		auto fromPcType = b.getPieceType(fromSq);

		if (!(fromPcType & 1))
		{
			return false;
		}
		const Bitboard occ = b.getOccupancy();
		auto toSq = board::getMoveToSq(m);
		switch (fromPcType >> 1)
		{
		case constants::pawnCode:
		{
			AttackMap pAttacks = pawnAttacks(fromSq) & b.their(occ);
			AttackMap pEpAttacks = pawnAttacks(fromSq) & b.getEp();
			Bitboard pMoves = forwardPawnMoves(occ, fromSq);

			if (!((pMoves | pEpAttacks | pAttacks) & aux::setbit(toSq)))
			{
				return false;
			}

			if (toSq >= 56)
			{
				if (moveType < constants::knightPromo)
				{
					return false;
				}
			}
			else if (aux::setbit(toSq) & pEpAttacks)
			{
				if (moveType != constants::enPCap)
				{
					return false;
				}
			}
			else if (moveType != constants::QMove)
			{
				return false;
			}

			break;
		}
		case constants::knightCode:
		{
			AttackMap nAttacks = knightAttacks(fromSq) & ~b.my(occ);
			if (!(nAttacks & aux::setbit(toSq)))
			{
				return false;
			}
			if (moveType != constants::QMove)
			{
				return false;
			}
			break;
		}
		case constants::bishopCode:
		{
			AttackMap bAttacks = hypqAllDiag(occ, fromSq) & ~b.my(occ);
			if (!(bAttacks & aux::setbit(toSq)))
			{
				return false;
			}
			if (moveType != constants::QMove)
			{
				return false;
			}
			break;
		}
		case constants::rookCode:
		{
			AttackMap rAttacks = hypqAllOrth(occ, fromSq) & ~b.my(occ);
			if (!(rAttacks & aux::setbit(toSq)))
			{
				return false;
			}
			if (moveType != constants::QMove)
			{
				return false;
			}
			break;
		}
		case constants::queenCode:
		{
			AttackMap qAttacks = (hypqAllOrth(occ, fromSq) | hypqAllDiag(occ, fromSq)) & ~b.my(occ);
			if (!(qAttacks & aux::setbit(toSq)))
			{
				return false;
			}
			if (moveType != constants::QMove)
			{
				return false;
			}
			break;
		}
		case constants::kingCode:
		{
			if (moveType == constants::KSCastle)
			{
				if (fromSq != board::e1 || toSq != board::g1)
					return false;
				if (!b.canCastleShort())
					return false;
				if (occ & (aux::setbit(board::f1) | aux::setbit(board::g1)))
					return false;
				if (b.their(getSqAttackers(b, board::e1)) || b.their(getSqAttackers(b, board::f1)) || b.their(getSqAttackers(b, board::g1)))
					return false;
				return true;
			}
			else if (moveType == constants::QSCastle)
			{
				if (fromSq != board::e1 || toSq != board::c1)
					return false;
				if (!b.canCastleLong())
					return false;
				if (occ & (aux::setbit(board::d1) | aux::setbit(board::c1) | aux::setbit(board::b1)))
					return false;
				if (b.their(getSqAttackers(b, board::e1)) || b.their(getSqAttackers(b, board::d1)) || b.their(getSqAttackers(b, board::c1)))
					return false;
				return true;
			}
			else if (moveType == constants::QMove)
			{
				AttackMap kAttacks = kingAttacks(fromSq) & ~b.my(occ);
				if (!(kAttacks & aux::setbit(toSq)))
					return false;
				auto attackers = kingAttacks(toSq) & b.their(b.getKings());
				attackers |= knightAttacks(toSq) & b.their(b.getKnights());
				attackers |= hypqAllDiag(occ & ~aux::setbit(fromSq), toSq) & b.their(b.getDiagSliders());
				attackers |= hypqAllOrth(occ & ~aux::setbit(fromSq), toSq) & b.their(b.getOrthSliders());
				attackers |= pawnAttacks(toSq) & b.their(b.getPawns());
				if (attackers)
				{
					return false;
				}
				return true;
			}
			else
			{
				return false;
			}
			break;
		}
		default:
			return false;
		}
		Bitboard newOcc = (occ | aux::setbit(toSq)) & ~aux::setbit(fromSq);
		if (moveType == constants::enPCap)
			newOcc &= ~(aux::setbit(toSq) >> 8);
		Bitboard myKing = b.my(b.getKings());
		auto attackers = kingAttacks(myKing) & b.their(b.getKings());
		attackers |= knightAttacks(myKing) & b.their(b.getKnights());
		attackers |= hypqAllDiag(newOcc, myKing) & b.their(b.getDiagSliders());
		attackers |= hypqAllOrth(newOcc, myKing) & b.their(b.getOrthSliders());
		attackers |= pawnAttacks(myKing) & b.their(b.getPawns());
		attackers &= ~aux::setbit(toSq);
		
		return !attackers;
	}

	Bitboard getVertPinnedPieces(Bitboard occ, Bitboard king, Bitboard orth)
	{
		return KSFile(occ, king) & KSFile(occ, orth) & occ;
	}
	Bitboard getHorPinnedPieces(Bitboard occ, Bitboard king, Bitboard orth)
	{
		return KSRank(occ, king) & KSRank(occ, orth) & occ;
	}
	Bitboard getDiagPinnedPieces(Bitboard occ, Bitboard king, Bitboard diag)
	{
		return KSDiag(occ, king) & KSDiag(occ, diag) & occ;
	}
	Bitboard getAntiDiagPinnedPieces(Bitboard occ, Bitboard king, Bitboard diag)
	{
		return KSAntiDiag(occ, king) & KSAntiDiag(occ, diag) & occ;
	}
	Bitboard getAllPinnedPieces(Bitboard occ, Bitboard king, Bitboard diag, Bitboard orth)
	{
		Bitboard pinned = 0;
		AttackMap diagAttacks = KSDiag(occ, diag);
		AttackMap antiDiagAttacks = KSAntiDiag(occ, diag);
		AttackMap fileAttacks = KSFile(occ, orth);
		AttackMap rankAttacks = KSRank(occ, orth);
		pinned |= KSDiag(occ, king) & diagAttacks & occ;
		pinned |= KSAntiDiag(occ, king) & antiDiagAttacks & occ;
		pinned |= KSFile(occ, king) & fileAttacks & occ;
		pinned |= KSRank(occ, king) & rankAttacks & occ;
		return pinned;
	}
	AttackMap genEnemyAttacks(Bitboard occ, const board::QBB& b)
	{
		Bitboard attacks = KSAllDiag(occ, b.their(b.getDiagSliders()));
		attacks |= KSAllOrth(occ, b.their(b.getOrthSliders()));
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
		Bitboard between = KSDiag(occ, myKing) & KSDiag(occ, checkers);
		between |= KSAntiDiag(occ, myKing) & KSAntiDiag(occ, checkers);
		between |= KSFile(occ, myKing) & KSFile(occ, checkers);
		between |= KSRank(occ, myKing) & KSRank(occ, checkers);

		return between;
	}
	Bitboard isInCheck(const board::QBB& b)
	{
		Bitboard myKing = b.my(b.getKings());
		Bitboard occ = b.getOccupancy();

		Bitboard checkers = b.their(b.getDiagSliders()) & KSAllDiag(occ, myKing);
		checkers |= b.their(b.getOrthSliders()) & KSAllOrth(occ, myKing);
		checkers |= b.their(b.getKnights()) & knightAttacks(myKing);
		checkers |= b.their(b.getPawns()) & (pawnAttacksLeft(myKing) | pawnAttacksRight(myKing));

		return checkers;
	}
	Bitboard getSqAttackers(const board::QBB& b, board::square s)
	{
		AttackMap orth = KSAllOrth(b.getOccupancy(), s) & b.getOrthSliders();
		AttackMap diag = KSAllDiag(b.getOccupancy(), s) & b.getDiagSliders();
		AttackMap knight = knightAttacks(s) & b.getKnights();
		AttackMap kings = kingAttacks(s) & b.getKings();
		AttackMap pawns = pawnAttacks(s) & b.their(b.getPawns());
		pawns |= enemyPawnAttacks(s) & b.my(b.getPawns());
		return orth | diag | knight | kings | pawns;
	}
	Bitboard getSliderAttackers(Bitboard occ, board::square s, Bitboard diag, Bitboard orth)
	{
		orth &= KSAllOrth(occ, s);
		diag &= KSAllDiag(occ, s);
		return orth | diag;
	}
}