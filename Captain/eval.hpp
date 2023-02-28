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
#include "movegen.hpp"

namespace eval
{
    using namespace aux;
    using Eval = std::int16_t;

    constexpr std::array<Eval, 64> PSQTknight = {
        200, 250, 210, 210, 210, 210, 250, 200,
        250, 210, 300, 300, 300, 300, 210, 250,
        250, 300, 360, 360, 360, 360, 300, 250,
        250, 300, 360, 360, 360, 360, 300, 250,
        250, 300, 360, 360, 360, 360, 300, 250,
        250, 300, 360, 360, 360, 360, 300, 250,
        250, 210, 300, 300, 300, 300, 210, 250,
        200, 250, 210, 210, 210, 210, 250, 200
    };

    constexpr std::array<Eval, 64> PSQTbishop = {
        270, 270, 270, 270, 270, 270, 270, 270,
        270, 310, 300, 300, 300, 300, 310, 270,
        270, 300, 330, 330, 330, 330, 300, 270,
        270, 300, 300, 330, 330, 300, 300, 270,
        270, 300, 300, 330, 330, 300, 300, 270,
        270, 300, 330, 330, 330, 330, 300, 270,
        270, 310, 300, 300, 300, 300, 310, 270,
        270, 270, 270, 270, 270, 270, 270, 270
    };

    constexpr std::array<Eval, 64> PSQTrookw = {
        500, 500, 500, 500, 500, 500, 500, 500,
        500, 500, 500, 500, 500, 500, 500, 500,
        500, 500, 500, 500, 500, 500, 500, 500,
        500, 500, 500, 500, 500, 500, 500, 500,
        500, 500, 500, 500, 500, 500, 500, 500,
        500, 500, 500, 500, 500, 500, 500, 500,
        525, 525, 525, 525, 525, 525, 525, 525,
        500, 500, 500, 500, 500, 500, 500, 500
    };

    constexpr std::array<Eval, 64> PSQTrookb = {
        500, 500, 500, 500, 500, 500, 500, 500,
        525, 525, 525, 525, 525, 525, 525, 525,
        500, 500, 500, 500, 500, 500, 500, 500,
        500, 500, 500, 500, 500, 500, 500, 500,
        500, 500, 500, 500, 500, 500, 500, 500,
        500, 500, 500, 500, 500, 500, 500, 500,
        500, 500, 500, 500, 500, 500, 500, 500,
        500, 500, 500, 500, 500, 500, 500, 500
    };

    constexpr std::array<Eval, 64> PSQTpawnw = {
        0, 0, 0, 0, 0, 0, 0, 0,
        100, 100, 100, 80, 80, 100, 100, 100,
        100, 100, 100, 100, 100, 100, 100, 100,
        100, 100, 100, 110, 110, 100, 100, 100,
        130, 130, 130, 130, 130, 130, 130, 130,
        150, 150, 150, 150, 150, 150, 150, 150,
        200, 200, 200, 200, 200, 200, 200, 200,
        0, 0, 0, 0, 0, 0, 0, 0
    };

    constexpr std::array<Eval, 64> PSQTpawnb = {
        0, 0, 0, 0, 0, 0, 0, 0,
        200, 200, 200, 200, 200, 200, 200, 200,
        150, 150, 150, 150, 150, 150, 150, 150,
        130, 130, 130, 130, 130, 130, 130, 130,
        100, 100, 100, 110, 110, 100, 100, 100,
        100, 100, 100, 100, 100, 100, 100, 100,
        100, 100, 100, 80, 80, 100, 100, 100,
        0, 0, 0, 0, 0, 0, 0, 0
    };

    constexpr std::array<Eval, 64> PSQTqueen = {
        810, 810, 810, 810, 810, 810, 810, 810,
        810, 810, 810, 900, 900, 810, 810, 810,
        900, 900, 930, 930, 930, 930, 900, 900,
        900, 900, 930, 930, 930, 930, 900, 900,
        900, 900, 930, 930, 930, 930, 900, 900,
        900, 900, 930, 930, 930, 930, 900, 900,
        810, 810, 810, 900, 900, 810, 810, 810,
        810, 810, 810, 810, 810, 810, 810, 810
    };

    constexpr std::array<Eval, 64> PSQTking = {
        0, 0, 20, 0, 0, 0, 20, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 20, 0, 0, 0, 20, 0
    };

