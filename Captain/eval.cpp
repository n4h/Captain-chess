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

namespace eval
{
    using namespace aux;

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
        const auto movetype = board::getMoveInfo<constants::moveTypeMask>(m);
        board::Bitboard attackers = moves::getSqAttackers(b, target);
        board::Bitboard attacker = aux::setbit(board::getMoveInfo<board::fromMask>(m));
        auto attackertype = b.getPieceType(board::getMoveFromSq(m)) >> 1;

        board::Bitboard occ = b.getOccupancy();
        board::Bitboard orth = b.getOrthSliders();
        board::Bitboard diag = b.getDiagSliders();
        board::Bitboard side = b.side;

        std::array<Eval, 6> pieceval = {100, 300, 300, 500, 900, 10000};
        std::array<Eval, 32> scores{};
        scores[0] = movetype == constants::enPCap ? pieceval[0] : pieceval[targettype];
        targettype = attackertype - 1;
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
            targettype = attackertype - 1;
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

    Eval evalCapture(const board::QBB& b, board::Move m)
    {
        Eval mvvlvaScore = mvvlva(b, m);
        return mvvlvaScore >= 0 ? mvvlvaScore : see(b, m);
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

    Eval Evaluator::apply7thRankBonus(board::Bitboard rooks, board::Bitboard rank) const
    {
        return rook7thRankBonus * (_popcnt64(rooks & rank));
    }

    Eval Evaluator::materialBalance(const board::QBB& b) const
    {
        Eval balance = 0;
        balance += piecevals[0] * (_popcnt64(b.my(b.getPawns())) - _popcnt64(b.their(b.getPawns())));
        balance += piecevals[1] * (_popcnt64(b.my(b.getKnights())) - _popcnt64(b.their(b.getKnights())));
        balance += piecevals[2] * (_popcnt64(b.my(b.getBishops())) - _popcnt64(b.their(b.getBishops())));
        balance += piecevals[3] * (_popcnt64(b.my(b.getRooks())) - _popcnt64(b.their(b.getRooks())));
        balance += piecevals[4] * (_popcnt64(b.my(b.getQueens())) - _popcnt64(b.their(b.getQueens())));
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

    Eval Evaluator::bishopOpenDiagonalBonus(board::Bitboard occ, board::Bitboard bishops) const
    {
        unsigned long index = 0;
        Eval e = 0;
        while (_BitScanForward64(&index, bishops))
        {
            bishops = _blsr_u64(bishops);
            auto square = board::square(index);
            if (board::diagMask(square) == moves::hypqDiag(occ, square))
            {
                e += _bishopOpenDiagonalBonus;
            }
            if (board::antiDiagMask(square) == moves::hypqAntiDiag(occ, square))
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

    Eval Evaluator::evalPawns(const board::Bitboard myPawns, const board::Bitboard theirPawns) const noexcept
    {
        std::array<board::Bitboard, 8> files = {board::fileMask(board::a1), board::fileMask(board::b1), 
        board::fileMask(board::c1), board::fileMask(board::d1), board::fileMask(board::e1),
        board::fileMask(board::f1), board::fileMask(board::g1), board::fileMask(board::h1)};

        std::array<board::Bitboard, 8> neighboringFiles = {files[1], files[0] | files[2], 
        files[1] | files[3], files[2] | files[4], files[3] | files[5], files[4] | files[6], 
        files[5] | files[7], files[6]};

        Eval evaluation = 0;
        for (auto file : files)
        {
            evaluation -= doubledpawnpenalty * (_popcnt64(myPawns & file) == 2);
            evaluation -= tripledpawnpenalty * (_popcnt64(myPawns & file) > 2);
            evaluation += doubledpawnpenalty * (_popcnt64(theirPawns & file) == 2);
            evaluation += tripledpawnpenalty * (_popcnt64(theirPawns & file) > 2);
        }

        for (std::size_t i = 0; i != files.size(); ++i)
        {
            if (myPawns & files[i])
            {
                if (!(myPawns & neighboringFiles[i]))
                {
                    evaluation -= isolatedpawnpenalty;
                }
            }
            if (theirPawns & files[i])
            {
                if (!(theirPawns & neighboringFiles[i]))
                {
                    evaluation += isolatedpawnpenalty;
                }
            }
        }

        auto [myPassedPawns, theirPassedPawns] = detectPassedPawns(myPawns, theirPawns);

        aux::GetNextBit<board::square> ppSquare(myPassedPawns);
        while (ppSquare())
        {
            auto rank = aux::rank(ppSquare.next);
            evaluation += _passedPawnBonus[rank - 1];
        }

        ppSquare = aux::GetNextBit<board::square>{theirPassedPawns};
        while (ppSquare())
        {
            auto rank = aux::rank(aux::flip(ppSquare.next));
            evaluation -= _passedPawnBonus[rank - 1];
        }

        //auto [myBackwards, theirBackwards] = moves::backwardPawns(myPawns, theirPawns);

        //evaluation -= backwardspawnpenalty * _popcnt64(myBackwards);
        //evaluation += backwardspawnpenalty * _popcnt64(theirBackwards);

        return evaluation;
    }

    Eval Evaluator::operator()(const board::QBB& b) const
    {
        Eval evaluation = 0;

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
        const auto occ = b.getOccupancy();
        enum {myPawns, myKnights, myBishops, myRooks, myQueens, myKing,
            theirPawns, theirKnights, theirBishops, theirRooks, theirQueens, theirKing,};

        for (std::size_t i = 0; i != 5; ++i)
        {
            evaluation += piecevals[i] * (_popcnt64(pieces[i]) - _popcnt64(pieces[i + 6]));
        }

        const auto myKingSq = board::square(_tzcnt_u64(pieces[myKing]));
        const auto oppKingSq = board::square(_tzcnt_u64(pieces[theirKing]));

        for (std::size_t i = 0; i != 12; ++i)
        {
            evaluation += (i < 6 ? 1 : -1) * applyAggressionBonus(i, i < 6 ? oppKingSq : myKingSq, pieces[i]);
        }

        aux::GetNextBit<board::Bitboard> mobility(pieces[myKnights]);
        while (mobility())
        {
            moves::AttackMap moves = moves::knightAttacks(mobility.next);
            moves &= ~(moves::enemyPawnAttacks(pieces[theirPawns]) | pieces[myKing] | pieces[myPawns]);
            evaluation += knightmobility*_popcnt64(moves);
        }
        mobility = GetNextBit<board::Bitboard>{ pieces[theirKnights] };
        while (mobility())
        {
            moves::AttackMap moves = moves::knightAttacks(mobility.next);
            moves &= ~(moves::pawnAttacks(pieces[myPawns]) | pieces[theirKing] | pieces[theirPawns]);
            evaluation -= knightmobility*_popcnt64(moves);
        }
        mobility = GetNextBit<board::Bitboard>{ pieces[myBishops] };
        while (mobility())
        {
            moves::AttackMap moves = moves::hypqAllDiag(occ & ~pieces[myQueens], mobility.next);
            moves &= ~(moves::enemyPawnAttacks(pieces[theirPawns]) | pieces[myKing] | pieces[myPawns]);
            evaluation += bishopmobility*_popcnt64(moves);
        }
        mobility = GetNextBit<board::Bitboard>{ pieces[theirBishops] };
        while (mobility())
        {
            moves::AttackMap moves = moves::hypqAllDiag(occ & ~pieces[theirQueens], mobility.next);
            moves &= ~(moves::pawnAttacks(pieces[myPawns]) | pieces[theirKing] | pieces[theirPawns]);
            evaluation -= bishopmobility*_popcnt64(moves);
        }
        mobility = GetNextBit<board::Bitboard>{ pieces[myRooks] };
        while (mobility())
        {
            moves::AttackMap moves = moves::hypqRank(occ & ~(pieces[myQueens] | pieces[myRooks]), mobility.next);
            moves &= ~(moves::enemyPawnAttacks(pieces[theirPawns]) | pieces[myKing] | pieces[myPawns]);
            evaluation += rookhormobility*_popcnt64(moves);
            moves = moves::hypqFile(occ & ~(pieces[myQueens] | pieces[myRooks]), mobility.next);
            moves &= ~(moves::enemyPawnAttacks(pieces[theirPawns]) | pieces[myKing] | pieces[myPawns]);
            evaluation += rookvertmobility*_popcnt64(moves);
        }
        mobility = GetNextBit<board::Bitboard>{ pieces[theirRooks] };
        while (mobility())
        {
            moves::AttackMap moves = moves::hypqRank(occ & ~(pieces[theirQueens] | pieces[theirRooks]), mobility.next);
            moves &= ~(moves::pawnAttacks(pieces[myPawns]) | pieces[theirKing] | pieces[theirPawns]);
            evaluation -= rookhormobility*_popcnt64(moves);
            moves = moves::hypqFile(occ & ~(pieces[theirQueens] | pieces[theirRooks]), mobility.next);
            moves &= ~(moves::pawnAttacks(pieces[myPawns]) | pieces[theirKing] | pieces[theirPawns]);
            evaluation -= rookvertmobility*_popcnt64(moves);
        }

        evaluation += evalPawns(pieces[myPawns], pieces[theirPawns]); // TODO store this in pawn hash

        if (isEndgame(b))
        {
            evaluation += kingCentralization(myKingSq);
            evaluation -= kingCentralization(oppKingSq);
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

        auto printTerm = [&](const std::string& name, Eval e) {oss << name << "=" << e << '\n'; };

        auto printArray = [&](const std::string& name, const auto& array)
        {
            oss << name << "={";
            for (const auto& i : array)
            {
                oss << i << ",";
            }
            oss << "}" << '\n';
        };

        auto printArrayOfPairs = [&](const std::string& name, const auto& array)
        {
            oss << name << "={";
            for (const auto& [i, j] : array)
            {
                oss << "(" << i << "," << j << ")" << ",";
            }
            oss << "}" << '\n';
        };

        oss << '\n';
        auto now = std::chrono::current_zone()->to_local(std::chrono::system_clock::now());
        oss << "Tuning completed date/time: " << now << '\n' << '\n';

        printArray("piecevals", piecevals);

        printTerm("knightmobility", knightmobility);
        printTerm("bishopmobility", bishopmobility);
        printTerm("rookvertmobility", rookvertmobility);
        printTerm("rookhormobility", rookhormobility);

        printTerm("doubledpawnpenalty", doubledpawnpenalty);
        printTerm("tripledpawnpenalty", tripledpawnpenalty);
        printTerm("isolatedpawnpenalty", isolatedpawnpenalty);

        printArray("_passedPawnBonus", _passedPawnBonus);
        printArrayOfPairs("_aggressionBonuses", _aggressionBonuses);

        printTerm("_bishopOpenDiagonalBonus", _bishopOpenDiagonalBonus);
        printTerm("_rookOpenFileBonus", _rookOpenFileBonus);
        printTerm("rook7thRankBonus", rook7thRankBonus);
        printTerm("_bishopPairBonus", _bishopPairBonus);
        printTerm("_kingCenterBonus", _kingCenterBonus);
        printTerm("_kingCenterRingBonus", _kingCenterRingBonus);
        printTerm("_knightOutpostBonus", _knightOutpostBonus);
        printTerm("openFileNextToKingPenalty", openFileNextToKingPenalty);
        printTerm("pawnShieldBonus", pawnShieldBonus);
        printTerm("kingOpenFilePenalty", kingOpenFilePenalty);

        return oss.str();

    }

    void EvaluatorGeneticOps::mutate(Evaluator& e, double mutation_rate)
    {
        std::bernoulli_distribution doMutate(mutation_rate);
        std::uniform_int_distribution positionalBonus(-50, 50);
        std::uniform_int_distribution ZeroTo8(0, 8);

        auto mutate = [&](auto& mutator, int p = 0) {
            return doMutate(aux::seed) ? mutator(aux::seed) : p;
        };

        for (std::size_t i = 0; i != e._aggressionBonuses.size(); ++i)
        {
            e._aggressionBonuses[i].first = mutate(ZeroTo8, e._aggressionBonuses[i].first);
            e._aggressionBonuses[i].second += mutate(positionalBonus);
        }

        for (std::size_t i = 0; i != e.piecevals.size(); ++i)
        {
            e.piecevals[i] += mutate(positionalBonus);
        }

        for (std::size_t i = 0; i != e._passedPawnBonus.size(); ++i)
        {
            e._passedPawnBonus[i] += mutate(positionalBonus);
        }


        e.knightmobility = mutate(ZeroTo8, e.knightmobility);
        e.bishopmobility = mutate(ZeroTo8, e.bishopmobility);
        e.rookvertmobility = mutate(ZeroTo8, e.rookvertmobility);
        e.rookhormobility = mutate(ZeroTo8, e.rookhormobility);

        e.doubledpawnpenalty += mutate(positionalBonus);
        e.tripledpawnpenalty += mutate(positionalBonus);
        e.isolatedpawnpenalty += mutate(positionalBonus);

        e._bishopOpenDiagonalBonus += mutate(positionalBonus);
        e._rookOpenFileBonus += mutate(positionalBonus);
        e._bishopPairBonus += mutate(positionalBonus);
        e.rook7thRankBonus += mutate(positionalBonus);

        e._kingCenterBonus += mutate(positionalBonus);
        e._kingCenterRingBonus += mutate(positionalBonus);

        e._knightOutpostBonus += mutate(positionalBonus);

        e.openFileNextToKingPenalty += mutate(positionalBonus);
        e.pawnShieldBonus += mutate(positionalBonus);

        e.kingOpenFilePenalty += mutate(positionalBonus);
    }

    Evaluator EvaluatorGeneticOps::crossover(const Evaluator& e1, const Evaluator& e2)
    {
        Evaluator e;
        std::bernoulli_distribution which_e;
        auto parent = [&e1, &e2, &which_e]() -> const Evaluator& {
            return which_e(aux::seed) ? e1 : e2;
        };

        for (std::size_t i = 0; i != e.piecevals.size(); ++i)
        {
            e.piecevals[i] = parent().piecevals[i];
        }

        for (std::size_t i = 0; i != e._passedPawnBonus.size(); ++i)
        {
            e._passedPawnBonus[i] = parent()._passedPawnBonus[i];
        }

        e.knightmobility = parent().knightmobility;
        e.bishopmobility = parent().bishopmobility;
        e.rookvertmobility = parent().rookvertmobility;
        e.rookhormobility = parent().rookhormobility;
        e.doubledpawnpenalty = parent().doubledpawnpenalty;
        e.tripledpawnpenalty = parent().tripledpawnpenalty;
        e.isolatedpawnpenalty = parent().isolatedpawnpenalty;
        e._bishopOpenDiagonalBonus = parent()._bishopOpenDiagonalBonus;
        e._rookOpenFileBonus = parent()._rookOpenFileBonus;
        e.rook7thRankBonus = parent().rook7thRankBonus;
        e._bishopPairBonus = parent()._bishopPairBonus;
        e._kingCenterBonus = parent()._kingCenterBonus;
        e._kingCenterRingBonus = parent()._kingCenterRingBonus;
        e._knightOutpostBonus = parent()._knightOutpostBonus;
        e._knightOutpostBonus = parent()._knightOutpostBonus;
        e.openFileNextToKingPenalty = parent().openFileNextToKingPenalty;
        e.pawnShieldBonus = parent().pawnShieldBonus;
        e.kingOpenFilePenalty = parent().kingOpenFilePenalty;

        for (std::size_t i = 0; i != 12; ++i)
        {
            e._aggressionBonuses[i].first = parent()._aggressionBonuses[i].first;
            e._aggressionBonuses[i].second = parent()._aggressionBonuses[i].second;
        }

        return e;
    }
}