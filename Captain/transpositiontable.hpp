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

namespace TTable
{
	enum : char {PV = 0, ALL = 1, CUT = 2};


	struct Entry
	{
		std::uint64_t key = 0;
		std::int32_t eval = 0;
		int depth = 0;
		char nodeType = 0; // 0 = PV (exact score), 1 = all (upper bound), 2 = cut (lower bound)
	};


	class TTable
	{
		Entry* table = nullptr;
		std::size_t sz = 0;

		void initRandom();
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

		void store(std::uint64_t hash, int depth, std::int32_t eval, char nodetype);

		Entry& operator[](std::uint64_t hash) noexcept;

		void resize(std::size_t);

		std::uint64_t incrementalUpdate(board::Move, const board::QBB&, const board::QBB&);
		std::uint64_t nullUpdate(const board::QBB&);
		TTable(const TTable&) = delete;
		TTable& operator=(const TTable&) = delete;
		TTable(TTable&&) = delete;
		TTable& operator=(TTable&&) = delete;
	};

}
#endif