    constexpr std::array<Eval, 64> PSQTkingEnd = {
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 15, 15, 15, 15, 0, 0,
    0, 0, 15, 20, 20, 15, 0, 0,
    0, 0, 15, 20, 20, 15, 0, 0,
    0, 0, 15, 15, 15, 15, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0
    };

    std::uint32_t getLVA(const board::QBB&, board::Bitboard, board::Bitboard&);
    Eval getCaptureValue(const board::QBB&, board::Move);
    Eval mvvlva(const board::QBB&, board::Move);
    Eval see(const board::QBB&, board::Move);
    Eval evalCapture(const board::QBB&, board::Move);

    Eval computeMaterialValue(board::Bitboard, const std::array<Eval, 64>&);

    // pawnCount = number of pawns on same color square as bishop
    constexpr Eval pawnCountBishopPenalty(unsigned pawnCount)
    {
        return (pawnCount >= 6) * 50;
    }

    constexpr Eval bishopOpenDiagonalBonus(bool open)
    {
        return open ? 15 : 0;
    }

    constexpr Eval rookOpenFileBonus(bool open)
    {
        return open ? 25 : 0;
    }

    constexpr Eval bishopPairBonus(bool pair)
    {
        return pair ? 25 : 0;
    }

    constexpr Eval aggressionBonus(board::square psq, board::square enemyKingSq, unsigned closeness, Eval bonus)
    {
        int pRank = rank(psq);
        int pFile = file(psq);
        int kRank = rank(enemyKingSq);
        int kFile = file(enemyKingSq);
        return (l1dist(pRank, pFile, kRank, kFile) <= closeness) * bonus;
    }

    enum class OutpostType{MyOutpost, OppOutpost};
    template<OutpostType t>
    constexpr Eval knightOutpostBonus(board::square knightsq, board::Bitboard myPawns, board::Bitboard enemyPawns)
    {
        if constexpr (t == OutpostType::MyOutpost)
        {
            board::Bitboard myKnight = setbit(knightsq);
            if ((myKnight & constants::topHalf) && (movegen::pawnAttacks(myPawns) & myKnight))
            {
                myKnight |= movegen::KSNorth(0, myKnight);
                myKnight = movegen::pawnAttacks(myKnight);
                if (!(myKnight & enemyPawns))
                {
                    return 15;
                }
            }
            return 0;
        }
        else if constexpr (t == OutpostType::OppOutpost)
        {
            board::Bitboard myKnight = setbit(knightsq);
            if ((myKnight & constants::botHalf) && (movegen::enemyPawnAttacks(enemyPawns) & myKnight))
            {
                myKnight |= movegen::KSSouth(0, myKnight);
                myKnight = movegen::enemyPawnAttacks(myKnight);
                if (!(myKnight & myPawns))
                {
                    return 15;
                }
            }
            return 0;
        }
    }

    Eval evaluate(const board::QBB& b);

    // This class represents a single evaluation function (useful for tuning)
    class Evaluator
    {
        using PSQT = std::array<Eval, 64>;
        std::array<PSQT, 12> _openingPSQT;
        unsigned _openToMid;
        unsigned _midToEnd;
        std::array<PSQT, 12> _midPSQT;
        std::array<PSQT, 12> _endPSQT;
        std::array<std::pair<unsigned, Eval>, 12> _aggressionBonuses;
        std::pair<unsigned, Eval> _pawnBishopPenalty;
        Eval _bishopOpenDiagonalBonus;
        Eval _rookOpenFileBonus;
        Eval _bishopPairBonus;
        std::pair<Eval, Eval> _knightOutpostBonus;

        unsigned totalMaterialValue(const board::QBB& b) const;

        constexpr Eval aggressionBonus(board::square psq, board::square enemyKingSq, std::pair<unsigned, Eval> aggression) const
        {
            auto [closeness, bonus] = aggression;
            int pRank = rank(psq);
            int pFile = file(psq);
            int kRank = rank(enemyKingSq);
            int kFile = file(enemyKingSq);
            return (l1dist(pRank, pFile, kRank, kFile) <= closeness) * bonus;
        }

        constexpr Eval pawnCountBishopPenalty(unsigned pawnCount, bool bishopsExist) const
        {
            return bishopsExist * (pawnCount >= _pawnBishopPenalty.first) * _pawnBishopPenalty.second;
        }

        Eval bishopOpenDiagonalBonus(board::Bitboard occ, board::Bitboard bishops) const;


