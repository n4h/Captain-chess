#ifndef MOVEGEN_H
#define MOVEGEN_H

#include <immintrin.h>
#include <intrin.h>

#pragma intrinsic(__popcnt64)
#pragma intrinsic(_BitScanForward64)
#pragma intrinsic(_pdep_u64)
#pragma intrinsic(_pext_u64)


#include <cstdint>
#include <cstddef>
#include <type_traits>
#include <cassert>

#include "board.hpp"
#include "auxiliary.hpp"
#include "constants.hpp"

namespace movegen
{
	using attackMap = std::uint64_t;

	constexpr bool kingSideCastle = true;
	constexpr bool queenSideCastle = false;

	constexpr std::size_t bishopAttacksSize()
	{
		return 4ULL * 1312ULL;
	}

	constexpr std::size_t rookAttacksSize()
	{
		std::size_t sum = 0;
		for (std::size_t i = 0; i != 64; ++i)
		{
			// on the edge but not in the corner
			if (board::isEdge(i) && !board::isCorner(i))
			{
				sum += (1ULL << 11); // 2^11
			}
			// in the corner
			else if (board::isCorner(i))
			{
				sum += (1ULL << 12); // 2^12
			}
			// in the interior
			else
			{
				sum += (1ULL << 10); // 2^10
			}
		}
		return sum;
	}

	extern attackMap bishopAttacks[bishopAttacksSize()];
	extern board::Bitboard bishopMasks[64];
	extern std::size_t bishopOffsets[64];

	extern attackMap rookAttacks[rookAttacksSize()];
	extern board::Bitboard rookMasks[64];
	extern std::size_t rookOffsets[64];

	extern attackMap kingAttacks[64];
	extern attackMap knightAttacks[64];
	extern attackMap wpawnAttacks[64];
	extern attackMap bpawnAttacks[64];

	constexpr std::uint32_t knightPromoCaps[5] = {constants::knightPromo, constants::knightPromoCapN, 
		constants::knightPromoCapB, constants::knightPromoCapR, constants::knightPromoCapQ};

	constexpr std::uint32_t bishopPromoCaps[5] = {constants::bishopPromo, constants::bishopPromoCapN,
		constants::bishopPromoCapB, constants::bishopPromoCapR, constants::bishopPromoCapQ};

	constexpr std::uint32_t rookPromoCaps[5] = { constants::rookPromo, constants::rookPromoCapN,
		constants::rookPromoCapB, constants::rookPromoCapR, constants::rookPromoCapQ };

	constexpr std::uint32_t queenPromoCaps[5] = { constants::queenPromo, constants::queenPromoCapN,
	constants::queenPromoCapB, constants::queenPromoCapR, constants::queenPromoCapQ };

	template<bool wToMove, board::pieceType p = board::none>
	constexpr attackMap getAttacks(board::Bitboard all, board::square s)
	{
		if constexpr (p == board::pawns)
		{
			if constexpr (wToMove)
				return wpawnAttacks[s];
			else
				return bpawnAttacks[s];
		}
		else if constexpr (p == board::knights)
			return knightAttacks[s];
		else if constexpr (p == board::bishops)
			return bishopAttacks[bishopOffsets[s] + _pext_u64(all, bishopMasks[s])];
		else if constexpr (p == board::rooks)
			return rookAttacks[rookOffsets[s] + _pext_u64(all, rookMasks[s])];
		else if constexpr (p == board::queens)
			return getAttacks<wToMove, board::rooks>(all, s) | getAttacks<wToMove, board::bishops>(all, s);
		else if constexpr (p == board::king)
			return kingAttacks[s];
		else if constexpr (p == board::none)
			return getAttacks<wToMove, board::pawns>(all, s) | getAttacks<wToMove, board::knights>(all, s) |
			getAttacks<wToMove, board::bishops>(all, s) | getAttacks<wToMove, board::rooks>(all, s) |
			getAttacks<wToMove, board::queens>(all, s) | getAttacks<wToMove, board::king>(all, s);
		else
			return 0;
	}

	board::Bitboard dumbFill(board::Bitboard loc, board::Bitboard occ, int r, int f);
	board::Bitboard filterEdgesRook(board::Bitboard rookMoves, std::size_t pos);
	void initRookBishopAttacks();
	void initPawnAttacks();
	void initKingAttacks();
	void initKnightAttacks();
	void initAttacks();

