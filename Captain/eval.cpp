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

    Eval Evaluator::applyAggressionBonus(std::size_t type, board::square enemyKingSq, board::Bitboard pieces) const
    {
        unsigned long index = 0;
        Eval e = 0;
        while (_BitScanForward64(&index, pieces))
        {
            pieces = _blsr_u64(pieces);
            e += aggressionBonus(board::square(index), enemyKingSq, _aggressionBonuses[type]);
        }
        return e;
    }

    unsigned Evaluator::totalMaterialValue(const board::QBB& b) const
    {
        unsigned materialVal = 0;
        unsigned piecevalues[6] = { 100, 300, 300, 500, 900, 0 };
        GetNextBit<board::square> currSquare(b.getOccupancy());
        while (currSquare())
        {
            auto sq = currSquare.next;
            auto pieceType = b.getPieceCodeIdx(sq);
            materialVal += piecevalues[pieceType];
        }
        return materialVal;
    }

    Eval Evaluator::bishopOpenDiagonalBonus(board::Bitboard occ, board::Bitboard bishops) const
    {
        unsigned long index = 0;
        Eval e = 0;
        while (_BitScanForward64(&index, bishops))
        {
            bishops = _blsr_u64(bishops);
            auto square = board::square(index);
            if (board::diagMask(square) == movegen::hypqDiag(occ, square))
            {
                e += _bishopOpenDiagonalBonus;
            }
            if (board::antiDiagMask(square) == movegen::hypqAntiDiag(occ, square))
            {
                e += _bishopOpenDiagonalBonus;
            }
        }
        return e;
    }

    Eval Evaluator::rookOpenFileBonus(board::Bitboard pawns, board::Bitboard rooks) const
    {
        unsigned long index = 0;
        Eval e = 0;
        while (_BitScanForward64(&index, rooks))
        {
            rooks = _blsr_u64(rooks);
            auto square = board::square(index);
            e += ((board::fileMask(square) & pawns) == 0) * _rookOpenFileBonus;
        }
        return e;
    }

    Eval Evaluator::operator()(const board::QBB& b) const
    {
        Eval evaluation = 0;
        unsigned materialVal = this->totalMaterialValue(b);

        const std::array<board::Bitboard, 12> pieces = {
            b.my(b.getPawns()),
            b.my(b.getKnights()),
            b.my(b.getBishops()),
            b.my(b.getRooks()),
            b.my(b.getQueens()),
            b.my(b.getKings()),
            b.their(b.getPawns()),
            b.their(b.getKnights()),
            b.their(b.getBishops()),
            b.their(b.getRooks()),
            b.their(b.getQueens()),
            b.their(b.getKings()),
        };

        if (materialVal > this->_openToMid)
        {
            for (std::size_t i = 0; i != 12; ++i)
            {
                evaluation += (i < 6 ? 1 : -1) * computeMaterialValue(pieces[i], this->_openingPSQT[i]);
            }
        }
        else if (materialVal > this->_midToEnd)
        {
            for (std::size_t i = 0; i != 12; ++i)
            {
                evaluation += (i < 6 ? 1 : -1) * computeMaterialValue(pieces[i], this->_midPSQT[i]);
            }
        }
        else
        {
            for (std::size_t i = 0; i != 12; ++i)
            {
                evaluation += (i < 6 ? 1 : -1) * computeMaterialValue(pieces[i], this->_endPSQT[i]);
            }
        }

        const auto myKingSq = board::square(_tzcnt_u64(pieces[5]));
        const auto oppKingSq = board::square(_tzcnt_u64(pieces[11]));

        for (std::size_t i = 0; i != 12; ++i)
        {
            evaluation += (i < 6 ? 1 : -1) * applyAggressionBonus(i, i < 6 ? oppKingSq : myKingSq, pieces[i]);
        }

        auto myPawnsWhite = whiteSquares & pieces[0];
        auto myPawnsBlack = blackSquares & pieces[0];
        auto oppPawnsWhite = whiteSquares & pieces[6];
        auto oppPawnsBlack = blackSquares & pieces[6];
        evaluation -= this->pawnCountBishopPenalty(_popcnt64(myPawnsWhite), pieces[2]);
        evaluation -= this->pawnCountBishopPenalty(_popcnt64(myPawnsBlack), pieces[2]);
        evaluation += this->pawnCountBishopPenalty(_popcnt64(oppPawnsWhite), pieces[8]);
        evaluation += this->pawnCountBishopPenalty(_popcnt64(oppPawnsBlack), pieces[8]);

        evaluation += this->bishopOpenDiagonalBonus(b.getOccupancy(), pieces[2]);
        evaluation -= this->bishopOpenDiagonalBonus(b.getOccupancy(), pieces[8]);

        evaluation += this->bishopPairBonus((pieces[2] & whiteSquares) && (pieces[2] & blackSquares));
        evaluation -= this->bishopPairBonus((pieces[8] & whiteSquares) && (pieces[8] & blackSquares));

        evaluation += this->rookOpenFileBonus(b.getPawns(), pieces[3]);
        evaluation -= this->rookOpenFileBonus(b.getPawns(), pieces[9]);

        evaluation += this->applyKnightOutPostBonus<OutpostType::MyOutpost>(pieces[1], pieces[0], pieces[6]);
        evaluation -= this->applyKnightOutPostBonus<OutpostType::OppOutpost>(pieces[7], pieces[0], pieces[6]);

        return evaluation;
    }

    const Evaluator& Evaluator::mutate(bool randomize)
    {
        std::mt19937_64 urbg(aux::seed);
        std::bernoulli_distribution doMutate(randomize ? 0.85 : 1.0/2500.0);
        std::uniform_int_distribution positionalBonus(-50, 50);
        std::uniform_int_distribution gamePhase(-500, 500);
        std::uniform_int_distribution PSQT(-300, 300);
        std::uniform_int_distribution ZeroTo8(0, 8);

        auto mutate = [&](auto& mutator, int p = 0) {
            return doMutate(urbg) ? mutator(urbg) : p;
        };

        for (std::size_t i = 0; i != 12; ++i)
        {
            for (std::size_t j = 0; j != 64; ++j)
            {
                _openingPSQT[i][j] += mutate(PSQT);
                _midPSQT[i][j] += mutate(PSQT);
                _endPSQT[i][j] += mutate(PSQT);
            }
            _aggressionBonuses[i].first = mutate(ZeroTo8, _aggressionBonuses[i].first);
            _aggressionBonuses[i].second += mutate(positionalBonus);
        }

        _openToMid += mutate(gamePhase);
        _midToEnd += mutate(gamePhase);

        _pawnBishopPenalty.first = mutate(ZeroTo8, _pawnBishopPenalty.first);
        _pawnBishopPenalty.second += mutate(positionalBonus);

        _bishopOpenDiagonalBonus += mutate(positionalBonus);
        _rookOpenFileBonus += mutate(positionalBonus);
        _bishopPairBonus += mutate(positionalBonus);

        _knightOutpostBonus.first += mutate(positionalBonus);
        _knightOutpostBonus.second += mutate(positionalBonus);

        return *this;
    }
    std::string Evaluator::asString() const
    {
        std::ostringstream oss;

        std::array<std::string, 12> PSQTNames= {"wpawns", "wknights", "wbishops", "wrooks", "wqueens", "wking",
        "bpawns", "bknights", "bbishops", "brooks", "bqueens", "bking" };

        auto printPSQTName = [&](std::size_t i) {oss << PSQTNames[i] << "="; };

        auto printArrayVal = [&](std::size_t sq, Eval e) {
            if (sq % 8 == 0)
                oss << '\n';
            oss << e << ",";
        };


        auto printPSQTSet = [&](const auto& set) {
            for (std::size_t n = 0; n != 12; ++n)
            {
                oss << '\n';
                printPSQTName(n);
                oss << "{";
                for (std::size_t m = 0; m != 64; ++m)
                {
                    printArrayVal(m, set[n][m]);
                }
                oss << "}";
                oss << '\n';
            }
        };

        oss << "OPENING PSQTs" << '\n';
        printPSQTSet(_openingPSQT);

        oss << '\n';
        oss << "Transition point: " << _openToMid;
        oss << '\n';

        oss << "MIDGAME PSQTs" << '\n';
        printPSQTSet(_midPSQT);

        oss << '\n';
        oss << "Transition point: " << _midToEnd;
        oss << '\n';

        oss << "ENDGAME PSQTs" << '\n';
        printPSQTSet(_endPSQT);

        oss << '\n';
        oss << "aggression";
        oss << '\n';

        for (const auto& [dist, bonus] : _aggressionBonuses)
        {
            oss << "<" << dist << "," << bonus << ">";
        }

        oss << '\n';
        oss << "pawnbishoppenalty";
        oss << '\n';

        oss << "<" << _pawnBishopPenalty.first << "," << _pawnBishopPenalty.second << ">";

        oss << '\n';
        oss << "opendiagonal,openfile,pair";
        oss << '\n';

        oss << "<" << _bishopOpenDiagonalBonus << "," << _rookOpenFileBonus << "," << _bishopPairBonus << ">";

        oss << '\n';
        oss << "knightoutpost";
        oss << '\n';

        oss << "<" << _knightOutpostBonus.first << "," << _knightOutpostBonus.second << ">";

        oss << '\n';
        return oss.str();

    }
}