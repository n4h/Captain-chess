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

#ifndef UCI_H
#define UCI_H

#include <vector>
#include <string>
#include <future>
#include <syncstream>
#include <chrono>

#include "board.hpp"
#include "engine.hpp"
#include "searchflags.hpp"
#include "tables.hpp"

namespace uci
{
    using namespace std::literals::chrono_literals;
    class UCIProtocol
    {
    public:
        void UCIStartup();
        void UCIStartLoop();
        UCIProtocol(): uci_out(std::cout) {}
    private:
        void UCIPositionCommand(const std::vector<std::string>&);
        void UCIGoCommand(const std::vector<std::string>&);
        void UCIStopCommand();
        void UCISetOptionCommand(const std::vector<std::string>&);
        void Tune(double, double, std::size_t, std::size_t, std::string);
        std::osyncstream uci_out;
        std::string UCIName = "Captain v4.0";
        std::string UCIAuthor = "Narbeh Mouradian";
        bool initialized = false;
        board::QBB b;
        engine::MoveHistory moves;
        engine::PositionHistory pos;
        engine::Engine e;
        std::future<void> engineResult;

        friend class engine::Engine;
    };

    std::tuple<board::Move, bool> uciMove2boardMove(const board::QBB&, const std::string&, board::Color);

    board::Move SAN2ucimove(const board::QBB&, const std::string&);

    struct TestPositions
    {
        using BestMoveList = std::vector<board::Move>;

        std::vector<std::pair<board::QBB, BestMoveList>> positions;

        void loadPositions(std::string filename);

        std::uint64_t score(const eval::Evaluator& e);
    };
}


#endif
