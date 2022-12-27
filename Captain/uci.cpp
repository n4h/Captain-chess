/*
Copyright 2022, Narbeh Mouradian

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

#include "uci.hpp"
#include "board.hpp"
#include "movegen.hpp"
#include "auxiliary.hpp"
#include "constants.hpp"

namespace uci
{
	using namespace std::literals::chrono_literals;

	void UCIProtocol::UCIStartup()
	{
		sync_cout << "id name " << UCIName << sync_endl;
		sync_cout << "id author " << UCIAuthor << sync_endl;
		sync_cout << "option name Hash type spin default 1 min 1 max 256" << sync_endl;
		sync_cout << "uciok" << sync_endl;
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
				sync_cout << "readyok" << sync_endl;
			if (UCIMessage[0] == "quit")
				std::exit(EXIT_SUCCESS);
			if (UCIMessage[0] == "setoption")
				UCISetOptionCommand(UCIMessage);
			if (UCIMessage[0] == "ucinewgame")
			{
				UCIStopCommand();
				if (!initialized)
				{
					e.setTTable(&tt);
					initialized = true;
				}
				tt.clear();
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
			e.setTTable(&tt);
			initialized = true;
		}
		e.setTTable(&tt);

		if (command[1] == "startpos")
		{
			b = board::QBB{ "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1" };
			if (command.size() >= 3 && command[2] == "moves")
			{
				for (std::size_t i = 3; i < command.size(); ++i)
				{
					// TODO playMoves on board (UCI)
				}
			}
		}
		else if (command[1] == "fen" && command.size() >= 8)
		{
			// FEN string contains 6 space separated fields
			b = board::QBB{command[2] + " " + command[3] + " " + command[4] + " " + command[5] + " "
			 + command[6] + " " + command[7]};

			if (command.size() >= 9 && command[8] == "moves")
			{
				for (std::size_t i = 9; i < command.size(); ++i)
				{
					// TODO play moves on board (UCI)
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
			if (i == "time")
				ss.maxTime = std::chrono::milliseconds(std::stoi(command[index + 1]));
			if (i == "wtime")
				ss.wmsec = std::chrono::milliseconds(std::stoi(command[index + 1]));
			if (i == "btime")
				ss.bmsec = std::chrono::milliseconds(std::stoi(command[index + 1]));
			if (i == "winc")
				ss.winc = std::chrono::milliseconds(std::stoi(command[index + 1]));
			if (i == "binc")
				ss.binc = std::chrono::milliseconds(std::stoi(command[index + 1]));
			if (i == "nodes")
				ss.maxNodes = (std::size_t)std::stoi(command[index + 1]);
			if (i == "wtime")
				ss.wmsec = std::chrono::milliseconds(std::stoi(command[index + 1]));
			if (i == "movestogo")
				ss.movestogo = (std::size_t)std::stoi(command[index + 1]);
			if (i == "ponder")
				ss.ponder = true;
			if (i == "infinite")
				ss.infiniteSearch = true;
			++index;
		}
		e.setSettings(ss);

		sf.searching.test_and_set();
		auto tmp = std::async(&engine::Engine::playBestMove, &e, std::cref(b), startTime);
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
		for (std::size_t index = 0; auto i : command)
		{
			if (i == "name")
				if (command[index + 1] == "Hash" && command[index + 2] == "value")
				{
					tt.resize( (1024*1024*std::stoi(command[index + 3])) / sizeof(TTable::Entry));
					tt.clear();
				}
			++index;
		}
	}

	// we're assuming that the GUI isn't sending us garbage moves
	board::Move uciMove2boardMove(const board::QBB& b, const std::string& uciMove)
	{
		
		// TODO rewrite uciMove2boardMove
		if (uciMove == "a")
			return 0;
		else
			return static_cast<board::Move>(b.epc);
	}
}