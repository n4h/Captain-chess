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

#include <intrin.h>
#include <algorithm>

#pragma intrinsic(_BitScanForward64)

#include "eval.hpp"
#include "board.hpp"
#include "movegen.hpp"
#include "auxiliary.hpp"
#include "constants.hpp"

namespace eval
{
	using namespace aux;
	using namespace constants;

	Eval computeMaterialValue(board::Bitboard bb, const std::array<Eval, 64>& PSQT)
	{
		Eval mval = 0;

		unsigned long index;

		while (_BitScanForward64(&index, bb))
		{
			bb ^= setbit(index);

			mval += PSQT[index];
		}
		return mval;
	}

	std::uint32_t getLVA(const board::QBB& b, board::Bitboard attackers, board::Bitboard& least)
	{
		// TODO rank promoting pawns higher
		if (attackers & b.getPawns())
		{
			least = _blsi_u64(attackers & b.getPawns());
			return constants::pawnCode;
		}
		if (attackers & b.getKnights())
		{
			least = _blsi_u64(attackers & b.getKnights());
			return constants::knightCode;
		}
		if (attackers & b.getBishops())
		{
			least = _blsi_u64(attackers & b.getBishops());
			return constants::bishopCode;
		}
		if (attackers & b.getRooks())
		{
			least = _blsi_u64(attackers & b.getRooks());
			return constants::rookCode;
		}
		if (attackers & b.getQueens())
		{
			least = _blsi_u64(attackers & b.getQueens());
			return constants::queenCode;
		}
		if (attackers & b.getKings())
		{
			least = _blsi_u64(attackers & b.getKings());
			return constants::kingCode;
		}
		least = 0;
		return 0;
	}

	Eval getCaptureValue(const board::QBB& b, board::Move m)
	{
		Eval values[6] = { 100, 300, 300, 500, 900, 10000 };
		if (board::getMoveInfo<constants::moveTypeMask>(m) == constants::enPCap)
		{
			return 100;
		}
		else
		{
			return values[(b.getPieceType(board::getMoveToSq(m)) >> 1) - 1];
		}
	}

	Eval mvvlva(const board::QBB& b, board::Move m)
	{
		Eval values[6] = {100, 300, 300, 500, 900, 10000}; //PNBRQK
		board::square from = board::getMoveFromSq(m);
		board::square to = board::getMoveToSq(m);
		if (board::getMoveInfo<constants::moveTypeMask>(m) == constants::enPCap)
			return 0;
		return values[(b.getPieceType(to) >> 1) - 1] - values[(b.getPieceType(from) >> 1) - 1];
	}

	// adapted from iterative SEE
	// https://www.chessprogramming.org/SEE_-_The_Swap_Algorithm
	Eval see(const board::QBB& b, board::Move m)
	{
		const board::square target = board::getMoveToSq(m);
		auto targettype = (b.getPieceType(target) >> 1) - 1;
		const auto movetype = board::getMoveInfo<moveTypeMask>(m);
		board::Bitboard attackers = movegen::getSqAttackers(b, target);
		board::Bitboard attacker = aux::setbit(board::getMoveInfo<board::fromMask>(m));
		auto attackertype = b.getPieceType(board::getMoveFromSq(m)) >> 1;

		board::Bitboard occ = b.getOccupancy();
		board::Bitboard orth = b.getOrthSliders();
		board::Bitboard diag = b.getDiagSliders();
		board::Bitboard side = b.side;

		std::array<Eval, 6> pieceval = {100, 300, 300, 500, 900, 10000};
		std::array<Eval, 32> scores;
		scores[0] = movetype == enPCap ? pieceval[0] : pieceval[targettype];
		targettype = attackertype - 1;
		attackers ^= attacker;
		occ ^= attacker;
		diag &= ~attacker;
		orth &= ~attacker;
		side = ~side;
		attackers |= movegen::getSliderAttackers(occ, target, diag, orth);
		attackertype = getLVA(b, attackers & side, attacker);
		std::size_t i = 1;
		for (; i != 32 && attackertype; ++i)
		{
			scores[i] = pieceval[targettype] - scores[i - 1];
			if (scores[i] < 0) break;
			targettype = attackertype - 1;
			attackers ^= attacker;
			occ ^= attacker;
			diag &= ~attacker;
			orth &= ~attacker;
			side = ~side;
			attackers |= movegen::getSliderAttackers(occ, target, diag, orth);
			attackertype = getLVA(b, attackers & side, attacker);
		}
		while (--i)
			scores[i - 1] = std::min<Eval>(scores[i - 1], -scores[i]);
		return scores[0];
	}

	Eval evalCapture(const board::QBB& b, board::Move m)
	{
		Eval mvvlvaScore = mvvlva(b, m);
		return mvvlvaScore >= 0 ? mvvlvaScore : see(b, m);
	}

