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

#ifndef EVALUATION_H
#define EVALUATION_H

#include <cstdint>
#include <cstddef>
#include <array>
#include <random>
#include <span>

#include "auxiliary.hpp"
#include "board.hpp"
#include "moves.hpp"

namespace eval
{
    using namespace aux;
    using Eval = std::int16_t;

    std::uint32_t getLVA(const board::QBB&, Bitboard, Bitboard&);
    Eval getCaptureValue(const board::QBB&, Move);
    Eval see(const board::QBB&, Move);

    // TODO better squareControl function
    constexpr Eval squareControl(const board::QBB& b, board::square s)
    {
        Eval control = 0;

        auto myAttackers = b.my(b.getPawns()) & moves::enemyPawnAttacks(s);
        auto theirAttackers = b.their(b.getPawns()) & moves::pawnAttacks(s);
        control += 900 * (_popcnt64(myAttackers) - _popcnt64(theirAttackers));

        myAttackers = b.my(b.getKnights()) & moves::knightAttacks(s);
        theirAttackers = b.their(b.getKnights()) & moves::knightAttacks(s);
        control += 500 * (_popcnt64(myAttackers) - _popcnt64(theirAttackers));

        const auto sliders = moves::getSliderAttackers(b.getOccupancy(), s, b.getDiagSliders(), b.getOrthSliders());

        myAttackers = b.my(b.getBishops()) & sliders;
        theirAttackers = b.their(b.getBishops()) & sliders;
        control += 500 * (_popcnt64(myAttackers) - _popcnt64(theirAttackers));

        myAttackers = b.my(b.getRooks()) & sliders;
        theirAttackers = b.their(b.getRooks()) & sliders;
        control += 300 * (_popcnt64(myAttackers) - _popcnt64(theirAttackers));

        myAttackers = b.my(b.getQueens()) & sliders;
        theirAttackers = b.their(b.getQueens()) & sliders;
        control += 100 * (_popcnt64(myAttackers) - _popcnt64(theirAttackers));

        myAttackers = b.my(b.getKings()) & moves::kingAttacks(s);
        theirAttackers = b.their(b.getKings()) & moves::kingAttacks(s);
        control += 50 * (_popcnt64(myAttackers) - _popcnt64(theirAttackers));

        return control;
    }

    Eval computeMaterialValue(Bitboard, const std::array<Eval, 64>&);


    // This class represents a single evaluation function (useful for tuning)
    class Evaluator
    {
    public:
        using PSQT = std::array<Eval, 64>;
        constexpr Eval piecevals(std::size_t i) const { return evalTerms[i]; };
        constexpr Eval knightMobility() const { return evalTerms[5]; }
        constexpr Eval bishopMobility() const { return evalTerms[6]; }
        constexpr Eval rookVertMobility() const { return evalTerms[7]; }
        constexpr Eval rookHorMobility() const { return evalTerms[8]; }
        constexpr Eval doubledPawnPenalty() const { return evalTerms[9]; }
        constexpr Eval tripledPawnPenalty() const { return evalTerms[10]; }
        constexpr Eval isolatedPawnPenalty() const { return evalTerms[11]; }
        constexpr Eval passedPawnBonus(std::size_t rank) const { return evalTerms[12 + rank - 1]; }
        constexpr Eval closenessBonus(std::size_t pt) const { return evalTerms[18 + (pt % 6)]; }
        constexpr Eval bishopOpenDiagBonus() const { return evalTerms[42]; }
        constexpr Eval rookOpenFileBonus() const { return evalTerms[43]; }
        constexpr Eval rookRank7Bonus() const { return evalTerms[44]; }
        constexpr Eval bishopPairBonus() const { return evalTerms[45]; }
        constexpr Eval kingCenterBonus() const { return evalTerms[46]; }
        constexpr Eval kingCenterRingBonus() const { return evalTerms[47]; }
        constexpr Eval knightOutpostBonus() const { return evalTerms[48]; }
        constexpr Eval kingAdjFileOpenPenalty() const { return evalTerms[49]; }
        constexpr Eval kingFileOpenPenalty() const { return evalTerms[50]; }
        constexpr Eval pawnShieldBonus() const { return evalTerms[51]; }
        constexpr Eval kingAttackerValue(std::size_t type) const { return evalTerms[52 + type]; }
        constexpr Eval backwardsPawnPenalty() const { return evalTerms[57]; }

