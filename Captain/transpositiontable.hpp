#ifndef TRANSPOSITION_TABLE_H
#define TRANSPOSITION_TABLE_H

#include <cstdint>

#include "board.hpp"

namespace TTable
{
	struct Entry
	{
		std::int32_t eval = 0;
		std::uint16_t key = 0;
		// upper 2 bits of move store node type
		// 0 = all node
		// 1 = cut node
		// 2 = pv node
		std::uint8_t move = 0;
		std::uint8_t depth = 0;
		
		constexpr std::uint8_t getNodeType()
		{
			return move & 0b11000000U;
		}
		constexpr std::uint8_t getMove()
		{
			return move & 0b00111111U;
		}
	};


	class TTable
	{
		Entry* table = nullptr;
		std::size_t sz = 0;

		void initRandom();
	public:
		std::uint64_t wPieces[6][64];
		std::uint64_t bPieces[6][64];
		std::uint64_t wToMove;
		std::uint64_t castling[16];
		std::uint64_t enPassant[8];

		TTable(std::size_t);
		TTable() { initRandom(); }
		~TTable();
		
		void clear();
		
		bool checkEntry(std::uint64_t hash) noexcept;

		Entry& operator[](std::uint64_t hash) noexcept;

		bool replace(Entry current, Entry candidate) noexcept;

		void resize(std::size_t);

		std::uint64_t incrementalUpdate(board::Move m, const board::Board& b);

		TTable(const TTable&) = delete;
		TTable& operator=(const TTable&) = delete;
		TTable(TTable&&) = delete;
		TTable& operator=(TTable&&) = delete;
	};

}
#endif