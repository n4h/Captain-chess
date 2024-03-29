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
#include <chrono>

#pragma intrinsic(_BitScanForward64)

#include "eval.hpp"
#include "board.hpp"
#include "moves.hpp"
#include "auxiliary.hpp"
#include "constants.hpp"
#include "types.hpp"

namespace eval
{
    using namespace aux;

    Eval computeMaterialValue(Bitboard bb, const std::array<Eval, 64>& PSQT)
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

    std::uint32_t getLVA(const board::QBB& b, Bitboard attackers, Bitboard& least)
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

    Eval getCaptureValue(const board::QBB& b, Move m)
    {
        Eval values[7] = { 0, 100, 300, 300, 500, 900, 10000 };
        if (board::getMoveInfo<constants::moveTypeMask>(m) == constants::enPCap)
        {
            return 100;
        }
        else
        {
            return values[b.getPieceCode(board::getMoveToSq(m))];
        }
    }

    // adapted from iterative SEE
    // https://www.chessprogramming.org/SEE_-_The_Swap_Algorithm
    Eval see(const board::QBB& b, Move m)
    {
        const board::square target = board::getMoveToSq(m);
        auto targettype = b.getPieceCode(target);
        const auto movetype = board::getMoveInfo<constants::moveTypeMask>(m);
        Bitboard attackers = moves::getAllAttackers(b, b.getOccupancy(), target);
        Bitboard attacker = aux::setbit(board::getMoveInfo<board::fromMask>(m));
        auto attackertype = b.getPieceCode(board::getMoveFromSq(m));

        Bitboard occ = b.getOccupancy();
        Bitboard orth = b.getOrthSliders();
        Bitboard diag = b.getDiagSliders();
        Bitboard side = b.side;

        std::array<Eval, 7> pieceval = {0, 100, 300, 300, 500, 900, 10000};
        std::array<Eval, 32> scores{};
        scores[0] = movetype == constants::enPCap ? pieceval[1] : pieceval[targettype];
        targettype = attackertype;
        attackers ^= attacker;
        occ ^= attacker;
        diag &= ~attacker;
        orth &= ~attacker;
        side = ~side;
        attackers |= moves::getSliderAttackers(occ, target, diag, orth);
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
            attackers |= moves::getSliderAttackers(occ, target, diag, orth);
            attackertype = getLVA(b, attackers & side, attacker);
        }
        while (--i)
            scores[i - 1] = std::min<Eval>(scores[i - 1], -scores[i]);
        return scores[0];
    }

    Eval Evaluator::applyAggressionBonus(std::size_t type, board::square enemyKingSq, Bitboard pieces) const
    {
        unsigned long index = 0;
        Eval e = 0;
        while (_BitScanForward64(&index, pieces))
        {
            pieces = _blsr_u64(pieces);
            e += aggressionBonus(board::square(index), enemyKingSq, closenessBonus(type));
        }
        return e;
    }

    Eval Evaluator::apply7thRankBonus(Bitboard rooks, Bitboard rank) const
    {
        return rookRank7Bonus() * (_popcnt64(rooks & rank));
    }

    Eval Evaluator::materialBalance(const board::QBB& b) const
    {
        Eval balance = 0;
        balance += piecevals(0) * (_popcnt64(b.my(b.getPawns())) - _popcnt64(b.their(b.getPawns())));
        balance += piecevals(1) * (_popcnt64(b.my(b.getKnights())) - _popcnt64(b.their(b.getKnights())));
        balance += piecevals(2) * (_popcnt64(b.my(b.getBishops())) - _popcnt64(b.their(b.getBishops())));
        balance += piecevals(3) * (_popcnt64(b.my(b.getRooks())) - _popcnt64(b.their(b.getRooks())));
        balance += piecevals(4) * (_popcnt64(b.my(b.getQueens())) - _popcnt64(b.their(b.getQueens())));
        return balance;
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

    bool Evaluator::isEndgame(const board::QBB& b) const
    {
        auto materialValue = 900 * (b.getQueens() & ~b.side);
        materialValue += 500 * (b.getRooks() & ~b.side);
        materialValue += 300 * (b.getBishops() & ~b.side);
        materialValue += 300 * (b.getKnights() & ~b.side);
        return materialValue < 1900;
    }

    Eval Evaluator::bishopOpenDiagonalBonus(Bitboard occ, Bitboard bishops) const
    {
        unsigned long index = 0;
        Eval e = 0;
        while (_BitScanForward64(&index, bishops))
        {
            bishops = _blsr_u64(bishops);
            auto square = board::square(index);
            if (board::diagMask(square) == moves::hypqDiag(occ, square))
            {
                e += bishopOpenDiagBonus();
            }
            if (board::antiDiagMask(square) == moves::hypqAntiDiag(occ, square))
            {
                e += bishopOpenDiagBonus();
            }
        }
        return e;
    }

    Eval Evaluator::rookOpenFileBonus(Bitboard pawns, Bitboard rooks) const
    {
        unsigned long index = 0;
        Eval e = 0;
        while (_BitScanForward64(&index, rooks))
        {
            rooks = _blsr_u64(rooks);
            auto square = board::square(index);
            e += ((board::fileMask(square) & pawns) == 0) * rookOpenFileBonus();
        }
        return e;
    }

    Eval Evaluator::evalPawns(const Bitboard myPawns, const Bitboard theirPawns) const noexcept
    {
        std::array<Bitboard, 8> files = {board::fileMask(board::a1), board::fileMask(board::b1), 
        board::fileMask(board::c1), board::fileMask(board::d1), board::fileMask(board::e1),
        board::fileMask(board::f1), board::fileMask(board::g1), board::fileMask(board::h1)};

        std::array<Bitboard, 8> neighboringFiles = {files[1], files[0] | files[2], 
        files[1] | files[3], files[2] | files[4], files[3] | files[5], files[4] | files[6], 
        files[5] | files[7], files[6]};

        Eval evaluation = 0;
        for (auto file : files)
        {
            evaluation -= doubledPawnPenalty() * (_popcnt64(myPawns & file) == 2);
            evaluation -= tripledPawnPenalty() * (_popcnt64(myPawns & file) > 2);
            evaluation += doubledPawnPenalty() * (_popcnt64(theirPawns & file) == 2);
            evaluation += tripledPawnPenalty() * (_popcnt64(theirPawns & file) > 2);
        }

        for (std::size_t i = 0; i != files.size(); ++i)
        {
            if (myPawns & files[i])
            {
                if (!(myPawns & neighboringFiles[i]))
                {
                    evaluation -= isolatedPawnPenalty();
                }
            }
            if (theirPawns & files[i])
            {
                if (!(theirPawns & neighboringFiles[i]))
                {
                    evaluation += isolatedPawnPenalty();
                }
            }
        }

        auto [myPassedPawns, theirPassedPawns] = detectPassedPawns(myPawns, theirPawns);

        aux::GetNextBit<board::square> ppSquare(myPassedPawns);
        while (ppSquare())
        {
            auto rank = aux::rank(ppSquare.next);
            evaluation += passedPawnBonus(rank);
        }

        ppSquare = aux::GetNextBit<board::square>{theirPassedPawns};
        while (ppSquare())
        {
            auto rank = aux::rank(aux::flip(ppSquare.next));
            evaluation -= passedPawnBonus(rank - 1);
        }

        auto [myBackwards, theirBackwards] = moves::backwardPawns(myPawns, theirPawns);

        evaluation -= backwardsPawnPenalty() * _popcnt64(myBackwards);
        evaluation += backwardsPawnPenalty() * _popcnt64(theirBackwards);

        auto myConnectedPawns = (aux::shiftLeftNoWrap(myPawns) & myPawns) | (aux::shiftRightNoWrap(myPawns) & myPawns);
        auto theirConnectedPawns = (aux::shiftLeftNoWrap(theirPawns) & theirPawns) | (aux::shiftRightNoWrap(theirPawns) & theirPawns);

        evaluation += connectedPawnBonus() * _popcnt64(myConnectedPawns);
        evaluation -= connectedPawnBonus() * _popcnt64(theirConnectedPawns);

        auto [myfileset, theirfileset] = moves::pawnFileset(myPawns, theirPawns);

        evaluation += pawnIslandPenalty() * _popcnt64(myfileset & (myfileset ^ (myfileset >> 1)));
        evaluation -= pawnIslandPenalty() * _popcnt64(theirfileset & (theirfileset ^ (theirfileset >> 1)));

        return evaluation;
    }

    Eval Evaluator::kingSafety(const board::QBB& b, board::square myKing, board::square theirKing) const
    {
        auto myKingFile = board::fileMask(myKing);
        auto theirKingFile = board::fileMask(theirKing);

        const auto pawns = b.getPawns();
        const auto myPawns = b.my(pawns);
        const auto theirPawns = b.their(pawns);

        double myScalingFactor = (_popcnt64(b.their(b.getKnights())) * piecevals(1)
            + _popcnt64(b.their(b.getBishops())) * piecevals(2)
            + _popcnt64(b.their(b.getRooks())) * piecevals(3)
            + _popcnt64(b.their(b.getQueens())) * piecevals(4));
        myScalingFactor /= 2 * (piecevals(1) + piecevals(2) + piecevals(3)) + piecevals(4);

        double theirScalingFactor = (_popcnt64(b.my(b.getKnights())) * piecevals(1)
            + _popcnt64(b.my(b.getBishops())) * piecevals(2)
            + _popcnt64(b.my(b.getRooks())) * piecevals(3)
            + _popcnt64(b.my(b.getQueens())) * piecevals(4));
        theirScalingFactor /= 2 * (piecevals(1) + piecevals(2) + piecevals(3)) + piecevals(4);


        Eval evaluation = 0;

        if (!(myKingFile & pawns))
            evaluation -= myScalingFactor * kingFileOpenPenalty();
        if (!(theirKingFile & pawns))
            evaluation += theirScalingFactor * kingFileOpenPenalty();


        if (!(aux::shiftLeftNoWrap(myKingFile) & pawns))
            evaluation -= myScalingFactor * kingAdjFileOpenPenalty();

        if (!(aux::shiftRightNoWrap(myKingFile) & pawns))
            evaluation -= 0.5 * myScalingFactor * kingAdjFileOpenPenalty();

        if (!(aux::shiftLeftNoWrap(theirKingFile) & pawns))
            evaluation += theirScalingFactor * kingAdjFileOpenPenalty();

        if (!(aux::shiftRightNoWrap(theirKingFile) & pawns))
            evaluation += 0.5 * theirScalingFactor * kingAdjFileOpenPenalty();

        auto pawnShield = moves::pawnAttacks(myKing) | moves::pawnMovesUp(myKing);

        if (pawnShield == (pawnShield & myPawns))
            evaluation += myScalingFactor * pawnShieldBonus();

        pawnShield = moves::enemyPawnAttacks(theirKing) | (moves::getBB(theirKing) >> 8);

        if (pawnShield == (pawnShield & theirPawns))
            evaluation -= theirScalingFactor * pawnShieldBonus();
        
        auto kingArea = moves::kingAttacks(myKing) | moves::getBB(myKing);

        auto [pAttackers, kAttackers, bAttackers, rAttackers, qAttackers, _]
            = moves::getTheirAttackers(b, kingArea | b.getOccupancy(), kingArea);
        (void)_;

        double attackerCountScale = (_popcnt64(pAttackers) + _popcnt64(kAttackers) + _popcnt64(bAttackers)
            + _popcnt64(rAttackers) + _popcnt64(qAttackers)) / 5.0;

        evaluation -= attackerCountScale * myScalingFactor * _popcnt64(pAttackers) * kingAttackerValue(0);
        evaluation -= attackerCountScale * myScalingFactor * _popcnt64(kAttackers) * kingAttackerValue(1);
        evaluation -= attackerCountScale * myScalingFactor * _popcnt64(bAttackers) * kingAttackerValue(2);
        evaluation -= attackerCountScale * myScalingFactor * _popcnt64(rAttackers) * kingAttackerValue(3);
        evaluation -= attackerCountScale * myScalingFactor * _popcnt64(qAttackers) * kingAttackerValue(4);

        kingArea = moves::kingAttacks(theirKing) | moves::getBB(theirKing);

        std::tie(pAttackers, kAttackers, bAttackers, rAttackers, qAttackers, std::ignore)
            = moves::getMyAttackers(b, kingArea | b.getOccupancy(), kingArea);

        attackerCountScale = (_popcnt64(pAttackers) + _popcnt64(kAttackers) + _popcnt64(bAttackers)
            + _popcnt64(rAttackers) + _popcnt64(qAttackers)) / 5.0;

        evaluation += attackerCountScale * theirScalingFactor * _popcnt64(pAttackers) * kingAttackerValue(0);
        evaluation += attackerCountScale * theirScalingFactor * _popcnt64(kAttackers) * kingAttackerValue(1);
        evaluation += attackerCountScale * theirScalingFactor * _popcnt64(bAttackers) * kingAttackerValue(2);
        evaluation += attackerCountScale * theirScalingFactor * _popcnt64(rAttackers) * kingAttackerValue(3);
        evaluation += attackerCountScale * theirScalingFactor * _popcnt64(qAttackers) * kingAttackerValue(4);
        
        return evaluation;
    }

    Eval Evaluator::operator()(const board::QBB& b) const
    {
        Eval evaluation = tempoBonus();

        const std::array<Bitboard, 12> pieces = {
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
        const auto occ = b.getOccupancy();
        enum {myPawns, myKnights, myBishops, myRooks, myQueens, myKing,
            theirPawns, theirKnights, theirBishops, theirRooks, theirQueens, theirKing,};

        for (std::size_t i = 0; i != 5; ++i)
        {
            evaluation += piecevals(i) * (_popcnt64(pieces[i]) - _popcnt64(pieces[i + 6]));
        }

        const auto myKingSq = board::square(_tzcnt_u64(pieces[myKing]));
        const auto oppKingSq = board::square(_tzcnt_u64(pieces[theirKing]));

        for (std::size_t i = 0; i != 12; ++i)
        {
            evaluation += (i < 6 ? 1 : -1) * applyAggressionBonus(i, i < 6 ? oppKingSq : myKingSq, pieces[i]);
        }

        const auto pawnCount = _popcnt64(pieces[myPawns] | pieces[theirPawns]);

        evaluation += _popcnt64(pieces[myKnights]) * knightPawnCountPenalty(pawnCount);
        evaluation -= _popcnt64(pieces[theirKnights]) * knightPawnCountPenalty(pawnCount);
        evaluation += _popcnt64(pieces[myRooks]) * rookPawnCountBonus(pawnCount);
        evaluation -= _popcnt64(pieces[theirRooks]) * rookPawnCountBonus(pawnCount);

        auto myConnectRookCnt = _popcnt64(moves::KSRank(occ, pieces[myRooks]) & pieces[myRooks])/2;
        auto myDoubleRookCnt = _popcnt64(moves::KSFile(occ, pieces[myRooks]) & pieces[myRooks]) / 2;
        auto theirConnectRookCnt = _popcnt64(moves::KSRank(occ, pieces[theirRooks]) & pieces[theirRooks]) / 2;
        auto theirDoubleRookCnt = _popcnt64(moves::KSFile(occ, pieces[theirRooks]) & pieces[theirRooks]) / 2;

        evaluation += myConnectRookCnt * connectedRookBonus();
        evaluation -= theirConnectRookCnt * connectedRookBonus();
        evaluation += myDoubleRookCnt * doubledRookBonus();
        evaluation -= theirDoubleRookCnt * doubledRookBonus();

        const auto [myPassed, theirPassed] = detectPassedPawns(pieces[myPawns], pieces[theirPawns]);

        const auto myRooksBehind = moves::KSSouth(myPassed, myPassed) & pieces[myRooks];
        const auto theirRooksBehind = moves::KSNorth(theirPassed, theirPassed) & pieces[theirRooks];

        evaluation += rookBehindPassedP() * _popcnt64(myRooksBehind);
        evaluation -= rookBehindPassedP() * _popcnt64(theirRooksBehind);


        aux::GetNextBit<Bitboard> mobility(pieces[myKnights]);
        while (mobility())
        {
            moves::AttackMap moves = moves::knightAttacks(mobility.next);
            moves &= ~(moves::enemyPawnAttacks(pieces[theirPawns]) | pieces[myKing] | pieces[myPawns]);
            evaluation += knightMobility()*_popcnt64(moves);
            if (!moves::getMyAttackersBB(b, occ, mobility.next))
                evaluation += undefendedKnightPenalty();
        }
        mobility = GetNextBit<Bitboard>{ pieces[theirKnights] };
        while (mobility())
        {
            moves::AttackMap moves = moves::knightAttacks(mobility.next);
            moves &= ~(moves::pawnAttacks(pieces[myPawns]) | pieces[theirKing] | pieces[theirPawns]);
            evaluation -= knightMobility()*_popcnt64(moves);
            if (!moves::getTheirAttackersBB(b, occ, mobility.next))
                evaluation -= undefendedKnightPenalty();
        }
        mobility = GetNextBit<Bitboard>{ pieces[myBishops] };
        while (mobility())
        {
            moves::AttackMap moves = moves::hypqAllDiag(occ & ~pieces[myQueens], mobility.next);
            moves &= ~(moves::enemyPawnAttacks(pieces[theirPawns]) | pieces[myKing] | pieces[myPawns]);
            evaluation += bishopMobility()*_popcnt64(moves);
            if (!moves::getMyAttackersBB(b, occ, mobility.next))
                evaluation += undefendedBishopPenalty();
        }
        mobility = GetNextBit<Bitboard>{ pieces[theirBishops] };
        while (mobility())
        {
            moves::AttackMap moves = moves::hypqAllDiag(occ & ~pieces[theirQueens], mobility.next);
            moves &= ~(moves::pawnAttacks(pieces[myPawns]) | pieces[theirKing] | pieces[theirPawns]);
            evaluation -= bishopMobility()*_popcnt64(moves);
            if (!moves::getTheirAttackersBB(b, occ, mobility.next))
                evaluation -= undefendedBishopPenalty();
        }
        mobility = GetNextBit<Bitboard>{ pieces[myRooks] };
        while (mobility())
        {
            moves::AttackMap moves = moves::hypqRank(occ & ~(pieces[myQueens] | pieces[myRooks]), mobility.next);
            moves &= ~(moves::enemyPawnAttacks(pieces[theirPawns]) | pieces[myKing] | pieces[myPawns]);
            evaluation += rookHorMobility()*_popcnt64(moves);
            moves = moves::hypqFile(occ & ~(pieces[myQueens] | pieces[myRooks]), mobility.next);
            moves &= ~(moves::enemyPawnAttacks(pieces[theirPawns]) | pieces[myKing] | pieces[myPawns]);
            evaluation += rookVertMobility()*_popcnt64(moves);
        }
        mobility = GetNextBit<Bitboard>{ pieces[theirRooks] };
        while (mobility())
        {
            moves::AttackMap moves = moves::hypqRank(occ & ~(pieces[theirQueens] | pieces[theirRooks]), mobility.next);
            moves &= ~(moves::pawnAttacks(pieces[myPawns]) | pieces[theirKing] | pieces[theirPawns]);
            evaluation -= rookHorMobility()*_popcnt64(moves);
            moves = moves::hypqFile(occ & ~(pieces[theirQueens] | pieces[theirRooks]), mobility.next);
            moves &= ~(moves::pawnAttacks(pieces[myPawns]) | pieces[theirKing] | pieces[theirPawns]);
            evaluation -= rookVertMobility()*_popcnt64(moves);
        }

        evaluation += evalPawns(pieces[myPawns], pieces[theirPawns]); // TODO store this in pawn hash

        if (isEndgame(b))
        {
            evaluation += kingCentralization(myKingSq);
            evaluation -= kingCentralization(oppKingSq);

            auto expansion = moves::kingAttacks(myPassed) | myPassed;
            unsigned distance;

            if (expansion)
            {
                for (distance = 1; !(expansion & pieces[myKing]); ++distance)
                    expansion |= moves::kingAttacks(expansion);
                evaluation -= kingPassedPDistPenalty() * distance;
            }

            expansion = moves::kingAttacks(theirPassed) | theirPassed;

            if (expansion)
            {
                for (distance = 1; !(expansion & pieces[theirKing]); ++distance)
                    expansion |= moves::kingAttacks(expansion);
                evaluation += kingPassedPDistPenalty() * distance;
            }
        }
        else
        {
            evaluation += kingSafety(b, myKingSq, oppKingSq);
        }

        evaluation += this->bishopPairBonus((pieces[2] & constants::whiteSquares) && (pieces[2] & constants::blackSquares));
        evaluation -= this->bishopPairBonus((pieces[8] & constants::whiteSquares) && (pieces[8] & constants::blackSquares));

        evaluation += this->applyKnightOutPostBonus<OutpostType::MyOutpost>(pieces[1], pieces[0], pieces[6]);
        evaluation -= this->applyKnightOutPostBonus<OutpostType::OppOutpost>(pieces[7], pieces[0], pieces[6]);

        evaluation += this->rookOpenFileBonus(pieces[myPawns] | pieces[theirPawns], pieces[myRooks]);
        evaluation -= this->rookOpenFileBonus(pieces[myPawns] | pieces[theirPawns], pieces[theirRooks]);

        evaluation += this->apply7thRankBonus(pieces[myRooks], board::rankMask(board::a7));
        evaluation -= this->apply7thRankBonus(pieces[theirRooks], board::rankMask(board::a2));

        return evaluation;
    }

    std::string Evaluator::asString() const
    {
        std::ostringstream oss;
        auto printArray = [&](const std::string& name, const auto& array)
        {
            oss << name << "={";
            for (std::size_t j = 0; const auto& i : array)
            {
                oss << "/* " << j << " */ " << i << ",";
                ++j;
            }
            oss << "}" << '\n';
        };

        oss << '\n';
        auto now = std::chrono::current_zone()->to_local(std::chrono::system_clock::now());
        oss << "Tuning completed date/time: " << now << '\n' << '\n';

        printArray("evalTerms", evalTerms);

        return oss.str();

    }

    void EvaluatorGeneticOps::mutate(Evaluator& e, double mutation_rate)
    {
        std::bernoulli_distribution doMutate(mutation_rate);
        std::uniform_int_distribution positionalBonus(-30, 30);
        std::uniform_int_distribution ZeroTo8(0, 8);

        auto mutate = [&](auto& mutator, int p = 0) {
            return doMutate(aux::seed) ? mutator(aux::seed) : p;
        };

        for (auto& i : e.evalTerms)
        {
            i += mutate(positionalBonus);
        }


        for (std::size_t i = 5; i != 9; ++i)
        {
            e.evalTerms[i] = mutate(ZeroTo8, e.evalTerms[i]);
        }

        for (std::size_t i = 18; i != 42; i += 2)
        {
            e.evalTerms[i] = mutate(ZeroTo8, e.evalTerms[i]);
        }

    }

    Evaluator EvaluatorGeneticOps::crossover(const Evaluator& e1, const Evaluator& e2)
    {
        Evaluator e;
        std::bernoulli_distribution which_e;
        auto parent = [&e1, &e2, &which_e]() -> const Evaluator& {
            return which_e(aux::seed) ? e1 : e2;
        };

        for (std::size_t i = 0; i != e.evalTerms.size(); ++i)
        {
            e.evalTerms[i] = parent().evalTerms[i];
        }

        return e;
    }
}