	template<bool by> // true = white, false = black
	constexpr bool isAttacked(const board::Board& b, board::square s)
	{
		const attackMap bAttacks = getAttacks<by, board::bishops>(b.all, s);
		const attackMap nAttacks = getAttacks<by, board::knights>(b.all, s);
		const attackMap rAttacks = getAttacks<by, board::rooks>(b.all, s);
		const attackMap kAttacks = getAttacks<by, board::king>(b.all, s);
		const attackMap pAttacks = getAttacks<!by, board::pawns>(b.all, s);

		if constexpr (by)
		{
			return (bAttacks & (b.wPieces[board::bishops] | b.wPieces[board::queens]))
				|| (rAttacks & (b.wPieces[board::rooks] | b.wPieces[board::queens]))
				|| (nAttacks & b.wPieces[board::knights])
				|| (pAttacks & b.wPieces[board::pawns])
				|| (kAttacks & b.wPieces[board::king]);
		}
		else
		{
			return (bAttacks & (b.bPieces[board::bishops] | b.bPieces[board::queens]))
				|| (rAttacks & (b.bPieces[board::rooks] | b.bPieces[board::queens]))
				|| (nAttacks & b.bPieces[board::knights])
				|| (pAttacks & b.bPieces[board::pawns])
				|| (kAttacks & b.bPieces[board::king]);
		}
	}

	template<bool w>
	constexpr bool isInCheck(const board::Board& b)
	{
		unsigned long kingPos;

		if constexpr (w) _BitScanForward64(&kingPos, b.wPieces[board::king]);
		else _BitScanForward64(&kingPos, b.bPieces[board::king]);

		return isAttacked<!w>(b, (board::square)kingPos);
	}

	template<bool w, bool kingSide>
	constexpr bool castleSquaresFree(const board::Board& b)
	{
		if constexpr (w)
		{
			if constexpr (kingSide) // white kingside
			{
				return !(b.all & board::wkCastleSquares) 
					&& !isAttacked<!w>(b, board::e1) && !isAttacked<!w>(b, board::f1) && !isAttacked<!w>(b, board::g1);
			}
			else // white queenside
			{
				return !(b.all & board::wqCastleSquares)
					&& !isAttacked<!w>(b, board::e1) && !isAttacked<!w>(b, board::c1) && !isAttacked<!w>(b, board::d1);
			}
		}
		else
		{
			if constexpr (kingSide) // black kingside
			{
				return !(b.all & board::bkCastleSquares)
					&& !isAttacked<!w>(b, board::e8) && !isAttacked<!w>(b, board::f8) && !isAttacked<!w>(b, board::g8);
			}
			else // black queenside
			{
				return !(b.all & board::bqCastleSquares)
					&& !isAttacked<!w>(b, board::e8) && !isAttacked<!w>(b, board::c8) && !isAttacked<!w>(b, board::d8);
			}
		}
	}

	template<bool wToMove, board::pieceType p, std::size_t N, bool qSearch = false>
	void genMovesForPiece(board::Board& b, board::Move heading, board::Move(&ml)[N], std::size_t& i)
	{
		static_assert(p != board::pawns, "Called wrong move generation function for pawns");

		board::Bitboard pieceBoard = 0;
		if constexpr (wToMove)
		{
			pieceBoard = b.wPieces[p];
		}
		else
		{
			pieceBoard = b.bPieces[p];
		}

		unsigned long index = 0;
		while (_BitScanForward64(&index,pieceBoard))
		{
			heading &= ~constants::fromMask;
			heading |= index;
			pieceBoard ^= aux::setbit(index);
			attackMap attacks = getAttacks<wToMove, p>(b.all, (board::square)index);
			if constexpr (wToMove)
			{
				attacks &= ~b.wAll;

			}
			else
			{
				attacks &= ~b.bAll;

			}

			while (_BitScanForward64(&index, attacks))
			{
				attacks ^= aux::setbit(index);

				heading &= ~constants::toMask;
				heading |= (index << constants::toMaskOffset);

				heading &= ~constants::moveTypeMask;
				heading |= board::getCapType(b.getPieceType(aux::setbit(index)));

				//b.makeMove<wToMove>(heading);

				//if (!isInCheck<wToMove>(b))
				//{
					ml[i++] = heading;
					//++i;
				//}

				//b.unmakeMove<wToMove>(heading);
			}
		}

		// generate special castling moves which are not covered by the attack maps
		if constexpr (p == board::king)
		{
			if (b.canCastleK<wToMove>() && castleSquaresFree<wToMove, kingSideCastle>(b))
			{
				heading &= ~(constants::moveTypeMask | constants::fromMask | constants::toMask);

				if constexpr (wToMove) heading |= aux::wk_start;
				else heading |= aux::bk_start;

				if constexpr (wToMove) heading |= ((aux::wkr_start - 1) << constants::toMaskOffset);
				else heading |= ((aux::bkr_start - 1) << constants::toMaskOffset);

				heading |= constants::KSCastle;

				ml[i] = heading;
				++i;
			}
			if (b.canCastleQ<wToMove>() && castleSquaresFree<wToMove, queenSideCastle>(b))
			{
				heading &= ~(constants::moveTypeMask | constants::fromMask | constants::toMask);

				if constexpr (wToMove) heading |= aux::wk_start;
				else heading |= aux::bk_start;

				if constexpr (wToMove) heading |= ((aux::wqr_start + 2) << constants::toMaskOffset);
				else heading |= ((aux::bqr_start + 2) << constants::toMaskOffset);

				heading |= constants::QSCastle;

				ml[i] = heading;
				++i;
			}
		}
	}

