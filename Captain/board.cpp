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

	void Board::clearPosition(std::uint64_t pos) noexcept
	{
		wAll &= ~pos;
		bAll &= ~pos;
		all &= ~pos;
		for (int i = 0; i != 6; ++i)
		{
			wPieces[i] &= ~pos;
			bPieces[i] &= ~pos;
		}
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

	void Board::playMoves(const std::vector<Move>& v)
	{
		for (auto i : v)
		{
			if (wMoving)
			{
				makeMove<true>(i);
			}
			else
			{
				makeMove<false>(i);
			}
		}
	}

	std::ostream& operator<<(std::ostream& os, const Board& b)
	{
		// First, print the board
		// we start by printing the top border
		os << "O===============O\n";
		for (unsigned int j = 8; j >= 1; --j)
		{
			os << "|"; // left border of jth rank
			for (unsigned int i = 0; i != 8; ++i)
			{
				unsigned int capitalOffset = 0;
				const auto pos = setbit(j - 1U, i);
				if (pos & b.all)
				{
					if (pos & b.wAll) capitalOffset = 0;
					if (pos & b.bAll) capitalOffset = 32;
					if ((b.wPieces[pawns] | b.bPieces[pawns]) & pos)
						os << (char)('P' + (char)capitalOffset) << "|";
					if ((b.wPieces[knights] | b.bPieces[knights]) & pos)
						os << (char)('N' + (char)capitalOffset) << "|";
					if ((b.wPieces[bishops] | b.bPieces[bishops]) & pos)
						os << (char)('B' + (char)capitalOffset) << "|";
					if ((b.wPieces[rooks] | b.bPieces[rooks]) & pos)
						os << (char)('R' + (char)capitalOffset) << "|";
					if ((b.wPieces[queens] | b.bPieces[queens]) & pos)
						os << (char)('Q' + (char)capitalOffset) << "|";
					if ((b.wPieces[king] | b.bPieces[king]) & pos)
						os << (char)('K' + (char)capitalOffset) << "|";
				}
				else
				{
					os << "." << "|";
				}
			}
			os << "\n";
		}
		// bottom border
		os << "O===============O";
		return os;
	}

	bool operator==(const Board& l, const Board& r) noexcept
	{
		return (l.all == r.all)
			&& (l.wAll == r.wAll)
			&& (l.bAll == r.bAll)
			&& (l.flags == r.flags)
			&& (l.epLoc == r.epLoc)
			&& (l.wPieces[0] == r.wPieces[0])
			&& (l.bPieces[0] == r.bPieces[0])
			&& (l.wPieces[1] == r.wPieces[1])
			&& (l.bPieces[1] == r.bPieces[1])
			&& (l.wPieces[2] == r.wPieces[2])
			&& (l.bPieces[2] == r.bPieces[2])
			&& (l.wPieces[3] == r.wPieces[3])
			&& (l.bPieces[3] == r.bPieces[3])
			&& (l.wPieces[4] == r.wPieces[4])
			&& (l.bPieces[4] == r.bPieces[4])
			&& (l.wPieces[5] == r.wPieces[5])
			&& (l.bPieces[5] == r.bPieces[5]);
	}
	QBB::QBB(const Board& b)
	{
		qbb[0] = b.wMoving ? b.wAll : b.bAll;

		qbb[1] = b.wPieces[pawns] | b.wPieces[bishops] | b.wPieces[queens]
			| b.bPieces[pawns] | b.bPieces[bishops] | b.bPieces[queens];

		qbb[2] = b.wPieces[knights] | b.wPieces[bishops] | b.wPieces[king]
			| b.bPieces[knights] | b.bPieces[bishops] | b.bPieces[king];

		qbb[3] = b.wPieces[rooks] | b.wPieces[queens] | b.wPieces[king]
			| b.bPieces[queens] | b.bPieces[queens] | b.bPieces[king];

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
			;// TODO vertically mirror
	}
	QBB::QBB(const std::string& s) : QBB(Board{ s }) {}
}