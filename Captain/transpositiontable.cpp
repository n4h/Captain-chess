#include <intrin.h>

#pragma intrinsic(_BitScanForward64)

#include <random>
#include <cstddef>

#include "transpositiontable.hpp"
#include "board.hpp"


namespace TTable
{
	void TTable::initRandom()
	{
		std::random_device rd;

		std::mt19937_64 rnum{rd()};

		for (std::size_t i = 0; i != 6; ++i)
		{
			for (std::size_t j = 0; j != 64; ++j)
			{
				wPieces[i][j] = rnum();
				bPieces[i][j] = rnum();
			}
		}

		wToMove = rnum();

		for (std::size_t i = 0; i != 4; ++i)
		{
			castling_first[i] = rnum();
		}

		for (std::size_t i = 0; i != 16; ++i)
		{
			castling[i] = 0;
			if (i & 0b1U) castling[i] ^= castling_first[0];
			if (i & 0b10U) castling[i] ^= castling_first[1];
			if (i & 0b100U) castling[i] ^= castling_first[2];
			if (i & 0b1000U) castling[i] ^= castling_first[3];
		}

		for (std::size_t i = 0; i != 8; ++i)
		{
			enPassant[i] = rnum();
		}
	}

	TTable::TTable(std::size_t N)
	{
		if (N > 0)
		{
			table = new Entry[N];
			this->sz = N;
		}
		else
		{
			table = nullptr;
			this->sz = 0;
		}
		initRandom();
	}

	TTable::~TTable()
	{
		delete[] table;
	}

	void TTable::clear()
	{
		for (std::size_t i = 0; i != sz; ++i)
		{
			table[i].eval = 0;
			table[i].key = 0;
			table[i].depth = 0;
			table[i].nodeType = 0;
		}
	}

	Entry& TTable::operator[](std::uint64_t hash) noexcept
	{
		return table[hash % sz];
	}

	void TTable::resize(std::size_t newSize)
	{
		delete[] table;

		if (newSize > 0)
		{
			table = new Entry[newSize];
			sz = newSize;
		}
		else
		{
			table = nullptr;
			sz = 0;
		}
	}

	// TODO hash table update
	std::uint64_t TTable::incrementalUpdatePre(board::Move m, const board::QBB& b, bool nullMove)
	{
		m; b; nullMove;
	}

	std::uint64_t TTable::incrementalUpdatePost(board::Move m, const board::QBB& b, bool nullMove)
	{
		m; b; nullMove;
	}

}