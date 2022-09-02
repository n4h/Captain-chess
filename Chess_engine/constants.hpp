#ifndef CHESS_CONSTANTS_H
#define CHESS_CONSTANTS_H

#include <cstdint>

namespace constants
{
	constexpr std::uint32_t fromMask = 077U;
	constexpr std::uint32_t toMask = 07700U;

	constexpr std::uint32_t fromMaskOffset = 0;
	constexpr std::uint32_t toMaskOffset = 6U;

	constexpr std::uint32_t halfMoveCntMask = 0b111111U << 12;

	constexpr std::uint32_t castleFlagMask = 0b1111U << 18;
	constexpr std::uint32_t canCastleWK = 0b0001U << 18;
	constexpr std::uint32_t canCastleWQ = 0b0010U << 18;
	constexpr std::uint32_t canCastleBK = 0b0100U << 18;
	constexpr std::uint32_t canCastleBQ = 0b1000U << 18;

	constexpr std::uint32_t flagsOffset = 12U;
	constexpr std::uint32_t castleFlagsOffset = 18U;

	constexpr std::uint32_t enPMasK = 0b1111U << 22;
	constexpr std::uint32_t enPFileMask = 0b0111U << 22;
	constexpr std::uint32_t enPExistsMask = 0b1000U << 22;

	constexpr std::uint32_t enPMaskOffset = 22;


	/* Move flags: similar to https://www.chessprogramming.org/Encoding_Moves#From-To_Based
	* Advantage is that better move types can be sorted before worse move types
	*/
	constexpr std::uint32_t moveTypeMask    = 0b11111U << 26;
	constexpr std::uint32_t nullMove        = 31U << 26;
	constexpr std::uint32_t queenPromoCapQ  = 30U << 26; // +18
	constexpr std::uint32_t queenPromoCapR  = 29U << 26; // +14
	constexpr std::uint32_t rookPromoCapQ   = 28U << 26; // +14
	constexpr std::uint32_t queenPromoCapB  = 27U << 26; // +12
	constexpr std::uint32_t queenPromoCapN  = 26U << 26; // +12
	constexpr std::uint32_t bishopPromoCapQ = 25U << 26; // +12
	constexpr std::uint32_t knightPromoCapQ = 24U << 26; // +12
	constexpr std::uint32_t rookPromoCapR   = 23U << 26; // +10
	constexpr std::uint32_t queenPromo      = 22U << 26; // +9
	constexpr std::uint32_t rookPromoCapB   = 21U << 26; // +8
	constexpr std::uint32_t rookPromoCapN   = 20U << 26; // +8
	constexpr std::uint32_t bishopPromoCapR = 19U << 26; // +8
	constexpr std::uint32_t knightPromoCapR = 18U << 26; // +8
	constexpr std::uint32_t bishopPromoCapB = 17U << 26; // +6
	constexpr std::uint32_t bishopPromoCapN = 16U << 26; // +6
	constexpr std::uint32_t knightPromoCapB = 15U << 26; // +6
	constexpr std::uint32_t knightPromoCapN = 14U << 26; // +6
	constexpr std::uint32_t rookPromo       = 13U << 26; // +5
	constexpr std::uint32_t bishopPromo     = 12U << 26; // +3
	constexpr std::uint32_t knightPromo     = 11U << 26; // +3
	constexpr std::uint32_t capQ            = 10U << 26;
	constexpr std::uint32_t capR            = 9U << 26;
	constexpr std::uint32_t capB            = 8U << 26;
	constexpr std::uint32_t capN            = 7U << 26;
	constexpr std::uint32_t capP            = 6U << 26;
	constexpr std::uint32_t enPCap          = 5U << 26;
	constexpr std::uint32_t KSCastle        = 4U << 26;
	constexpr std::uint32_t QSCastle        = 3U << 26;
	constexpr std::uint32_t QMove           = 2U << 26;
	constexpr std::uint32_t dblPawnMove     = 1U << 26;

	constexpr std::uint32_t moveTypeOffset = 26U;
}
#endif