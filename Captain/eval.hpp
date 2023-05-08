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
        constexpr Eval knightPawnCountPenalty(std::size_t pawnCount) const { return evalTerms[24 + (pawnCount / 4)]; }
        constexpr Eval rookPawnCountBonus(std::size_t pawnCount) const { return evalTerms[29 + (pawnCount / 4)]; }
        constexpr Eval connectedRookBonus() const {return evalTerms[34];}
        constexpr Eval doubledRookBonus() const { return evalTerms[35]; }
        constexpr Eval undefendedKnightPenalty() const { return evalTerms[36]; }
        constexpr Eval undefendedBishopPenalty() const { return evalTerms[37]; }
        constexpr Eval kingPassedPDistPenalty() const { return evalTerms[38]; }
        constexpr Eval rookBehindPassedP() const { return evalTerms[39]; }
        constexpr Eval pawnIslandPenalty() const { return evalTerms[40]; }
        constexpr Eval connectedPawnBonus() const { return evalTerms[41]; }
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
        constexpr Eval overDefendedPenalty(std::size_t pc) const { return evalTerms[58 + pc]; }
        constexpr Eval connectivityBonus(std::size_t pc) const { return evalTerms[64 + pc]; }
        constexpr Eval connectivityBonus(std::size_t pc1, std::size_t pc2) const
        {
            if (pc1 > pc2) std::swap(pc1, pc2);
            constexpr std::array<unsigned char, 6> offsets = {0, 6, 6+5, 6+5+4, 6+5+4+3, 6+5+4+3+2};
            constexpr std::array<signed char, 6> adj = {0, -1, -2, -3, -4, -5};
            return evalTerms[70 + offsets[pc1] + (pc2 + adj[pc1])];
        }
        constexpr Eval connectivityBonus(std::size_t pc1, std::size_t pc2, std::size_t pc3) const
        {
            /*
            * Code for computation of offsets array
            * 
            offsets[0] = 0;
            int lastNumAdded = 6;
            for (size_t i = 0; i != 6; ++i)
            {
                for (size_t j = (i == 0 ? 1 : i); j != 6; ++j)
                {
                    auto idx = cb2offset(i, j) - 70;
                    offsets[idx] = offsets[idx - 1] + lastNumAdded;
                    lastNumAdded = 6 - j;
                }
            }
            */
            constexpr auto cb2offset = [](std::size_t pc1, std::size_t pc2) {
                if (pc1 > pc2) std::swap(pc1, pc2);
                constexpr std::array<unsigned char, 6> offsets = { 0, 6, 6+5, 6+5+4, 6+5+4+3, 6+5+4+3+2 };
                constexpr std::array<signed char, 6> adj = { 0, -1, -2, -3, -4, -5 };
                return offsets[pc1] + (pc2 + adj[pc1]);
            };
            aux::sort3(pc1, pc2, pc3);
            constexpr std::array<unsigned char, 21> offsets = { 0,6,11,15,18,20,21,26,30,33,35,36,40,43,45,46,49,51,52,54,55, };
            constexpr std::array<signed char, 6> adj = { 0, -1, -2, -3, -4, -5 };
            return evalTerms[91 + offsets[cb2offset(pc1, pc2)] + (pc3 + adj[pc2])];
        }
        constexpr Eval connectivityPieceVal(std::size_t pc) const { return evalTerms[147 + pc]; }
        constexpr Eval tempoBonus() const { return evalTerms[153]; }

        std::array<Eval, 154> evalTerms =
        { 93,256,276,440,1070,17,14,15,9,11,
            111,-1,-24,-10,2,57,131,160,1,1,
            2,3,3,1,-16,-12,-8,-4,0,16,
            12,8,4,0,2,4,-8,-8,5,50,
            -5,5,15,-14,37,27,17,14,17,18,
            114,-1,27,33,22,64,70,27,20,10,
            10, 8, 1, 30, 8, 4, 5, 3 ,2 ,1,
            15, 11, 12, 10, 9, 10, 7, 8, 6, 5,
            6, 6, 7, 6, 6, 5, 4, 5, 8, 4,
            0, 0, 18, 19, 17, 16, 17, 14, 15, 13,
            12, 13, 0, 14, 13, 13, 12, 11, 12, 7,
            11, 0, 5, 11, 9, 8, 9, 3, 11, 10,
            8, 8, 7, 7, 7, 7, 0, 5, 5, 5, 
            5, 9, 8, 6, 6, 7, 0, 8, 6, 7,
            7, 6, 0, 6, 7, 0, 0, 50, 35, 30,
            10, 4, 0, 10};

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
