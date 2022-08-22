export module Search;

import <vector>;
import <algorithm>;

import Board;
import Movegen;
import Eval;

namespace search
{

	// Fail-soft alpha beta
	export double alphaBetaSearch(board::Board& b, double alpha, double beta, unsigned int depth)
	{
		if (depth == 0)
			return static_cast<int>(b.toMove)*eval::evaluate(b);
		std::vector<board::Move> ml = movegen::genMoves(b);
		double eval = -100000;
		for (auto i : ml)
		{
			b.makeMove(i);
			eval = std::max(eval, -1 * alphaBetaSearch(b, -1*beta, -1*alpha, depth - 1));
			b.unmakeMove(i);
			alpha = std::max(alpha, eval);
			if (alpha > beta)
				break;
		}
		return eval;
	}
}