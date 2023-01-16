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

#include "board.hpp"
#include "engine.hpp"
#include "searchflags.hpp"
#include "transpositiontable.hpp"

namespace uci
{
	class UCIProtocol
	{
	public:
		void UCIStartup();
		void UCIStartLoop();
		UCIProtocol()
		{
			tt.resize((1024*1024) / sizeof(TTable::Entry));
			e.setTTable(&tt);
		};
	private:
		void UCIPositionCommand(const std::vector<std::string>&);
		void UCIGoCommand(const std::vector<std::string>&);
		void UCIStopCommand();
		void UCISetOptionCommand(const std::vector<std::string>&);
		std::string UCIName = "Captain v3.1";
		std::string UCIAuthor = "Narbeh Mouradian";
		bool initialized = false;
		board::QBB b;
		board::ExtraBoardInfo ebi;
		engine::Engine e;
		TTable::TTable tt;
		std::future<void> engineResult;
		searchFlags sf;

		friend class engine::Engine;
	};

	std::tuple<board::Move, bool> uciMove2boardMove(const board::QBB&, const std::string&, board::Color);
}


#endif
