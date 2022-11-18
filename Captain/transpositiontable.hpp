#ifndef TRANSPOSITION_TABLE_H
#define TRANSPOSITION_TABLE_H

#include <cstdint>

#include "board.hpp"

namespace TTable
{
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
		std::uint64_t wPieces[6][64];
		std::uint64_t bPieces[6][64];
		std::uint64_t wToMove;
		std::uint64_t castling_first[4];
		std::uint64_t castling[16];
		std::uint64_t enPassant[8];

		TTable(std::size_t);
		TTable() { initRandom(); }
		~TTable();
		
		void clear();

		Entry& operator[](std::uint64_t hash) noexcept;

		void resize(std::size_t);

		std::uint64_t incrementalUpdatePre(board::Move m, const board::Board& b, bool nullMove);
		std::uint64_t incrementalUpdatePost(board::Move m, const board::Board& b, bool nullMove);
		TTable(const TTable&) = delete;
		TTable& operator=(const TTable&) = delete;
		TTable(TTable&&) = delete;
		TTable& operator=(TTable&&) = delete;
	};

}
#endif