        std::array<Eval, 58> evalTerms =
        { 93,256,276,440,1070,17,14,15,9,11,
            111,-1,-24,-10,2,57,131,160,1,1,
            2,3,3,1,8,32,6,50,0,0,
            0,0,5,28,0,0,8,40,5,50,
            0,0,15,-14,37,27,17,14,17,18,
            114,-1,27,33,22,64,70,27, };

        using ParamListType = decltype(evalTerms);

    private:
        enum OutpostType {MyOutpost, OppOutpost};

        unsigned totalMaterialValue(const board::QBB& b) const;

        bool isEndgame(const board::QBB&) const;

        constexpr Eval kingCentralization(board::square s) const
        {
            if (aux::setbit(s) & constants::center)
                return kingCenterBonus();
            else if (aux::setbit(s) & constants::centerRing)
                return kingCenterRingBonus();
            else
                return 0;
        }

        Eval kingSafety(const board::QBB& b, board::square myKing, board::square theirKing) const;

        constexpr std::pair<Bitboard, Bitboard> detectPassedPawns(Bitboard myPawns, Bitboard theirPawns) const
        {
            Bitboard myPawnSpans = moves::pawnAttacksLeft(myPawns) | moves::pawnAttacksRight(myPawns) | moves::pawnMovesUp(myPawns);
            myPawnSpans |= moves::KSNorth(0, myPawnSpans);
            Bitboard theirPawnSpans = moves::enemyPawnAttacksLeft(theirPawns) | moves::enemyPawnAttacksRight(theirPawns) | (theirPawns >> 8);
            theirPawnSpans |= moves::KSSouth(0, theirPawnSpans);

            return std::make_pair(myPawns & ~theirPawnSpans, theirPawns & ~myPawnSpans);
        }

        constexpr Eval aggressionBonus(board::square psq, board::square enemyKingSq, Eval bonus) const
        {
            int pRank = rank(psq);
            int pFile = file(psq);
            int kRank = rank(enemyKingSq);
            int kFile = file(enemyKingSq);
            return bonus * (7 - std::max(std::abs(pRank - kRank), std::abs(pFile - kFile)));
        }

        Eval bishopOpenDiagonalBonus(Bitboard occ, Bitboard bishops) const;

        Eval rookOpenFileBonus(Bitboard pawns, Bitboard rooks) const;

        Eval evalPawns(Bitboard myPawns, Bitboard theirPawns) const noexcept;

        constexpr Eval bishopPairBonus(bool pair) const
        {
            return pair ? bishopPairBonus() : 0;
        }

        template<OutpostType t>
        Eval knightOutpostBonus(board::square knightsq, Bitboard myPawns, Bitboard enemyPawns) const
        {
            if constexpr (t == OutpostType::MyOutpost)
            {
                Bitboard myKnight = setbit(knightsq);
                if ((myKnight & constants::topHalf) && (moves::pawnAttacks(myPawns) & myKnight))
                {
                    myKnight |= moves::KSNorth(0, myKnight);
                    myKnight = moves::pawnAttacks(myKnight);
                    if (!(myKnight & enemyPawns))
                    {
                        return knightOutpostBonus();
                    }
                }
                return 0;
            }
            else if constexpr (t == OutpostType::OppOutpost)
            {
                Bitboard myKnight = setbit(knightsq);
                if ((myKnight & constants::botHalf) && (moves::enemyPawnAttacks(enemyPawns) & myKnight))
                {
                    myKnight |= moves::KSSouth(0, myKnight);
                    myKnight = moves::enemyPawnAttacks(myKnight);
                    if (!(myKnight & myPawns))
                    {
                        return knightOutpostBonus();
                    }
                }
                return 0;
            }
        }

        template<OutpostType ot>
        Eval applyKnightOutPostBonus(Bitboard knights, Bitboard myPawns, Bitboard oppPawns) const
        {
            GetNextBit<board::square> square(knights);
            Eval e = 0;
            while (square())
            {
                auto sq = square.next;
                e += knightOutpostBonus<ot>(sq, myPawns, oppPawns);
            }
            return e;
        }

        Eval applyAggressionBonus(std::size_t type, board::square enemyKingSq, Bitboard pieces) const;

        Eval apply7thRankBonus(Bitboard rooks, Bitboard rank) const;

    public:
        friend struct EvaluatorGeneticOps;
        using GAOps = EvaluatorGeneticOps;

        Eval operator()(const board::QBB&) const;

        Eval materialBalance(const board::QBB& b) const;

        constexpr Evaluator() {}
        std::string asString() const;
    };

    struct EvaluatorGeneticOps
    {
        void mutate(Evaluator&, double mutation_rate);
        Evaluator crossover(const Evaluator&, const Evaluator&);
    };
}
#endif