	template<bool wToMove, std::size_t N, bool qSearch = false>
	void genPawnMoves(board::Board& b, board::Move heading, board::Move(&ml)[N], std::size_t& i)
	{
		auto checkMoveAndAdd = [&b, &ml, &i](board::Move m) constexpr {
			//b.makeMove<wToMove>(m);
			//if (!isInCheck<wToMove>(b))
			//{
				ml[i++] = m;
				//++i;
			//}
			//b.unmakeMove<wToMove>(m);
		};

		unsigned long index;
		board::Bitboard pawns = 0;
		if constexpr (wToMove) pawns = b.wPieces[board::pawns];
		else pawns = b.bPieces[board::pawns];
		while (_BitScanForward64(&index, pawns))
		{
			pawns ^= aux::setbit(index);
			heading &= ~constants::fromMask;
			heading |= index;

			bool seventhRank;
			if constexpr (wToMove) seventhRank = (aux::setbit(index) & board::rankMask[6]);
			else seventhRank = (aux::setbit(index) & board::rankMask[1]);

			board::Bitboard attacks = 0;
			board::Bitboard moves = 0;
			board::Bitboard dblMove = 0;
			if constexpr (wToMove)
			{
				attacks = (wpawnAttacks[index] & (b.bAll | b.epLoc));
				moves = (aux::setbit(index + 8) & ~b.all);
				if (aux::setbit(index) & board::rankMask[1])
					dblMove = moves ? (aux::setbit(index + 16) & ~b.all) : 0;
			}
			else
			{
				attacks = (bpawnAttacks[index] & (b.wAll | b.epLoc));
				moves = (aux::setbit(index - 8) & ~b.all);
				if (aux::setbit(index) & board::rankMask[6])
					dblMove = moves ? (aux::setbit(index - 16) & ~b.all) : 0;
			}


			while (_BitScanForward64(&index, attacks))
			{
				attacks ^= aux::setbit(index);
				heading &= ~constants::toMask;
				heading |= index << constants::toMaskOffset;
				heading &= ~constants::moveTypeMask;
				if (!seventhRank)
				{
					if (aux::setbit(index) != b.epLoc)
					{
						heading |= board::getCapType(b.getPieceType(aux::setbit(index)));
					}
					else
					{
						heading |= constants::enPCap;
					}

					checkMoveAndAdd(heading);
				}
				else
				{
					board::pieceType promoCapType = b.getPieceType(aux::setbit(index));

					heading |= knightPromoCaps[promoCapType];
					checkMoveAndAdd(heading);

					heading &= ~constants::moveTypeMask;
					heading |= bishopPromoCaps[promoCapType];
					checkMoveAndAdd(heading);

					heading &= ~constants::moveTypeMask;
					heading |= rookPromoCaps[promoCapType];
					checkMoveAndAdd(heading);

					heading &= ~constants::moveTypeMask;
					heading |= queenPromoCaps[promoCapType];
					checkMoveAndAdd(heading);
				}
			}

			while (_BitScanForward64(&index, moves))
			{
				moves ^= aux::setbit(index);
				heading &= ~constants::toMask;
				heading |= index << constants::toMaskOffset;
				heading &= ~constants::moveTypeMask;

				if (!seventhRank)
				{
					heading |= constants::QMove;
					checkMoveAndAdd(heading);
				}
				else
				{
					heading |= constants::queenPromo;
					checkMoveAndAdd(heading);

					heading &= ~constants::moveTypeMask;
					heading |= constants::rookPromo;
					checkMoveAndAdd(heading);

					heading &= ~constants::moveTypeMask;
					heading |= constants::bishopPromo;
					checkMoveAndAdd(heading);

					heading &= ~constants::moveTypeMask;
					heading |= constants::knightPromo;
					checkMoveAndAdd(heading);
				}
			}

			if (dblMove)
			{
				heading &= ~constants::toMask;
				heading &= ~constants::moveTypeMask;
				heading |= constants::dblPawnMove;

				_BitScanForward64(&index, dblMove);
				heading |= index << constants::toMaskOffset;
				checkMoveAndAdd(heading);
			}
		}
	}

	template<bool wToMove, std::size_t N, bool qSearch = false>
	std::size_t genMoves(board::Board& b, board::Move(&ml)[N], std::size_t i)
	{
		board::Move heading = b.getHeading();

		genMovesForPiece<wToMove, board::knights>(b, heading, ml, i);
		genMovesForPiece<wToMove, board::bishops>(b, heading, ml, i);
		genMovesForPiece<wToMove, board::rooks>(b, heading, ml, i);
		genMovesForPiece<wToMove, board::queens>(b, heading, ml, i);

		genPawnMoves<wToMove>(b, heading, ml, i);

		genMovesForPiece<wToMove, board::king>(b, heading, ml, i);

		return i;
	}
}
#endif