        Eval rookOpenFileBonus(board::Bitboard pawns, board::Bitboard rooks) const;


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
                if ((myKnight & constants::topHalf) && (movegen::pawnAttacks(myPawns) & myKnight))
                {
                    myKnight |= movegen::KSNorth(0, myKnight);
                    myKnight = movegen::pawnAttacks(myKnight);
                    if (!(myKnight & enemyPawns))
                    {
                        return _knightOutpostBonus.first;
                    }
                }
                return 0;
            }
            else if constexpr (t == OutpostType::OppOutpost)
            {
                board::Bitboard myKnight = setbit(knightsq);
                if ((myKnight & constants::botHalf) && (movegen::enemyPawnAttacks(enemyPawns) & myKnight))
                {
                    myKnight |= movegen::KSSouth(0, myKnight);
                    myKnight = movegen::enemyPawnAttacks(myKnight);
                    if (!(myKnight & myPawns))
                    {
                        return _knightOutpostBonus.second;
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
    public:
        Eval operator()(const board::QBB&) const;

        constexpr Evaluator()
            : _openToMid(7000), _midToEnd(3000), _pawnBishopPenalty(std::make_pair(6, 50)), 
            _bishopOpenDiagonalBonus(15), _rookOpenFileBonus(25), _bishopPairBonus(25),
            _knightOutpostBonus(std::make_pair(15, 15))
        {
            _openingPSQT[0].fill(100);
            _openingPSQT[1].fill(300);
            _openingPSQT[2].fill(300);
            _openingPSQT[3].fill(500);
            _openingPSQT[4].fill(900);
            _openingPSQT[5].fill(0);
            for (std::size_t i = 0; i != 6; ++i)
            {
                _midPSQT[i] = _openingPSQT[i];
                _endPSQT[i] = _openingPSQT[i];
            }
            for (std::size_t i = 6; i != 12; ++i)
            {
                _openingPSQT[i] = _openingPSQT[i - 6];
                _midPSQT[i] = _midPSQT[i - 6];
                _endPSQT[i] = _endPSQT[i - 6];
            }
            _aggressionBonuses[0] = std::make_pair(0, 0);
            _aggressionBonuses[1] = std::make_pair(4, 5);
            _aggressionBonuses[2] = std::make_pair(0, 0);
            _aggressionBonuses[3] = std::make_pair(7, 5);
            _aggressionBonuses[4] = std::make_pair(3, 5);
            _aggressionBonuses[5] = std::make_pair(0, 0);
            for (std::size_t i = 0; i != 12; ++i)
            {
                _aggressionBonuses[i] = _aggressionBonuses[i - 6];
            }
        }

        const Evaluator& mutate(bool randomize);

        static Evaluator crossover(const Evaluator& e1, const Evaluator& e2)
        {
            Evaluator e;
            std::random_device rd;
            std::mt19937_64 urbg(rd());
            std::bernoulli_distribution whiche;
            auto parent = [&e1, &e2, &whiche, &urbg]() -> const Evaluator& {
                return whiche(urbg) ? e1 : e2;
            };

            e._openToMid = parent()._openToMid;
            e._midToEnd = parent()._midToEnd;
            e._bishopOpenDiagonalBonus = parent()._bishopOpenDiagonalBonus;
            e._rookOpenFileBonus = parent()._rookOpenFileBonus;
            e._bishopPairBonus = parent()._bishopPairBonus;
            e._knightOutpostBonus.first = parent()._knightOutpostBonus.first;
            e._knightOutpostBonus.second = parent()._knightOutpostBonus.second;
            e._pawnBishopPenalty.first = parent()._pawnBishopPenalty.first;
            e._pawnBishopPenalty.second = parent()._pawnBishopPenalty.second;
            for (std::size_t i = 0; i != 12; ++i)
            {
                for (std::size_t j = 0; j != 64; ++j)
                {
                    e._openingPSQT[i][j] = parent()._openingPSQT[i][j];
                    e._midPSQT[i][j] = parent()._midPSQT[i][j];
                    e._endPSQT[i][j] = parent()._endPSQT[i][j];
                }
            }

            for (std::size_t i = 0; i != 12; ++i)
            {
                e._aggressionBonuses[i].first = parent()._aggressionBonuses[i].first;
                e._aggressionBonuses[i].second = parent()._aggressionBonuses[i].second;
            }

            return e;
        }
    };
}
#endif
