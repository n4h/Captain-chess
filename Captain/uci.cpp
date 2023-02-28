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

#include "uci.hpp"
#include "board.hpp"
#include "movegen.hpp"
#include "auxiliary.hpp"
#include "constants.hpp"
#include "divide.hpp"

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
                moves.clear();
                pos.clear();
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
        }
    }

    void UCIProtocol::UCIPositionCommand(const std::vector<std::string>& command)
    {
        if (!initialized)
        {
            initialized = true;
        }
        moves.clear();
        pos.clear();
        if (command[1] == "startpos")
        {
            b = board::QBB{ "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"};
            pos.push_back(Tables::tt.initialHash(b));
            if (command.size() >= 3 && command[2] == "moves")
            {
                for (std::size_t i = 3; i < command.size(); ++i)
                {
                    auto [move, halfMove] = uciMove2boardMove(b, command[i], b.getColorToPlay());
                    b.makeMove(move);
                    moves.push_back(move);
                    pos.push_back(Tables::tt.initialHash(b));
                }
            }
        }
        else if (command[1] == "fen" && command.size() >= 8)
        {
            // FEN string contains 6 space separated fields
            b = board::QBB{command[2] + " " + command[3] + " " + command[4] + " " + command[5] + " "
             + command[6] + " " + command[7]};
            pos.push_back(Tables::tt.initialHash(b));
            if (command.size() >= 9 && command[8] == "moves")
            {
                for (std::size_t i = 9; i < command.size(); ++i)
                {
                    auto [move, halfMove] = uciMove2boardMove(b, command[i], b.getColorToPlay());
                    b.makeMove(move);
                    moves.push_back(move);
                    pos.push_back(Tables::tt.initialHash(b));
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

        sf.searching.test_and_set();
        auto tmp = std::async(&engine::Engine::rootSearch, &e, std::cref(b), startTime, std::cref(moves), std::cref(pos));
        engineResult = std::move(tmp);
    }

    void UCIProtocol::UCIStopCommand()
    {
        sf.searching.clear();
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

    // we're assuming that the GUI isn't sending us invalid moves
    std::tuple<board::Move, bool> uciMove2boardMove(const board::QBB& b, const std::string& uciMove, board::Color c)
    {
        auto fromFile = aux::fileNumber(uciMove[0]);
        unsigned fromRank = c == board::Color::White ? uciMove[1] - '0' - 1: 7 - (uciMove[1] - '0' - 1);
        auto toFile = aux::fileNumber(uciMove[2]);
        unsigned toRank = c == board::Color::White ? uciMove[3] - '0' - 1: 7 - (uciMove[3] - '0' - 1);
        board::Move m = aux::index(fromRank, fromFile);
        m |= aux::index(toRank, toFile) << constants::toMaskOffset;

        board::square from = static_cast<board::square>(aux::index(fromRank, fromFile));
        board::square to = static_cast<board::square>(aux::index(toRank, toFile));

        auto piecetype = b.getPieceType(from);
        auto piecetypeTo = b.getPieceType(to);
        bool incHalfClock = !(piecetypeTo || piecetype == constants::myPawn);
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
        
        return std::make_tuple(m, incHalfClock);
    }
}