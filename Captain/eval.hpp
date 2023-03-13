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

#include "auxiliary.hpp"
#include "board.hpp"
#include "moves.hpp"

namespace eval
{
    using namespace aux;
    using Eval = std::int16_t;

    std::uint32_t getLVA(const board::QBB&, board::Bitboard, board::Bitboard&);
    Eval getCaptureValue(const board::QBB&, board::Move);
    Eval mvvlva(const board::QBB&, board::Move);
    Eval see(const board::QBB&, board::Move);
    Eval evalCapture(const board::QBB&, board::Move);

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

    Eval computeMaterialValue(board::Bitboard, const std::array<Eval, 64>&);


    // This class represents a single evaluation function (useful for tuning)
    class Evaluator
    {
        using PSQT = std::array<Eval, 64>;
        std::array<Eval, 5> piecevals = {100, 300, 300, 500, 900};
        Eval knightmobility = 3;
        Eval bishopmobility = 3;
        Eval rookvertmobility = 2;
        Eval rookhormobility = 2;
        Eval doubledpawnpenalty = 50;
        Eval tripledpawnpenalty = 100;
        Eval isolatedpawnpenalty = 25;
        std::array<Eval, 6> _passedPawnBonus = {0, 10, 50, 50, 100, 300};
        std::array<std::pair<unsigned, Eval>, 12> _aggressionBonuses;
        Eval _bishopOpenDiagonalBonus = 15;
        Eval _rookOpenFileBonus = 25;
        Eval rook7thRankBonus = 25;
        Eval _bishopPairBonus = 25;
        Eval _kingCenterBonus = 20;
        Eval _kingCenterRingBonus = 15;
        Eval _knightOutpostBonus = 15;

        enum OutpostType {MyOutpost, OppOutpost};

        unsigned totalMaterialValue(const board::QBB& b) const;

        bool isEndgame(const board::QBB&) const;

        constexpr Eval kingCentralization(board::square s) const
        {
            if (aux::setbit(s) & constants::center)
                return _kingCenterBonus;
            else if (aux::setbit(s) & constants::centerRing)
                return _kingCenterRingBonus;
            else
                return 0;
        }

        constexpr std::pair<board::Bitboard, board::Bitboard> detectPassedPawns(board::Bitboard myPawns, board::Bitboard theirPawns) const
        {
            board::Bitboard myPawnSpans = moves::pawnAttacksLeft(myPawns) | moves::pawnAttacksRight(myPawns) | moves::pawnMovesUp(myPawns);
            myPawnSpans |= moves::KSNorth(0, myPawnSpans);
            board::Bitboard theirPawnSpans = moves::enemyPawnAttacksLeft(theirPawns) | moves::enemyPawnAttacksRight(theirPawns) | (theirPawns >> 8);
            theirPawnSpans |= moves::KSSouth(0, theirPawnSpans);

            return std::make_pair(myPawns & ~theirPawnSpans, theirPawns & ~myPawnSpans);
        }

        constexpr Eval aggressionBonus(board::square psq, board::square enemyKingSq, std::pair<unsigned, Eval> aggression) const
        {
            auto [closeness, bonus] = aggression;
            int pRank = rank(psq);
            int pFile = file(psq);
            int kRank = rank(enemyKingSq);
            int kFile = file(enemyKingSq);
            return (l1dist(pRank, pFile, kRank, kFile) <= closeness) * bonus;
        }

        Eval bishopOpenDiagonalBonus(board::Bitboard occ, board::Bitboard bishops) const;

        Eval rookOpenFileBonus(board::Bitboard pawns, board::Bitboard rooks) const;

        Eval evalPawns(board::Bitboard myPawns, board::Bitboard theirPawns) const noexcept;

        constexpr Eval bishopPairBonus(bool pair) const
        {
            return pair ? _bishopPairBonus : 0;
        }

        template<OutpostType t>
        Eval knightOutpostBonus(board::square knightsq, board::Bitboard myPawns, board::Bitboard enemyPawns) const
        {
            if constexpr (t == OutpostType::MyOutpost)
            {
                board::Bitboard myKnight = setbit(knightsq);
                if ((myKnight & constants::topHalf) && (moves::pawnAttacks(myPawns) & myKnight))
                {
                    myKnight |= moves::KSNorth(0, myKnight);
                    myKnight = moves::pawnAttacks(myKnight);
                    if (!(myKnight & enemyPawns))
                    {
                        return _knightOutpostBonus;
                    }
                }
                return 0;
            }
            else if constexpr (t == OutpostType::OppOutpost)
            {
                board::Bitboard myKnight = setbit(knightsq);
                if ((myKnight & constants::botHalf) && (moves::enemyPawnAttacks(enemyPawns) & myKnight))
                {
                    myKnight |= moves::KSSouth(0, myKnight);
                    myKnight = moves::enemyPawnAttacks(myKnight);
                    if (!(myKnight & myPawns))
                    {
                        return _knightOutpostBonus;
                    }
                }
                return 0;
            }
        }

        template<OutpostType ot>
        Eval applyKnightOutPostBonus(board::Bitboard knights, board::Bitboard myPawns, board::Bitboard oppPawns) const
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

        Eval applyAggressionBonus(std::size_t type, board::square enemyKingSq, board::Bitboard pieces) const;

        Eval apply7thRankBonus(board::Bitboard rooks, board::Bitboard rank) const;

    public:
        Eval operator()(const board::QBB&) const;

        constexpr Evaluator()
        {
            _aggressionBonuses[0] = std::make_pair(0, 0);
            _aggressionBonuses[1] = std::make_pair(4, 5);
            _aggressionBonuses[2] = std::make_pair(0, 0);
            _aggressionBonuses[3] = std::make_pair(7, 5);
            _aggressionBonuses[4] = std::make_pair(3, 5);
            _aggressionBonuses[5] = std::make_pair(0, 0);

            _aggressionBonuses[6] = std::make_pair(0, 0);
            _aggressionBonuses[7] = std::make_pair(4, 5);
            _aggressionBonuses[8] = std::make_pair(0, 0);
            _aggressionBonuses[9] = std::make_pair(7, 5);
            _aggressionBonuses[10] = std::make_pair(3, 5);
            _aggressionBonuses[11] = std::make_pair(0, 0);
        }
        // TODO update mutate and crossover to support new eval terms
        const Evaluator& mutate(bool randomize);

        static Evaluator crossover(const Evaluator& e1, const Evaluator& e2)
        {
            Evaluator e;
            std::bernoulli_distribution whiche;
            auto parent = [&e1, &e2, &whiche]() -> const Evaluator& {
                return whiche(aux::seed) ? e1 : e2;
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

            for (std::size_t i = 0; i != 12; ++i)
            {
                e._aggressionBonuses[i].first = parent()._aggressionBonuses[i].first;
                e._aggressionBonuses[i].second = parent()._aggressionBonuses[i].second;
            }

            return e;
        }
        std::string asString() const;
    };
}
#endif
