export module Search;

import <vector>;

import Board;
import Eval;

namespace search
{
	// Fail-soft alpha beta
	double alphaBetaSearch(board::Board& b, double alpha, double beta, unsigned int depth)
	{
		if (depth == 0)
			return eval::evaluate(b);
		return alpha*beta;
	}
}