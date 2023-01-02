#include <intrin.h>

#pragma intrinsic(_BitScanForward64)

#include <random>
#include <cstddef>
#include <cstdlib>

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

	void TTable::store(std::uint64_t hash, int depth, std::int32_t eval, char nodetype)
	{
		(*this)[hash].key = hash;
		(*this)[hash].depth = depth;
		(*this)[hash].eval = eval;
		(*this)[hash].nodeType = nodetype;
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

}