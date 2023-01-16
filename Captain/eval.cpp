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

	std::int16_t computeMaterialValue(board::Bitboard bb, const std::array<std::int16_t, 64>& PSQT)
	{
		std::int16_t mval = 0;

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

	std::int16_t mvvlva(const board::QBB& b, board::Move m)
	{
		std::int16_t values[6] = {100, 300, 300, 500, 900, 10000}; //PNBRQK
		board::square from = static_cast<board::square>(board::getMoveInfo<constants::fromMask>(m));
		board::square to = static_cast<board::square>(board::getMoveInfo<constants::toMask>(m));
		return values[(b.getPieceType(to) >> 1) - 1] - values[(b.getPieceType(from) >> 1) - 1];
	}

	// adapted from iterative SEE
	// https://www.chessprogramming.org/SEE_-_The_Swap_Algorithm
	std::int16_t see(const board::QBB& b, board::Move m)
	{
		const board::square target = board::getMoveToSq(m);
		auto targettype = (b.getPieceType(target) >> 1) - 1;

		board::Bitboard attackers = movegen::getSqAttackers(b, target);
		board::Bitboard attacker = aux::setbit(board::getMoveInfo<board::fromMask>(m));
		auto attackertype = b.getPieceType(board::getMoveFromSq(m));

		board::Bitboard occ = b.getOccupancy();
		board::Bitboard orth = b.getOrthSliders();
		board::Bitboard diag = b.getDiagSliders();
		board::Bitboard side = b.side;

		std::array<std::int16_t, 6> pieceval = {100, 300, 300, 500, 900, 10000};
		std::array<std::int16_t, 32> scores;

		scores[0] = pieceval[targettype];
		targettype = attackertype;
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
			targettype = attackertype;
			attackers ^= attacker;
			occ ^= attacker;
			diag &= ~attacker;
			orth &= ~attacker;
			side = ~side;
			attackers |= movegen::getSliderAttackers(occ, target, diag, orth);
			attackertype = getLVA(b, attackers & side, attacker);
		}
		while (--i)
			scores[i - 1] = std::min(scores[i - 1], (std::int16_t)-scores[i]);
		return scores[0];
	}

	std::int16_t evalCapture(const board::QBB& b, board::Move m)
	{
		std::int16_t mvvlvaScore = mvvlva(b, m);
		return mvvlvaScore >= 0 ? mvvlvaScore : see(b, m);
	}

	std::int16_t evaluate(const board::QBB& b)
	{
		std::int16_t totalW = 0;

		totalW += computeMaterialValue(b.my(b.getPawns()), PSQTpawnw);
		totalW += computeMaterialValue(b.my(b.getKnights()), PSQTknight);
		totalW += computeMaterialValue(b.my(b.getBishops()), PSQTbishop);
		totalW += computeMaterialValue(b.my(b.getRooks()), PSQTrookw);
		totalW += computeMaterialValue(b.my(b.getQueens()), PSQTqueen);

		std::int16_t totalB = 0;

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

		std::int16_t eval = totalW - totalB;

		return eval;
	}
}