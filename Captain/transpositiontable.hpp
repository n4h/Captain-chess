#ifndef TRANSPOSITION_TABLE_H
#define TRANSPOSITION_TABLE_H

#include <cstdint>

namespace TTable
{
	struct Entry
	{
		// upper 2 bits store node type
        // lower 6 bits store from and to squares
		std::uint8_t typefromTo = 0;
		std::uint8_t depth = 0;
		std::int32_t eval = 0;
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
		std::uint64_t castling[4];
		std::uint64_t enPassant[8];

		TTable(std::size_t);
		TTable() { initRandom(); }
		~TTable();
		
		void clear();
		
		bool checkEntry(std::uint64_t hash) noexcept;

		Entry& operator[](std::uint64_t hash) noexcept;

		bool replace(Entry current, Entry candidate) noexcept;

		void resize(std::size_t);

		TTable(const TTable&) = delete;
		TTable& operator=(const TTable&) = delete;
		TTable(TTable&&) = delete;
		TTable& operator=(TTable&&) = delete;
	};

}
#endif