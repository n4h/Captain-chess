#include "eval.hpp"
#include "board.hpp"
#include "auxiliary.hpp"
#include "constants.hpp"

namespace eval
{
	using namespace aux;
	using namespace constants;

	unsigned int getCaptureValue(board::Move m)
	{
		switch (board::getMoveInfo<constants::moveTypeMask>(m))
		{
		case capP:
			return pawn_val;
		case knightPromoCapN:
		case bishopPromoCapN:
		case rookPromoCapN:
		case queenPromoCapN:
		case capN:
			return knight_val;
		case knightPromoCapB:
		case bishopPromoCapB:
		case rookPromoCapB:
		case queenPromoCapB:
		case capB:
			return bishop_val;
		case knightPromoCapR:
		case bishopPromoCapR:
		case rookPromoCapR:
		case queenPromoCapR:
		case capR:
			return rook_val;
		case knightPromoCapQ:
		case bishopPromoCapQ:
		case rookPromoCapQ:
		case queenPromoCapQ:
		case capQ:
			return queen_val;
		default:
			return 0U;
		}
	}
}