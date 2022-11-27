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

#include <string>
#include <iostream>
#include <cctype>
#include <vector>
#include <tuple>

#include "board.hpp"
#include "auxiliary.hpp"
#include "constants.hpp"
#include "transpositiontable.hpp"

namespace board
{
	using namespace aux;
	using namespace constants;

	std::tuple<bool, unsigned int> makeSquare(const char i)
	{
		bool w = i >= 'A' && i <= 'Z';

		switch (i)
		{
		case 'K':
		case 'k':
			return std::make_tuple(w, king);
		case 'Q':
		case 'q':
			return std::make_tuple(w, queens);
		case 'R':
		case 'r':
			return std::make_tuple(w, rooks);
		case 'B':
		case 'b':
			return std::make_tuple(w, bishops);
		case 'N':
		case 'n':
			return std::make_tuple(w, knights);
		case 'P':
		case 'p':
			return std::make_tuple(w, pawns);
		default:
			return std::make_tuple(w, 50); // generates out of bounds error
		}
	}

	std::vector<std::string> splitString(std::string s, const char d)
	{
		// TODO better string split
		std::vector<std::string> split = {};
		std::string word = "";
		for (auto i = s.cbegin(); i != s.cend(); ++i)
		{
			if (*i != d)
			{
				word += *i;
				continue;
			}
			if (word != "") split.push_back(word);
			word = "";
		}
		if (word != "") split.push_back(word);
		return split;
	}

	// see https://en.wikipedia.org/wiki/Forsyth%E2%80%93Edwards_Notation
	Board::Board(std::string fen)
	{
		unsigned int currFile = 0;
		
		auto splitfen = splitString(fen, ' ');
		auto splitboard = splitString(splitfen[0], '/');

		for (unsigned int i = 0; i != 8; ++i)
		{
			for (auto j : splitboard[i])
			{
				if (isPiece(j))
				{
					auto [color, pieceType] = makeSquare(j);
					all |= setbit(7 - i, currFile);
					if (color)
					{
						wPieces[pieceType] |= setbit(7 - i, currFile);
						wAll |= setbit(7 - i, currFile);
					}
					else
					{
						bPieces[pieceType] |= setbit(7 - i, currFile);
						bAll |= setbit(7 - i, currFile);
					}
					currFile = incFile(currFile, 1);
				}
				else if (isNumber(j))
				{
					currFile = incFile(currFile, j - '0');
				}
			}
		}

		if (splitfen[1] == "w") wMoving = true;
		if (splitfen[1] == "b") wMoving = false;

		for (auto i : splitfen[2])
		{
			if (i == 'K') flags |= wkCastleFlagMask;
			if (i == 'k') flags |= bkCastleFlagMask;
			if (i == 'q') flags |= bqCastleFlagMask;
			if (i == 'Q') flags |= wqCastleFlagMask;
		}

		if (splitfen[3] != "-")
		{
			const std::size_t file = fileNumber(splitfen[3][0]);
			const std::size_t rank = splitfen[3][1] - '0' - 1;
			epLoc = setbit(rank, file);
		}

		flags += (std::uint16_t)(std::stoi(splitfen[4]));
		currMove = std::stoi(splitfen[5]);
	}

	pieceType Board::getPieceType(Bitboard at) const noexcept
	{
		for (unsigned int i = 0; i != 6; ++i)
		{
			if (((wPieces[i] & at) != 0) || ((bPieces[i] & at) != 0))
				return (pieceType)i;
		}
		return none;
	}

	QBB::QBB(const Board& b)
	{
		std::uint64_t qbbArray[4] = {0, 0, 0, 0};
		qbbArray[0] = b.wMoving ? b.wAll : b.bAll;

		qbbArray[1] = b.wPieces[pawns] | b.wPieces[bishops] | b.wPieces[queens]
			| b.bPieces[pawns] | b.bPieces[bishops] | b.bPieces[queens];

		qbbArray[2] = b.wPieces[knights] | b.wPieces[bishops] | b.wPieces[king]
			| b.bPieces[knights] | b.bPieces[bishops] | b.bPieces[king];

		qbbArray[3] = b.wPieces[rooks] | b.wPieces[queens] | b.wPieces[king]
			| b.bPieces[rooks] | b.bPieces[queens] | b.bPieces[king];

		qbb = _mm256_set_epi64x(qbbArray[3], qbbArray[2], qbbArray[1], qbbArray[0]);

		epc = b.epLoc;

		if (b.flags & Board::wkCastleFlagMask)
			epc |= setbit(e1) | setbit(h1);
		if (b.flags & Board::wqCastleFlagMask)
			epc |= setbit(e1) | setbit(a1);
		if (b.flags & Board::bkCastleFlagMask)
			epc |= setbit(e8) | setbit(h8);
		if (b.flags & Board::bqCastleFlagMask)
			epc |= setbit(e8) | setbit(a8);

		if (!b.wMoving)
			this->flipQBB();
	}
	QBB::QBB(const std::string& s) : QBB(Board{ s }) {}
}