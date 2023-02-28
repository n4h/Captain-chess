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

#include <intrin.h>

#pragma intrinsic(_BitScanForward64)

#include <random>
#include <cstddef>
#include <cstdlib>

#include "tables.hpp"
#include "board.hpp"


namespace Tables
{
	TTable tt{(1024 * 1024) / sizeof(Entry)};
	void TTable::initRandom()
	{
		std::random_device rd;

		std::mt19937_64 rnum{rd()};

		for (std::size_t i = 0; i != 6; ++i)
		{
			for (std::size_t j = 0; j != 64; ++j)
			{
				whitePSQT[i][j] = rnum();
				blackPSQT[i][j] = rnum();
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

	bool TTable::isBetterEntry(const Entry& curr, std::int16_t depth, unsigned char age)
	{
		if (curr.age < age)
		{
			return true;
		}

		return curr.depth < depth;
	}

	void TTable::tryStore(std::uint64_t hash, std::int16_t depth, Eval eval, board::Move m, char nodetype, unsigned char age)
	{
		const auto& currEntry = (*this)[hash];
		if (isBetterEntry(currEntry, depth, age))
		{
			store(hash, depth, eval, m, nodetype, age);
		}
	}

	void TTable::store(std::uint64_t hash, std::int16_t depth, Eval eval, board::Move m, char nodetype, unsigned char age)
	{
		(*this)[hash].key = hash;
		(*this)[hash].depth = depth;
		(*this)[hash].eval = eval;
		(*this)[hash].move = m;
		(*this)[hash].nodeType = nodetype;
		(*this)[hash].age = age;
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
			table[i].age = 0;
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

	std::uint64_t TTable::initialHash(const board::QBB& b)
	{
		std::uint64_t inithash = 0;
		
		for (std::size_t i = 0; i != 64; ++i)
		{
			auto piecetype = b.getPieceType(static_cast<board::square>(i));
			if (b.isWhiteToPlay())
			{
				if (piecetype)
				{
					if (piecetype & 1)
						inithash ^= whitePSQT[(piecetype >> 1) - 1][i];
					else
						inithash ^= blackPSQT[(piecetype >> 1) - 1][i];
				}
			}
			else
			{
				if (piecetype)
				{
					if (piecetype & 1)
						inithash ^= blackPSQT[(piecetype >> 1) - 1][aux::flip(i)];
					else
						inithash ^= whitePSQT[(piecetype >> 1) - 1][aux::flip(i)];
				}
			}
		}

		if (b.isWhiteToPlay())
			inithash ^= wToMove;

		if (b.isWhiteToPlay())
		{
			inithash ^= b.canCastleLong() ? castling_first[0] : 0;
			inithash ^= b.canCastleShort() ? castling_first[1] : 0;
			inithash ^= b.oppCanCastleLong() ? castling_first[2] : 0;
			inithash ^= b.oppCanCastleShort() ? castling_first[3] : 0;
		}
		else
		{
			inithash ^= b.oppCanCastleLong() ? castling_first[0] : 0;
			inithash ^= b.oppCanCastleShort() ? castling_first[1] : 0;
			inithash ^= b.canCastleLong() ? castling_first[2] : 0;
			inithash ^= b.canCastleShort() ? castling_first[3] : 0;
		}

		if (b.enpExists())
			inithash ^= enPassant[b.getEnpFile()];
		return inithash;
	}

	std::uint64_t TTable::incrementalUpdate(board::Move m, const board::QBB& old, const board::QBB& newb)
	{
		std::uint64_t update = 0;
		update ^= wToMove;

		const auto* myPSQT = &whitePSQT;
		const auto* oppPSQT = &blackPSQT;
		auto from = static_cast<board::square>(board::getMoveInfo<constants::fromMask>(m));
		auto to = static_cast<board::square>(board::getMoveInfo<constants::toMask>(m));
		auto fromPcType = (old.getPieceType(from) >> 1);
		auto toPcType = (old.getPieceType(to) >> 1);

		if (!old.isWhiteToPlay())
		{
			myPSQT = &blackPSQT;
			oppPSQT = &whitePSQT;
			from = static_cast<board::square>(aux::flip(from));
			to = static_cast<board::square>(aux::flip(to));
		}

		update ^= (*myPSQT)[fromPcType - 1][from];

		if (toPcType)
			update ^= (*oppPSQT)[toPcType - 1][to];

		std::size_t queenRook = old.isWhiteToPlay() ? board::a1 : board::a8;
		std::size_t kingRook = old.isWhiteToPlay() ? board::h1 : board::h8;

		switch (board::getMoveInfo<constants::moveTypeMask>(m))
		{
		case constants::QMove:
			update ^= (*myPSQT)[fromPcType - 1][to];
			break;
		case constants::QSCastle:
			update ^= (*myPSQT)[fromPcType - 1][to];
			update ^= (*myPSQT)[constants::rookCode - 1][queenRook];
			update ^= (*myPSQT)[constants::rookCode - 1][queenRook + 3];
			break;
		case constants::KSCastle:
			update ^= (*myPSQT)[fromPcType - 1][to];
			update ^= (*myPSQT)[constants::rookCode - 1][kingRook];
			update ^= (*myPSQT)[constants::rookCode - 1][kingRook - 2];
			break;
		case constants::enPCap:
			update ^= (*myPSQT)[fromPcType - 1][to];
			update ^= (*oppPSQT)[constants::pawnCode - 1][old.isWhiteToPlay() ? to - 8 : to + 8];
			break;
		case constants::knightPromo:
			update ^= (*myPSQT)[constants::knightCode - 1][to];
			break;
		case constants::bishopPromo:
			update ^= (*myPSQT)[constants::bishopCode - 1][to];
			break;
		case constants::rookPromo:
			update ^= (*myPSQT)[constants::rookCode - 1][to];
			break;
		case constants::queenPromo:
			update ^= (*myPSQT)[constants::queenCode - 1][to];
			break;
		}

		auto EPChange = _bextr_u64(old.getEp() ^ newb.getEp(), 40U, 8);
		unsigned long index = 0;
		while (_BitScanForward64(&index, EPChange))
		{
			EPChange = _blsr_u64(EPChange);
			update ^= enPassant[index];
		}
		if (old.isWhiteToPlay())
			update ^= castling[board::getCastlingDiff(old, newb)];
		else
			update ^= castling[board::getCastlingDiff(newb, old)];

		return update;
	}

	std::uint64_t TTable::nullUpdate(const board::QBB& b)
	{
		std::uint64_t update = 0;
		update ^= this->wToMove;
		auto ep = b.getEp();
		unsigned long index;
		if (_BitScanForward64(&index, ep))
		{
			update ^= enPassant[aux::file(index)];
		}
		return update;
	}

	double TTable::capturePct(const board::QBB& b) const
	{
		std::size_t total = 0;
		std::size_t count = 0;
		for (std::size_t i = 0; i != sz; ++i)
		{
			if (table[i].age)
			{
				++total;
				if (table[i].move && b.isCapture(table[i].move))
				{
					++count;
				}
			}
		}
		if (!total)
		{
			return 0;
		}
		return static_cast<double>(count * 100) / total;
	}

	double TTable::nodeTypePct(char nodetype) const
	{
		std::size_t total = 0;
		std::size_t count = 0;
		for (std::size_t i = 0; i != sz; ++i)
		{
			if (table[i].age)
			{
				++total;
				if (table[i].nodeType == nodetype)
				{
					++count;
				}
			}
		}
		if (!total)
		{
			return 0;
		}
		return static_cast<double>(count * 100) / total;
	}

	double TTable::usedPct() const
	{
		std::size_t used = 0;
		for (std::size_t i = 0; i != sz; ++i)
		{
			if (table[i].age)
				++used;
		}
		return sz ? static_cast<double>(used * 100) / sz : 0;
	}
}