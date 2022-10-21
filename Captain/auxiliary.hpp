/*
Copyright 2022, Narbeh Mouradian

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

#ifndef AUXILIARY_H
#define AUXILIARY_H

#include <intrin.h>

#pragma intrinsic(_BitScanForward64)

#include <concepts>
#include <cstdint>
#include <string_view>
#include <type_traits>


// from Stockfish
enum SyncCout { ioLock, ioUnlock };
std::ostream& operator<<(std::ostream&, SyncCout);
#define sync_cout std::cout << ioLock
#define sync_endl std::endl << ioUnlock

// auxiliary functions that have broad applicability throughout
// the codebase, such as functions for converting between array
// indices and rank and file numbers.
namespace aux
{
	using std::integral;

	template<typename T>
	concept uintOrSquare = std::unsigned_integral<T> || std::unsigned_integral<std::underlying_type_t<T>>;

	// piece values given in centipawns
	constexpr unsigned int king_val = 5000;
	constexpr unsigned int queen_val = 900;
	constexpr unsigned int knight_val = 300;
	constexpr unsigned int bishop_val = 300;
	constexpr unsigned int rook_val = 500;
	constexpr unsigned int pawn_val = 100;

	constexpr unsigned int pieceValues[7] = {pawn_val, knight_val, bishop_val, rook_val, queen_val, king_val, 0U};

	constexpr unsigned int wk_start = 4U;
	constexpr unsigned int wkr_start = 7U;
	constexpr unsigned int wqr_start = 0U;
	constexpr unsigned int bk_start = 60U;
	constexpr unsigned int bkr_start = 63U;
	constexpr unsigned int bqr_start = 56U;

	// rank = rows, file = columns. Both rank and column
    // go from 0 to 7 inclusive. This translates (rank, file)
    // to the right index on a bitboard
	constexpr std::unsigned_integral auto index(std::unsigned_integral auto rank, std::unsigned_integral auto file)
	{
		return rank * 8 + file;
	}

	constexpr std::uint64_t setbit(uintOrSquare auto pos)
	{
		return 0b1ULL << pos;
	}
	
	constexpr std::uint64_t setbit(std::unsigned_integral auto rank, std::unsigned_integral auto file)
	{
		return setbit(index(rank, file));
	}

	constexpr std::unsigned_integral auto rank(uintOrSquare auto x)
	{
		return x / 8;;
	}

	// file and rank satisfy index(rank(x),file(x)) = x;
	// so we can convert between the index of a 64 elem
	// array and the rank/file associated with that index
	constexpr std::unsigned_integral auto file(uintOrSquare auto x)
	{
		return x - 8 * rank(x);
	}

	// getDiag and getAntiDiag index into diagMask and antidiagMask
	constexpr std::unsigned_integral auto getDiag(uintOrSquare auto x)
	{
		return rank(x) + file(x);
	}

	constexpr std::unsigned_integral auto getAntiDiag(uintOrSquare auto x)
	{
		return 7U + rank(x) - file(x);
	}

	// takes and index, and returns a new index
	// after offsetting the old index by r ranks
	// and f files
	constexpr std::unsigned_integral auto index2index(uintOrSquare auto index, integral auto r, integral auto f)
	{
		return aux::index( rank(index) + r, file(index) + f);
	}
	// offset i by rank r and file f and check if it is still on the board
	constexpr bool isIndex(uintOrSquare auto i, integral auto r, integral auto f)
	{
		return !(rank(i) + r > 8 || rank(i) + r < 1 || file(i) + f > 8 || file(i) + f < 1);
	}

	// increment the file number by c. There are 8 squares per
	// row, so if file + c is greater than 8, just start counting from
	// 1 again (e.g. 6 + 2 --> 1)
	constexpr unsigned int incFile(int file, int c)
	{
		return (file + c) % 8;
	}

	// isNumber only checks if i is in the range '1'-'8'
	// use isMoveNumber for '1'-'9'
	constexpr bool isNumber(const char& i)
	{
		return (i == '1' || i == '2' || i == '3' || i == '4' || i == '5' || i == '6' || i == '7' || i == '8');
	}

	constexpr bool isMoveNumber(const char& i)
	{
		return isNumber(i) || i == '9' || i == '0';
	}

	constexpr bool isFile(const char& i)
	{
		return (i == 'a' || i == 'b' || i == 'c' || i == 'd' || i == 'e' || i == 'f' || i == 'g' || i == 'h');
	}

	constexpr bool isMove(std::string_view mv)
	{
		return mv.size() >= 4 && isFile(mv[0]) && isNumber(mv[1]) && isFile(mv[2]) && isNumber(mv[3]);
	}

	constexpr std::size_t fileNumber(const char& i)
	{
		switch (i)
		{
		case 'a':
			return 0U;
		case 'b':
			return 1U;
		case 'c':
			return 2U;
		case 'd':
			return 3U;
		case 'e':
			return 4U;
		case 'f':
			return 5U;
		case 'g':
			return 6U;
		case 'h':
			return 7U;
		default:
			return 0U;
		}
	}

	constexpr char file2char(std::size_t file)
	{
		switch (file)
		{
		case 0:
			return 'a';
		case 1:
			return 'b';
		case 2:
			return 'c';
		case 3:
			return 'd';
		case 4:
			return 'e';
		case 5:
			return 'f';
		case 6:
			return 'g';
		case 7:
			return 'h';
		default:
			return ' ';
		}
	}

	constexpr bool isPiece(const char i)
	{
		switch (i)
		{
		case 'r':
		case 'R':
		case 'k':
		case 'K':
		case 'n':
		case 'N':
		case 'q':
		case 'Q':
		case 'b':
		case 'B':
		case 'p':
		case 'P':
			return true;
		default:
			return false;
		}
	}
}
#endif