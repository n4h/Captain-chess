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

#ifndef CHESS_CONSTANTS_H
#define CHESS_CONSTANTS_H

#include <cstdint>

namespace constants
{
	constexpr std::uint32_t fromMask = 077U;
	constexpr std::uint32_t toMask = 07700U;

	constexpr std::uint32_t fromMaskOffset = 0;
	constexpr std::uint32_t toMaskOffset = 6U;

	constexpr std::uint32_t moveTypeMask    = 0b1111U << 12;
	constexpr std::uint32_t queenPromo      = 7U;
	constexpr std::uint32_t rookPromo       = 6U;
	constexpr std::uint32_t bishopPromo     = 5U;
	constexpr std::uint32_t knightPromo     = 4U;
	constexpr std::uint32_t enPCap          = 3U;
	constexpr std::uint32_t KSCastle        = 2U;
	constexpr std::uint32_t QSCastle        = 1U;
	constexpr std::uint32_t QMove           = 0U;

	constexpr std::uint32_t moveTypeOffset = 12U;

	constexpr std::uint32_t emptySquare = 0U;
	constexpr std::uint32_t oppPawn = 0b0010U;
	constexpr std::uint32_t oppKnight = 0b0100U;
	constexpr std::uint32_t oppBishop = 0b0110U;
	constexpr std::uint32_t oppRook = 0b1000U;
	constexpr std::uint32_t oppQueen = 0b1010U;
	constexpr std::uint32_t oppKing = 0b1100U;
	constexpr std::uint32_t myPawn = 0b0011U;
	constexpr std::uint32_t myKnight = 0b0101U;
	constexpr std::uint32_t myBishop = 0b0111U;
	constexpr std::uint32_t myRook = 0b1001U;
	constexpr std::uint32_t myQueen = 0b1011U;
	constexpr std::uint32_t myKing = 0b1101U;
	constexpr std::uint32_t pawnCode = 0b001U;
	constexpr std::uint32_t knightCode = 0b010U;
	constexpr std::uint32_t bishopCode = 0b011U;
	constexpr std::uint32_t rookCode = 0b100U;
	constexpr std::uint32_t queenCode = 0b101U;
	constexpr std::uint32_t kingCode = 0b110U;
}
#endif