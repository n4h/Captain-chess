#include <random>
#include <cstddef>

#include "transpositiontable.hpp"


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
			castling[i] = rnum();
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
			table[i].typefromTo = 0;
			table[i].depth = 0;
			table[i].eval = 0;
		}
	}

	bool TTable::checkEntry(std::uint64_t hash) noexcept
	{
		return (*this)[hash].depth;
	}

	Entry& TTable::operator[](std::uint64_t hash) noexcept
	{
		return table[hash % sz];
	}

	bool TTable::replace(Entry current, Entry candidate) noexcept
	{
		if (candidate.depth >= current.depth)
			return true;
		return false;
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

}