	Eval evaluate(const board::QBB& b)
	{
		//board::Bitboard occ = b.getOccupancy();
		Eval totalW = 0;

		totalW += computeMaterialValue(b.my(b.getPawns()), PSQTpawnw);
		totalW += computeMaterialValue(b.my(b.getKnights()), PSQTknight);
		totalW += computeMaterialValue(b.my(b.getBishops()), PSQTbishop);
		totalW += computeMaterialValue(b.my(b.getRooks()), PSQTrookw);
		totalW += computeMaterialValue(b.my(b.getQueens()), PSQTqueen);

		Eval totalB = 0;

		totalB += computeMaterialValue(b.their(b.getPawns()), PSQTpawnb);
		totalB += computeMaterialValue(b.their(b.getKnights()), PSQTknight);
		totalB += computeMaterialValue(b.their(b.getBishops()), PSQTbishop);
		totalB += computeMaterialValue(b.their(b.getRooks()), PSQTrookb);
		totalB += computeMaterialValue(b.their(b.getQueens()), PSQTqueen);

		if (totalW + totalB > 3000) // not in endgame
		{
			totalW += computeMaterialValue(b.my(b.getKings()), PSQTking);
			totalB += computeMaterialValue(b.their(b.getKings()), PSQTking);
		}
		else
		{
			totalW += computeMaterialValue(b.my(b.getKings()), PSQTkingEnd);
			totalB += computeMaterialValue(b.their(b.getKings()), PSQTkingEnd);
		}

		auto myBishops = b.my(b.getBishops());
		const auto myPawns = b.my(b.getPawns());
		auto oppBishops = b.their(b.getBishops());
		const auto oppPawns = b.their(b.getPawns());

		totalW += bishopPairBonus((myBishops & whiteSquares) && (myBishops & blackSquares));
		totalB += bishopPairBonus((oppBishops & whiteSquares) && (oppBishops & blackSquares));

		totalW -= pawnCountBishopPenalty(_popcnt64(myPawns & whiteSquares) * static_cast<bool>(myBishops & whiteSquares));
		totalW -= pawnCountBishopPenalty(_popcnt64(myPawns & blackSquares) * static_cast<bool>(myBishops & blackSquares));
		totalB -= pawnCountBishopPenalty(_popcnt64(oppPawns & whiteSquares) * static_cast<bool>(oppBishops & whiteSquares));
		totalB -= pawnCountBishopPenalty(_popcnt64(oppPawns & blackSquares) * static_cast<bool>(oppBishops & blackSquares));

		auto lsb = 0ULL;
		while ((lsb = _blsi_u64(myBishops)))
		{
			myBishops = _blsr_u64(myBishops);
			totalW += bishopOpenDiagonalBonus((board::multiAntiDiagMask(lsb) & b.getPawns()) == 0);
			totalW += bishopOpenDiagonalBonus((board::multiDiagMask(lsb) & b.getPawns()) == 0);
		}
		while ((lsb = _blsi_u64(oppBishops)))
		{
			oppBishops = _blsr_u64(oppBishops);
			totalB += bishopOpenDiagonalBonus((board::multiAntiDiagMask(lsb) & b.getPawns()) == 0);
			totalB += bishopOpenDiagonalBonus((board::multiDiagMask(lsb) & b.getPawns()) == 0);
		}

		auto myKnights = b.my(b.getKnights());
		auto oppKnights = b.their(b.getKnights());
		const auto myKing = b.my(b.getKings());
		const auto oppKing = b.their(b.getKings());
		unsigned long index = 0;
		_BitScanForward64(&index, myKing);
		const auto myKingSq = static_cast<board::square>(index);
		_BitScanForward64(&index, oppKing);
		const auto oppKingSq = static_cast<board::square>(index);

		while (_BitScanForward64(&index, myKnights))
		{
			board::square knightsq = static_cast<board::square>(index);
			myKnights = _blsr_u64(myKnights);
			totalW += aggressionBonus(knightsq, oppKingSq, 4, 5);
			totalW += knightOutpostBonus<OutpostType::MyOutpost>(knightsq, b.my(b.getPawns()), b.their(b.getPawns()));
		}
		while (_BitScanForward64(&index, oppKnights))
		{
			board::square knightsq = static_cast<board::square>(index);
			oppKnights = _blsr_u64(oppKnights);
			totalB += aggressionBonus(knightsq, myKingSq, 4, 5);
			totalB += knightOutpostBonus<OutpostType::OppOutpost>(knightsq, b.my(b.getPawns()), b.their(b.getPawns()));
		}

		auto myQueens = b.my(b.getQueens());
		auto oppQueens = b.their(b.getQueens());
		
		while (_BitScanForward64(&index, myQueens))
		{
			board::square queensq = static_cast<board::square>(index);
			myQueens = _blsr_u64(myQueens);
			totalW += aggressionBonus(queensq, oppKingSq, 3, 5);
		}
		while (_BitScanForward64(&index, oppQueens))
		{
			board::square queensq = static_cast<board::square>(index);
			oppQueens = _blsr_u64(oppQueens);
			totalB += aggressionBonus(queensq, myKingSq, 3, 5);
		}

		auto myRooks = b.my(b.getRooks());
		auto oppRooks = b.their(b.getRooks());

		while (_BitScanForward64(&index, myRooks))
		{
			auto rookbit = setbit(index);
			myRooks = _blsr_u64(myRooks);
			totalW += aggressionBonus(board::square(index), oppKingSq, 7, 5);
			totalW += rookOpenFileBonus((board::multiFileMask(rookbit) & b.getPawns()) == 0);
		}
		while (_BitScanForward64(&index, oppRooks))
		{
			auto rookbit = setbit(index);
			oppRooks = _blsr_u64(oppRooks);
			totalB += aggressionBonus(board::square(index), myKingSq, 7, 5);
			totalB += rookOpenFileBonus((board::multiFileMask(rookbit) & b.getPawns()) == 0);
		}
		Eval eval = totalW - totalB;

		return eval;
	}
}