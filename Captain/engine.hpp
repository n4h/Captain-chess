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

#include <algorithm>
#include <limits>
#include <utility>
#include <cstdint>
#include <chrono>
#include <atomic>
#include <iterator>
#include <array>
#include <string>

#include "board.hpp"
#include "movegen.hpp"
#include "eval.hpp"
#include "auxiliary.hpp"
#include "searchflags.hpp"
#include "transpositiontable.hpp"

namespace engine
{
	struct Timeout
	{
		Timeout() {}
	};

	using namespace std::literals::chrono_literals;

	// 500000 is arbitrary 
	constexpr auto negInf = -500000;
	constexpr auto posInf = 500000;

	struct SearchSettings
	{
		std::size_t maxDepth = std::numeric_limits<std::size_t>::max();
		std::size_t maxNodes = std::numeric_limits<std::size_t>::max();
		std::size_t movestogo = std::numeric_limits<std::size_t>::max();
		bool infiniteSearch = false;
		bool ponder = false;
		std::chrono::milliseconds maxTime = std::chrono::milliseconds::max();
		std::chrono::milliseconds wmsec = 0ms;
		std::chrono::milliseconds bmsec = 0ms;
		std::chrono::milliseconds winc = 0ms;
		std::chrono::milliseconds binc = 0ms;
	};

	class Engine
	{
	public:
		void rootSearch(const board::QBB&, std::chrono::time_point<std::chrono::steady_clock>, board::ExtraBoardInfo);
		double getEval();
		Engine() {}
		void setSettings(SearchSettings ss) noexcept { settings = ss; }
		void setTTable(TTable::TTable*);
	private:
		std::string move2uciFormat(board::Move);
		SearchSettings settings;
		std::chrono::time_point<std::chrono::steady_clock> searchStart;
		std::size_t nodes = 0;
		std::size_t hash = 0;
		std::size_t currIDdepth = 0;
		board::ExtraBoardInfo ebi;
		bool engineW = true;
		std::chrono::milliseconds moveTime = 0ms;
		TTable::TTable* tt = nullptr;
		bool shouldStop() noexcept;
		void initialHash(const board::QBB&);
		// TODO 3-fold repetition
		std::int32_t quiesceSearch(const board::QBB& b, std::int32_t alpha, std::int32_t beta, int depth);

		std::int32_t alphaBetaSearch(const board::QBB& , std::int32_t, std::int32_t, int, bool);
		
		std::int32_t eval = 0;
		// 218 = current max number of moves in chess position
		// 256 = leeway for pseudolegal move generation
	};
}
#endif