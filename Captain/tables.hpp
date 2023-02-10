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

#ifndef TRANSPOSITION_TABLE_H
#define TRANSPOSITION_TABLE_H

#include <cstdint>
#include <array>

#include "board.hpp"
#include "eval.hpp"

namespace TTable
{
	enum : char {NONE = 0, PV = 1, ALL = 2, CUT = 3};
	using eval::Eval;
	
	struct Entry
	{
		std::uint64_t key = 0;
		std::int16_t depth = 0;
		Eval eval = 0;
		board::Move move = 0;
		char nodeType = NONE;
		unsigned char age = 0;
	};
	
	class TTable
	{
		Entry* table = nullptr;
		std::size_t sz = 0;

		void initRandom();
		static bool isBetterEntry(const Entry& curr, std::int16_t depth, unsigned char age);
	public:
		std::array<std::array<std::uint64_t, 64>, 6> whitePSQT;
		std::array<std::array<std::uint64_t, 64>, 6> blackPSQT;
		std::uint64_t wToMove;
		std::array<std::uint64_t, 4> castling_first;
		std::array<std::uint64_t, 16> castling;
		std::array<std::uint64_t, 8> enPassant;

		TTable(std::size_t);
		TTable() { initRandom(); }
		~TTable();
		
		void clear();
		
		void tryStore(std::uint64_t hash, std::int16_t depth, Eval eval, board::Move m, char nodetype, unsigned char age);
		void store(std::uint64_t hash, std::int16_t depth, Eval eval, board::Move m, char nodetype, unsigned char age);

		Entry& operator[](std::uint64_t hash) noexcept;

		void resize(std::size_t);
		std::uint64_t initialHash(const board::QBB&);
		std::uint64_t incrementalUpdate(board::Move, const board::QBB&, const board::QBB&);
		std::uint64_t nullUpdate(const board::QBB&);

		double capturePct(const board::QBB& b) const;
		double nodeTypePct(char nodetype) const;
		double usedPct() const;
		TTable(const TTable&) = delete;
		TTable& operator=(const TTable&) = delete;
		TTable(TTable&&) = delete;
		TTable& operator=(TTable&&) = delete;
	};

}
#endif