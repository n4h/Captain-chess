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

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <exception>
#include <chrono>
#include <future>
#include <utility>
#include <functional>
#include <cstdlib>
#include <tuple>
#include <locale>
#include <numeric>

#include "uci.hpp"
#include "board.hpp"
#include "moves.hpp"
#include "auxiliary.hpp"
#include "constants.hpp"
#include "divide.hpp"
#include "tune.hpp"
#include "types.hpp"

namespace uci
{
    using namespace std::literals::chrono_literals;

    void UCIProtocol::UCIStartup()
    {
        uci_out << "id name " << UCIName << std::endl;
        uci_out << "id author " << UCIAuthor << std::endl;
        uci_out << "option name Hash type spin default 1 min 1 max 256" << std::endl;
        uci_out << "uciok" << std::endl;
        uci_out.emit();
    }

    void UCIProtocol::UCIStartLoop()
    {
        std::string input;
        while (std::getline(std::cin, input))
        {
            std::istringstream process{input};

            std::string term;
            std::vector<std::string> UCIMessage = {};
            while (process >> term)
            {
                UCIMessage.push_back(term);
            }
            if (UCIMessage[0] == "isready")
            {
                uci_out << "readyok" << std::endl;
                uci_out.emit();
            }
            if (UCIMessage[0] == "quit")
                std::exit(EXIT_SUCCESS);
            if (UCIMessage[0] == "setoption")
                UCISetOptionCommand(UCIMessage);
            if (UCIMessage[0] == "ucinewgame")
            {
                UCIStopCommand();
                if (!initialized)
                {
                    initialized = true;
                }
                Tables::tt.clear();
                e.newGame();
            }
            if (UCIMessage[0] == "position" && UCIMessage.size() >= 2)
            {
                UCIStopCommand();
                UCIPositionCommand(UCIMessage);
            }
            if (UCIMessage[0] == "go" && UCIMessage.size() >= 2)
            {
                UCIStopCommand();
                UCIGoCommand(UCIMessage);
            }
            if (UCIMessage[0] == "stop")
                UCIStopCommand();
            if (UCIMessage[0] == "tune")
            {
                Tune(std::stod(UCIMessage[1]), 
                    std::stod(UCIMessage[2]), 
                    std::stoi(UCIMessage[3]), 
                    std::stoi(UCIMessage[4]), 
                    UCIMessage[5]);
            }
            if (UCIMessage[0] == "tune2")
            {
                Tune(UCIMessage[1]);
            }
        }
    }

    void UCIProtocol::UCIPositionCommand(const std::vector<std::string>& command)
    {
        if (!initialized)
        {
            initialized = true;
        }
        if (command[1] == "startpos")
        {
            b = board::Board{ "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"};
            if (command.size() >= 3 && command[2] == "moves")
            {
                for (std::size_t i = 3; i < command.size(); ++i)
                {
                    auto move = uciMove2boardMove(b, command[i]);
                    b.makeMove(move);
                }
            }
        }
        else if (command[1] == "fen" && command.size() >= 8)
        {
            b = board::Board{command[2] + " " + command[3] + " " + command[4] + " " + command[5] + " "
             + command[6] + " " + command[7]};
            if (command.size() >= 9 && command[8] == "moves")
            {
                for (std::size_t i = 9; i < command.size(); ++i)
                {
                    auto move = uciMove2boardMove(b, command[i]);
                    b.makeMove(move);
                }
            }
        }
    }

