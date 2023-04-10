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

#ifndef CAPTAIN_MOVE_ORDER_HPP
#define CAPTAIN_MOVE_ORDER_HPP

#include "tables.hpp"
#include "board.hpp"
#include "moves.hpp"
#include "eval.hpp"

namespace moves
{
    class MoveOrder
    {
    public:
        MoveOrder(Tables::KillerTable* _kt, Tables::HistoryTable* _ht, std::uint64_t h, std::size_t depth)
            :kt(_kt), ht(_ht), hash(h), d(depth) {}
        bool next(const board::QBB& b, Move& m)
        {
            switch (stage)
            {
            case Stage::hash:
                hashmove = Tables::tt[hash].move;
                if (kt)
                {
                    k1move = kt->getKiller(d, 0);
                    k2move = kt->getKiller(d, 1);
                }
                if (Tables::tt[hash].key == hash && hashmove)
                {
                    if (isLegalMove(b, hashmove))
                    {
                        m = hashmove;
                        stage = Stage::captureStageGen;
                        stageReturned = Stage::hash;
                        return true;
                    }
                }
                stage = Stage::captureStageGen;
                [[fallthrough]];
            case Stage::captureStageGen:
                genMoves<QSearch>(b, ml);
                ml.remove_moves_if(ml.begin(), ml.end(), [this](auto sm) {return sm.m == hashmove; });
                for (auto& [move, score] : ml)
                {
                    score = eval::see(b, move);
                }
                captureBegin = ml.begin();
                captureEnd = ml.end();
                stage = ml.size() ? Stage::captureStage : Stage::killer1Stage;
                [[fallthrough]];
            case Stage::captureStage:
                if (captureBegin == captureEnd)
                {
                    stage = Stage::killer1Stage;
                }
                else
                {
                    auto bestCapture = std::max_element(captureBegin, captureEnd);
                    if (bestCapture->score < 0)
                    {
                        losingCapturesBegin = captureBegin;
                        stage = Stage::killer1Stage;
                    }
                    else
                    {
                        std::iter_swap(captureBegin, bestCapture);
                        m = captureBegin->m;
                        ++captureBegin;
                        stageReturned = Stage::captureStage;
                        return true;
                    }
                }
                [[fallthrough]];
            case Stage::killer1Stage:
                stage = Stage::killer2Stage;
                if (k1move != hashmove && isLegalMove(b, k1move))
                {
                    m = k1move;
                    stageReturned = Stage::killer1Stage;
                    return true;
                }
                [[fallthrough]];
            case Stage::killer2Stage:
                stage = Stage::quietsGen;
                if (k2move != hashmove && isLegalMove(b, k2move))
                {
                    m = k2move;
                    stageReturned = Stage::killer2Stage;
                    return true;
                }
                [[fallthrough]];
            case Stage::quietsGen:
                quietsCurrent = captureEnd;
                genMoves<!QSearch, Quiets>(b, ml);
                ml.remove_moves_if(quietsCurrent, ml.end(), [this](ScoredMove k) {return k.m == hashmove || k.m == k1move || k.m == k2move; });
                for (auto i = quietsCurrent; i != ml.end(); ++i)
                {
                    i->score = ht->getHistoryScore(b.getPieceCodeIdx(board::getMoveFromSq(i->m)), board::getMoveToSq(i->m));
                }
                quietsEnd = ml.end();
                stage = Stage::quiets;
                [[fallthrough]];
            case Stage::quiets:
                if (quietsCurrent == quietsEnd)
                {
                    stage = Stage::losingCaptures;
                }
                else
                {
                    auto max = std::max_element(quietsCurrent, quietsEnd);
                    std::iter_swap(quietsCurrent, max);
                    m = quietsCurrent->m;
                    ++quietsCurrent;
                    stageReturned = Stage::quiets;
                    return true;
                }
                [[fallthrough]];
            case Stage::losingCaptures:
                if (losingCapturesBegin == captureEnd)
                {
                    return false;
                }
                else
                {
                    m = losingCapturesBegin->m;
                    ++losingCapturesBegin;
                    stageReturned = Stage::losingCaptures;
                    return true;
                }
            default:
                return false;
            }
        }
    private:
        MoveOrder() {}
        Movelist<ScoredMove, 218> ml;
        decltype(ml.begin()) captureBegin = ml.begin();
        decltype(ml.end()) captureEnd = ml.end();
        decltype(ml.begin()) quietsCurrent;
        decltype(ml.begin()) quietsEnd;
        decltype(ml.begin()) losingCapturesBegin = ml.begin();
        Tables::KillerTable* kt = nullptr;
        Tables::HistoryTable* ht = nullptr;
        std::uint64_t hash = 0;
        std::size_t d = 0;
        Stage stage = Stage::hash;
        Move hashmove = 0;
        Move k1move = 0;
        Move k2move = 0;
    public:
        Stage stageReturned = Stage::none;

    };
}


#endif
