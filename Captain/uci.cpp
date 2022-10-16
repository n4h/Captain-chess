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

// from Stockfish
std::ostream& operator<<(std::ostream& o, SyncCout s)
{
	static std::mutex m;
	if (s == ioLock)
		m.lock();
	if (s == ioUnlock)
		m.unlock();
	return o;
}
namespace uci
{
	using namespace std::literals::chrono_literals;

	void UCIProtocol::UCIStartup()
	{
		sync_cout << "id name " << UCIName << sync_endl;
		sync_cout << "id author " << UCIAuthor << sync_endl;
		sync_cout << "option name Hash type spin default 1 min 1 max 32" << sync_endl;
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
					movegen::initAttacks();
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
			movegen::initAttacks();
			e.setTTable(&tt);
			initialized = true;
		}
		e.setTTable(&tt);

		if (command[1] == "startpos")
		{
			b = board::Board{ "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1" };
			if (command.size() >= 3 && command[2] == "moves")
			{
				std::vector<board::Move> mvs = {};
				for (std::size_t i = 3; i < command.size(); ++i)
				{
					mvs.push_back(uciMove2boardMove(b, command[i]));
				}
				b.playMoves(mvs);
			}
		}
		else if (command[1] == "fen" && command.size() >= 8)
		{
			// FEN string contains 6 space separated fields
			b = board::Board{command[2] + " " + command[3] + " " + command[4] + " " + command[5] + " "
			 + command[6] + " " + command[7]};

			if (command.size() >= 9 && command[8] == "moves")
			{
				std::vector<board::Move> mvs = {};
				for (std::size_t i = 9; i < command.size(); ++i)
				{
					mvs.push_back(uciMove2boardMove(b, command[i]));
				}
				b.playMoves(mvs);
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
	// such as a pawn on the 6th rank moving to the 3rd rank.
	// If the GUI is sending us garbage moves, there is no really
	// good way to deal with it - either the GUI is buggy, or 
	// the we are unsynchronized with the GUI, in which case we
	// will be re-synchronized upon the next position command
	board::Move uciMove2boardMove(const board::Board& b, const std::string& uciMove)
	{
		if (!aux::isMove(uciMove))
			return 0;

		board::Move m = b.getHeading();

		const auto fromIndex = aux::index((unsigned)(uciMove[1] - '0') - 1U, aux::fileNumber(uciMove[0]));
		const auto toIndex = aux::index((unsigned)(uciMove[3] - '0') - 1U, aux::fileNumber(uciMove[2]));
		const auto from = aux::setbit(fromIndex);
		const auto to = aux::setbit(toIndex);
		const auto fromPieceType = b.getPieceType(from);
		const auto toPieceType = b.getPieceType(to);

		m |= fromIndex;
		m |= toIndex << constants::toMaskOffset;

		// Promo moves
		if (uciMove.size() >= 5 && aux::isPiece(uciMove[4]))
		{
			board::pieceType promoType = board::char2pieceType(uciMove[4]);

			m |= board::getPromoType(promoType, toPieceType);
			return m;
		}

		// pawn moves
		if (fromPieceType == board::pawns)
		{
			if (aux::rank(fromIndex) == 1 || aux::rank(fromIndex) == 6)
			{
				if (aux::rank(toIndex) == 3 || aux::rank(toIndex) == 4)
				{
					m |= constants::dblPawnMove;
					return m;
				}
			}
			if (to == b.epLoc)
			{
				m |= constants::enPCap;
				return m;
			}
			m |= board::getCapType(toPieceType);
			return m;
		}

		// king moves
		if (fromPieceType == board::king)
		{
			if ((from == board::square::e1 && to == board::square::g1)
				|| (from == board::square::e8 && to == board::square::g8))
			{
				m |= constants::KSCastle;
				return m;
			}
			if ((from == board::square::e1 && to == board::square::c1)
				|| (from == board::square::e8 && to == board::square::c8))
			{
				m |= constants::QSCastle;
				return m;
			}
			m |= board::getCapType(toPieceType);
			return m;
		}

		m |= board::getCapType(toPieceType);
		return m;
	}
}