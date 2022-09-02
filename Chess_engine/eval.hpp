#ifndef EVALUATION_H
#define EVALUATION_H

#include <intrin.h>

#pragma intrinsic(__popcnt64)


#include <cstddef>
#include "auxiliary.hpp"
#include "board.hpp"

namespace eval
{
	using namespace aux;
	constexpr double PSQTknight[8][8] = {
		{0.5, 0.6, 0.7, 0.7, 0.7, 0.7, 0.6, 0.5},
		{0.6, 0.7, 1, 1, 1, 1, 0.7, 0.6},
		{0.7, 1, 1.2, 1.2, 1.2, 1.2, 1, 0.7},
		{0.7, 1, 1.2, 1.2, 1.2, 1.2, 1, 0.7},
		{0.7, 1, 1.2, 1.2, 1.2, 1.2, 1, 0.7},
		{0.7, 1, 1.2, 1.2, 1.2, 1.2, 1, 0.7},
		{0.6, 0.7, 1, 1, 1, 1, 0.7, 0.6},
		{0.5, 0.6, 0.7, 0.7, 0.7, 0.7, 0.6, 0.5}
	};

	constexpr double PSQTbishop[8][8] = {
		{0.9, 0.9, 0.9, 0.9, 0.9, 0.9, 0.9, 0.9},
		{0.9, 1.2, 1, 1, 1, 1, 1.2, 0.9},
		{0.9, 1, 1, 1.1, 1.1, 1, 1, 0.9},
		{0.9, 1, 1, 1.1, 1.1, 1, 1, 0.9},
		{0.9, 1, 1, 1.1, 1.1, 1, 1, 0.9},
		{0.9, 1, 1, 1.1, 1.1, 1, 1, 0.9},
		{0.9, 1.2, 1, 1, 1, 1, 1.2, 0.9},
		{0.9, 0.9, 0.9, 0.9, 0.9, 0.9, 0.9, 0.9}
	};

	constexpr double PSQTrookfile[8] = {1, 1, 1, 1.2, 1.2, 1, 1, 1};
	constexpr double PSQTrookrankW[8] = {1, 1, 1, 1, 1, 1, 1.4, 1.3};
	constexpr double PSQTrookrankB[8] = {1.3, 1.4, 1, 1, 1, 1, 1, 1};

	constexpr double PSQTpawnrankW[8] = {0, 1, 1, 1, 1, 1.3, 3, 0};
	constexpr double PSQTpawnrankB[8] = {0, 3, 1.3, 1, 1, 1, 1, 0};

	constexpr double PSQTqueen[8][8] = {
		{0.9, 0.9, 0.9, 0.9, 0.9, 0.9, 0.9, 0.9},
		{0.9, 0.9, 0.9, 1, 1, 0.9, 0.9, 0.9},
		{1, 1, 1, 1, 1, 1, 1, 1},
		{1, 1, 1, 1, 1, 1, 1, 1},
		{1, 1, 1, 1, 1, 1, 1, 1},
		{1, 1, 1, 1, 1, 1, 1, 1},
		{0.9, 0.9, 0.9, 1, 1, 0.9, 0.9, 0.9},
		{0.9, 0.9, 0.9, 0.9, 0.9, 0.9, 0.9, 0.9}
	};

	constexpr double PSQTking[8][8] = {
		{0, 0, 0, -0.1, -0.1, 0, 0, 0},
		{-2, -2, -2, -2, -2, -2, -2, -2},
		{-2, -2, -2, -2, -2, -2, -2, -2},
		{-2, -2, -2, -2, -2, -2, -2, -2},
		{-2, -2, -2, -2, -2, -2, -2, -2},
		{-2, -2, -2, -2, -2, -2, -2, -2},
		{-2, -2, -2, -2, -2, -2, -2, -2},
		{0, 0, 0, -0.1, -0.1, 0, 0, 0}
	};

	constexpr double PSQTkingend[8][8] = {
	{-0.1, -0.1, -0.1, 0, 0, -0.1, -0.1, -0.1},
	{2, 2, 2.2, 2.2, 2.2, 2.2, 2, 2},
	{2, 2, 2.2, 2.2, 2.2, 2.2, 2, 2},
	{2, 2, 2.2, 2.2, 2.2, 2.2, 2, 2},
	{2, 2, 2.2, 2.2, 2.2, 2.2, 2, 2},
	{2, 2, 2.2, 2.2, 2.2, 2.2, 2, 2},
	{2, 2, 2.2, 2.2, 2.2, 2.2, 2, 2},
	{-0.1, -0.1, -0.1, 0, 0, -0.1, -0.1, -0.1}
	};

	template<bool wToMove>
	std::int32_t evaluate(const board::Board& b)
	{
		const auto wsum = __popcnt64(b.wPieces[0]) * pawn_val
			+ __popcnt64(b.wPieces[1]) * knight_val
			+ __popcnt64(b.wPieces[2]) * bishop_val
			+ __popcnt64(b.wPieces[3]) * rook_val
			+ __popcnt64(b.wPieces[4]) * queen_val;

		const auto bsum = __popcnt64(b.bPieces[0]) * pawn_val
			+ __popcnt64(b.bPieces[1]) * knight_val
			+ __popcnt64(b.bPieces[2]) * bishop_val
			+ __popcnt64(b.bPieces[3]) * rook_val
			+ __popcnt64(b.bPieces[4]) * queen_val;

		if constexpr (wToMove)
		{
			return (std::int32_t)(wsum - bsum);
		}
		else
		{
			return (std::int32_t)(bsum - wsum);
		}
	}

	unsigned int getCaptureValue(board::Move m);
}
#endif