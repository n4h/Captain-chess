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

#ifndef ENGINE_H
#define ENGINE_H

#include <climits>
#include <utility>
#include <cstdint>
#include <chrono>
#include <syncstream>
#include <forward_list>
#include <fstream>
#include <iostream>

#include "board.hpp"
#include "moves.hpp"
#include "eval.hpp"
#include "auxiliary.hpp"
#include "searchflags.hpp"
#include "tables.hpp"

namespace engine
{
    struct Timeout
    {
        Timeout() {}
    };

    using namespace std::literals::chrono_literals;
    using eval::Eval;
    using PrincipalVariation = std::forward_list<Move>;
    // 12000 is arbitrary 
    constexpr auto negInf = -12000;
    constexpr auto posInf = 12000;
    constexpr auto rootMinBound = -13000;
    constexpr auto rootMaxBound = 13000;

    struct SearchSettings
    {
        std::size_t maxDepth = std::numeric_limits<std::size_t>::max();
        std::size_t maxNodes = std::numeric_limits<std::size_t>::max();
        std::size_t movestogo = std::numeric_limits<std::size_t>::max();
        bool infiniteSearch = false;
        bool ponder = false;
        bool ignoreSearchFlags = false;
        std::chrono::milliseconds maxTime = std::chrono::milliseconds::max();
        std::chrono::milliseconds wmsec = std::chrono::milliseconds::max();
        std::chrono::milliseconds bmsec = std::chrono::milliseconds::max();
        std::chrono::milliseconds winc = 0ms;
        std::chrono::milliseconds binc = 0ms;
        bool quiet = false;
    };

    class Engine
    {
    public:
        void rootSearch(board::Board _b, std::chrono::time_point<std::chrono::steady_clock>);
        double getEval();
        Engine() :engine_out(std::cout) {}
        void setSettings(SearchSettings ss) noexcept { settings = ss; }
        void setEvaluator(const eval::Evaluator& e) { evaluate = e; }
        void newGame();
        void newSearch(board::Board, std::chrono::time_point<std::chrono::steady_clock>);
        Eval quiesceSearch(Eval alpha, Eval beta, int depth);
        Eval eval = 0;
        moves::Movelist<moves::ScoredMove> rootMoves;
    private:
        std::string move2uciFormat(const board::QBB&, Move);
        std::string getPVuciformat(board::QBB b);
        std::string getCurrline();
        std::size_t ply() const;
        bool shouldStop() noexcept;
        void uciUpdate();
        bool threeFoldRep() const;
        bool insufficientMaterial(const board::QBB&) const;
        Eval alphaBetaSearch(PrincipalVariation& pv, Eval, Eval, int, bool);
        bool isPVNode(Eval alpha, Eval beta);
        int LMR(std::size_t i, const board::QBB& before, Move m, const board::QBB& after, int currDepth, bool PV, bool isKiller);
        void printPV(const board::QBB& b);
        std::string line2string(const std::vector<Move>& moves);
        std::chrono::milliseconds elapsed() const;

        std::osyncstream engine_out;
        SearchSettings settings;
        std::chrono::time_point<std::chrono::steady_clock> searchStart;
        std::chrono::time_point<std::chrono::steady_clock> lastUpdate;
        std::size_t nodes = 0;
        std::size_t currIDdepth = 0;
        PrincipalVariation MainPV;
        std::size_t initialMove = 0;
        std::size_t initialPos = 0;
        board::Board b;
        
        bool engineW = true;
        std::chrono::milliseconds moveTime = 0ms;
        Tables::KillerTable killers;
        Tables::HistoryTable historyHeuristic;
        eval::Evaluator evaluate;
    };
}
#endif
