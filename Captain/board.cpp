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
#include <cstdlib>

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
	QBB::QBB(const std::string& fen, ExtraBoardInfo& EBI)
	{
		unsigned int currFile = 0;
		
		auto splitfen = splitString(fen, ' ');
		auto splitboard = splitString(splitfen[0], '/');

		bool wToMove = splitfen[1] == "w";

		if (wToMove)
			EBI.initialMover = Color::White;
		else
			EBI.initialMover = Color::Black;

		for (unsigned int i = 0; i != 8; ++i)
		{
			for (auto j : splitboard[i])
			{
				if (isPiece(j))
				{
					auto [color, pieceType] = makeSquare(j);

					switch (pieceType)
					{
					case pawns:
						pbq |= setbit(7 - i, currFile);
						break;
					case knights:
						nbk |= setbit(7 - i, currFile);
						break;
					case bishops:
						pbq |= setbit(7 - i, currFile);
						nbk |= setbit(7 - i, currFile);
						break;
					case rooks:
						rqk |= setbit(7 - i, currFile);
						break;
					case queens:
						pbq |= setbit(7 - i, currFile);
						rqk |= setbit(7 - i, currFile);
						break;
					case king:
						rqk |= setbit(7 - i, currFile);
						nbk |= setbit(7 - i, currFile);
						break;
					}
					if (wToMove == color)
						side |= setbit(7 - i, currFile);

					currFile = incFile(currFile, 1);
				}
				else if (isNumber(j))
				{
					currFile = incFile(currFile, j - '0');
				}
			}
		}

		for (auto i : splitfen[2])
		{
			if (i == 'K') epc |= setbit(e1) | setbit(h1);
			if (i == 'k') epc |= setbit(e8) | setbit(h8);
			if (i == 'q') epc |= setbit(e8) | setbit(a8);
			if (i == 'Q') epc |= setbit(e1) | setbit(a1);
		}

		if (splitfen[3] != "-")
		{
			const std::size_t file = fileNumber(splitfen[3][0]);
			const std::size_t rank = splitfen[3][1] - '0' - 1;
			epc |= setbit(rank, file);
		}

		//if (splitfen[1] == "w") wMoving = true;
		if (!wToMove) flipQBB();

		EBI.halfMoves = (std::uint16_t)(std::stoi(splitfen[4]));
		EBI.moveNumber = std::stoi(splitfen[5]);
	}

	// adapted from https://www.chessprogramming.org/AVX2#VerticalNibble
	unsigned QBB::getPieceType(square s) const noexcept
	{
		__m256i qbb = _mm256_set_epi64x(rqk, nbk, pbq, side);
		__m128i  shift = _mm_cvtsi32_si128(s ^ 63U);
		qbb = _mm256_sll_epi64(qbb, shift);
		std::uint32_t qbbsigns = _mm256_movemask_epi8(qbb);
		return _pext_u32(qbbsigns, 0x80808080);
	}

	void QBB::flipQBB()
	{
		side = _byteswap_uint64(side);
		pbq = _byteswap_uint64(pbq);
		nbk = _byteswap_uint64(nbk);
		rqk = _byteswap_uint64(rqk);
		epc = _byteswap_uint64(epc);
	}

	void QBB::makeMove(const Move m)
	{
		const auto fromSq = getMoveInfo<fromMask>(m);
		const auto toSq = getMoveInfo<toMask>(m);
		const auto fromBB = setbit(fromSq);
		const auto toBB = setbit(toSq);
		const auto fromPcType = getPieceType(static_cast<square>(fromSq)) >> 1;
		side &= ~fromBB;
		side |= toBB;
		
		pbq &= ~toBB;
		pbq |= toBB * ((fromBB & pbq) >> fromSq);
		pbq ^= (fromBB & pbq);

		nbk &= ~toBB;
		nbk |= toBB * ((fromBB & nbk) >> fromSq);
		nbk ^= (fromBB & nbk);

		rqk &= ~toBB;
		rqk |= toBB * ((fromBB & rqk) >> fromSq);
		rqk ^= (fromBB & rqk);

		epc &= ~fromBB;
		epc &= ~toBB;
		constexpr Bitboard rank6 = 0x00'00'FF'00'00'00'00'00U;
		epc &= ~rank6;

		constexpr Bitboard rank3 = 0x00'00'00'00'00'FF'00'00U;
		
		const Bitboard enPassant = (rank3 & (fromBB << 8) & (toBB >> 8)) << 8 * (fromPcType - 1);
		epc |= enPassant & rank3;

		const auto moveType = getMoveInfo<moveTypeMask>(m);

		constexpr std::uint32_t promoUpdateRules[8] = { 0, 0, 0, 0, 0x00'01'01'00U, 0x00'01'00'00U, 0x01'00'01'00U, 0x01'00'00'00U };
		const std::uint32_t promoUpdate = promoUpdateRules[moveType] << file(toSq);

		rqk ^= _bextr_u64(promoUpdate, 24U, 8U) << 56;
		nbk ^= _bextr_u64(promoUpdate, 16U, 8U) << 56;
		pbq ^= _bextr_u64(promoUpdate, 8U, 8U) << 56;

		constexpr std::uint64_t castleUpdate = 0x00'00'00'00'00'A0'09'00U;
		rqk ^= _bextr_u64(castleUpdate, moveType * 8, 8U);
		side ^= _bextr_u64(castleUpdate, moveType * 8, 8U);

		constexpr std::uint64_t enPUpdate = 0x00'00'00'00'01'00'00'00U;
		pbq ^= _bextr_u64(enPUpdate, moveType * 8, 8U) << (file(toSq)+32);

		side = ~side & (pbq | nbk | rqk);
		flipQBB();
	}
}