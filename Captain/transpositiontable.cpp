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

	std::uint64_t TTable::incrementalUpdatePre(board::Move m, const board::QBB& b, bool nullMove)
	{
		std::uint64_t update = this->wToMove;
		if (nullMove)
			return update;
		const auto from = board::getMoveInfo<constants::fromMask>(m);
		const auto to = board::getMoveInfo<constants::toMask>(m);
		const auto fromBit = aux::setbit(from);
		const auto toBit = aux::setbit(to);
		const auto pieceType = b.getPieceType(static_cast<board::square>(from));
		const auto toPieceType = b.getPieceType(static_cast<board::square>(to));
		

		update ^= this->bPieces[pieceType][from];
		if (toPieceType != board::pieceType::none)
			update ^= this->wPieces[toPieceType][to];

		auto moveType = board::getMoveInfo<constants::moveTypeMask>(m);
		switch (moveType)
		{
		case constants::QSCastle:
			update ^= this->wPieces[board::rooks][board::a1] ^ this->wPieces[board::rooks][board::d1];
			break;
		case constants::KSCastle:
			update ^= this->wPieces[board::rooks][board::h1] ^ this->wPieces[board::rooks][board::f1];
			break;
		case constants::enPCap:
			update ^= this->bPieces[board::pawns][to - 8];
		}

		if (board::getPromoPiece(m) != board::king)
		{
			update ^= this->wPieces[board::getPromoPiece(m)][to];
		}

		return update;
	}

	std::uint64_t TTable::incrementalUpdatePost(board::Move m, const board::QBB& b, bool nullMove)
	{
		const auto mcastling = (board::getMoveInfo<constants::castleFlagMask>(m) >> constants::castleFlagsOffset);
		const auto bcastling = (b.flags & 0b1111000000U) >> 6;
		auto update = 0;
		update ^= this->castling[mcastling ^ bcastling];

		if (board::getMoveInfo<constants::enPExistsMask>(m))
			update ^= this->enPassant[board::getMoveInfo<constants::enPFileMask>(m)];
		if (nullMove)
			return update; // null move does not introduce new en passant square

		unsigned long index;
		if (_BitScanForward64(&index, b.epLoc))
		{
			update ^= this->enPassant[aux::file(index)];
		}

		return update;
	}

}