    void UCIProtocol::UCIGoCommand(const std::vector<std::string>& command)
    {
        const auto startTime = std::chrono::steady_clock::now();
        engine::SearchSettings ss;

        //command[0] is "go"
        for (std::size_t index = 0; auto& i : command)
        {
            if (i == "depth")
                ss.maxDepth = (std::size_t)std::stoi(command[index + 1]);
            else if (i == "time")
                ss.maxTime = std::chrono::milliseconds(std::stoi(command[index + 1]));
            else if (i == "wtime")
                ss.wmsec = std::chrono::milliseconds(std::stoi(command[index + 1]));
            else if (i == "btime")
                ss.bmsec = std::chrono::milliseconds(std::stoi(command[index + 1]));
            else if (i == "winc")
                ss.winc = std::chrono::milliseconds(std::stoi(command[index + 1]));
            else if (i == "binc")
                ss.binc = std::chrono::milliseconds(std::stoi(command[index + 1]));
            else if (i == "movetime")
                ss.maxTime = std::chrono::milliseconds(std::stoi(command[index + 1]));
            else if (i == "nodes")
                ss.maxNodes = (std::size_t)std::stoi(command[index + 1]);
            else if (i == "wtime")
                ss.wmsec = std::chrono::milliseconds(std::stoi(command[index + 1]));
            else if (i == "movestogo")
                ss.movestogo = (std::size_t)std::stoi(command[index + 1]);
            else if (i == "ponder")
                ss.ponder = true;
            else if (i == "infinite")
                ss.infiniteSearch = true;
            else if (i == "perft")
            {
                auto start = std::chrono::steady_clock::now();
                divide::perftDivide(b, std::stoi(command[index + 1]));
                auto end = std::chrono::steady_clock::now();
                std::chrono::milliseconds time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
                uci_out << "Time: " << time << std::endl;
                uci_out.emit();
                return;
            }
            ++index;
        }
        e.setSettings(ss);

        SearchFlags::searching.test_and_set();
        auto tmp = std::async(&engine::Engine::rootSearch, &e, b, startTime);
        engineResult = std::move(tmp);
    }

    void UCIProtocol::UCIStopCommand()
    {
        SearchFlags::searching.clear();
        if (engineResult.valid())
            engineResult.get();
    }

    void UCIProtocol::UCISetOptionCommand(const std::vector<std::string>& command)
    {
        for (std::size_t index = 0; const auto& i : command)
        {
            if (i == "name")
                if (command[index + 1] == "Hash" && command[index + 2] == "value")
                {
                    Tables::tt.resize( (1024*1024*std::stoi(command[index + 3])) / sizeof(Tables::Entry));
                    Tables::tt.clear();
                }
            ++index;
        }
    }

    void UCIProtocol::Tune(double mutation, double selectivity, std::size_t popsize, std::size_t gens, std::string file)
    {
        TestPositions EPDsuite;
        EPDsuite.loadPositions(file);

        std::vector<std::pair<eval::Evaluator, std::uint64_t>> initialPop{ popsize };

        for (eval::EvaluatorGeneticOps ego; auto& i : initialPop)
        {
            ego.mutate(i.first, 0.80);
        }
        Tuning::GeneticTuner t{ initialPop };

        uci_out << "Current score " << EPDsuite.score(eval::Evaluator{}) << std::endl;
        uci_out << "Positions count " << EPDsuite.positions.size() << std::endl;
        uci_out.emit();
        std::this_thread::sleep_for(std::chrono::seconds{ 10 });

        t.tune(mutation, selectivity, gens, [&EPDsuite](const eval::Evaluator& e) {
            return EPDsuite.score(e);
            });

        const auto [evaluator, fitness] = t.get_historical_best();

        std::ofstream output{ std::string("finalevaluator.txt"), std::ios::app };
        output << evaluator.asString();
        uci_out << "fitness " << fitness << std::endl;
        uci_out.emit();
    }

    void UCIProtocol::Tune(std::string filename)
    {
        TestPositions EPDSuite;
        EPDSuite.loadScoredPositions(filename);
        
        auto error = [&EPDSuite](eval::Evaluator ev, double k) {
            double N = EPDSuite.scoredPositions.size();
            double sum = std::transform_reduce(std::execution::par,
                EPDSuite.scoredPositions.cbegin(),
                EPDSuite.scoredPositions.cend(),
                0.0,
                std::plus<>(), 
                [&ev, &k](const auto& x) -> double {
                    const auto& pos = x.first;
                    const auto score = x.second;
                    engine::SearchSettings ss;
                    ss.quiet = true;
                    ss.ignoreSearchFlags = true;
                    engine::Engine eng{};
                    eng.setEvaluator(ev);
                    eng.setSettings(ss);
                    eng.newGame();
                    eng.newSearch(pos, std::chrono::steady_clock::now());
                    SearchFlags::searching.test_and_set();
                    double tmp = score - aux::sigmoid(k, eng.quiesceSearch(engine::rootMinBound, engine::rootMaxBound, 0));
                    return tmp * tmp;
                });
            return sum / N;
        };

        const double K = Tuning::find_best_K(eval::Evaluator{}, error);
        uci_out << "best K " << K << std::endl;
        uci_out.emit();

        std::ofstream output{ std::string("finalevaluator.txt"), std::ios::app };
        auto best = Tuning::local_search_one_iteration(eval::Evaluator{}, error, K);

        double olderror = best.second;
        
        do
        {
            olderror = best.second;
            for (std::size_t i = 0; i != 10; ++i)
            {
                best = Tuning::local_search_one_iteration(best.first, error, K);
            }
            output << best.first.asString();
            uci_out << "error " << best.second << std::endl;
            uci_out.emit();
        } while (best.second < olderror);
        uci_out << "done tuning" << std::endl;
        uci_out.emit();
    }

    // we're assuming that the GUI isn't sending us invalid moves
    Move uciMove2boardMove(const board::QBB& b, const std::string& uciMove)
    {
        auto fromFile = aux::fileNumber(uciMove[0]);
        unsigned fromRank = b.isWhiteToPlay() ? uciMove[1] - '0' - 1 : 7 - (uciMove[1] - '0' - 1);
        auto toFile = aux::fileNumber(uciMove[2]);
        unsigned toRank = b.isWhiteToPlay() ? uciMove[3] - '0' - 1 : 7 - (uciMove[3] - '0' - 1);

        Move m = aux::index(fromRank, fromFile);
        m |= aux::index(toRank, toFile) << constants::toMaskOffset;

        board::square from = static_cast<board::square>(aux::index(fromRank, fromFile));
        board::square to = static_cast<board::square>(aux::index(toRank, toFile));

        auto piecetype = b.getPieceType(from);
        if (piecetype == constants::myPawn)
        {
            if (_tzcnt_u64(b.getEp()) == to)
                m |= constants::enPCap << constants::moveTypeOffset;
            else if (toRank == 7 && uciMove.size() == 5)
                m |= board::getPromoType(board::char2pieceType(uciMove[4])) << constants::moveTypeOffset;
        }
        else if (piecetype == constants::myKing)
        {
            if (from == board::e1 && to == board::g1)
                m |= constants::KSCastle << constants::moveTypeOffset;
            else if (from == board::e1 && to == board::c1)
                m |= constants::QSCastle << constants::moveTypeOffset;
        }
        
        return m;
    }

    void TestPositions::loadPositions(std::string filename)
    {
        std::ifstream test{ filename };
        std::string input;
        while (std::getline(test, input))
        {
            std::vector<std::string> pos = {};
            std::istringstream iss{ input };
            std::string term;
            while (iss >> term)
                pos.push_back(term);

            std::string fen = pos[0] + " " + pos[1] + " " + pos[2] + " " + pos[3];

            board::QBB b{ fen, false };

            if (board::validPosition(b))
            {
                std::vector<Move> ml = { uciMove2boardMove(b, pos[6]) };
                /*
                for (std::size_t i = 5; i != pos.size(); ++i)
                {
                    ml.push_back(SAN2ucimove(b, pos[i]));
                }
                */
                assert(ml.size() > 0);
                positions.push_back(std::make_pair(b, ml));
            }
        }
    }

    void TestPositions::loadScoredPositions(std::string filename)
    {
        std::ifstream test{ filename };
        std::string input;
        while (std::getline(test, input))
        {
            std::vector<std::string> pos = {};
            std::istringstream iss{ input };
            std::string term;
            while (iss >> term)
                pos.push_back(term);

            std::string fen = pos[0] + " " + pos[1] + " " + pos[2] + " " + pos[3];

            board::QBB b{ fen, false };

            if (board::validPosition(b))
            {
                std::string pgneq = "pgn=";
                auto first = pos[8].find_first_not_of(pgneq);
                assert(pos[8][first] == '0' || pos[8][first] == '1');
                double score = std::stod(pos[8].substr(first));
                scoredPositions.push_back(std::make_pair(b, score));
            }
        }
    }

    std::uint64_t TestPositions::score(const eval::Evaluator& e) const
    {
        engine::Engine eng;
        engine::SearchSettings ss;
        ss.maxDepth = 2;
        ss.quiet = true;
        eng.setSettings(ss);
        eng.setEvaluator(e);
        std::uint64_t mistakes = 0;
        for (std::size_t count = 0; const auto& [pos, ml] : positions)
        {
            eng.newGame();
            SearchFlags::searching.test_and_set();
            eng.rootSearch(pos, std::chrono::steady_clock::now());
            auto bestmove = eng.rootMoves[0].m;
            bool found = false;
            for (auto i : ml)
            {
                if (i == bestmove)
                    found = true;
            }
            if (!found)
                ++mistakes;
            ++count;
            (void)count;
        }
        return mistakes;
    }
}