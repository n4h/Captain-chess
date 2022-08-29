export module Search;

import <vector>;
import <algorithm>;
import <variant>;

import Board;
import Movegen;
import Eval;

namespace search
{
	double getCaptureValue(board::Move& m)
	{
		if (std::holds_alternative<board::enPMove>(m))
			return 1;
		if (std::holds_alternative<board::simpleMove>(m))
		{
			auto sm = std::get<board::simpleMove>(m);
			return ((double)(sm.cap))/100;
		}
		if (std::holds_alternative<board::promoMove>(m))
		{
			auto pm = std::get<board::promoMove>(m);
			return ((double)(pm.base.cap))/100;
		}
		return 0;
	}

	export double quiesceSearch(board::Board& b, double alpha, double beta, int depth)
	{
		if (depth > 10)
			return eval::evaluate(b);
		double checkPos = eval::evaluate(b);
		if (checkPos > beta)
			return checkPos;
		alpha = std::max(checkPos, alpha);

		std::vector<board::Move> ml = movegen::genMoves(b);
		std::vector<board::Move> capturesOnly = {};
		for (auto i : ml)
		{
			if (std::holds_alternative<board::simpleMove>(i) && std::get<board::simpleMove>(i).cap != board::Piece::none)
				capturesOnly.push_back(i);
			if (std::holds_alternative<board::enPMove>(i))
				capturesOnly.push_back(i);
		}

		std::sort(capturesOnly.begin(), capturesOnly.end(), [&b](const board::Move& a, const board::Move& c) {
			double diffA = 0; double diffC = 0;
			if (std::holds_alternative<board::simpleMove>(a))
			{
				auto sm = std::get<board::simpleMove>(a);
				diffA = ((double)(sm.cap)) / 100 - ((double)(b.mailbox[sm.from].piece)) / 100;
			}
			if (std::holds_alternative<board::simpleMove>(c))
			{
				auto sm = std::get<board::simpleMove>(c);
				diffC = ((double)(sm.cap)) / 100 - ((double)(b.mailbox[sm.from].piece)) / 100;
			}
			return diffA > diffC;
			});

		double eval = alpha;

		for (auto i : capturesOnly)
		{
			if (checkPos + getCaptureValue(i) + 2 < alpha)
				continue;
			b.makeMove(i);
			eval = std::max(eval, -1 * quiesceSearch(b, -1 * beta, -1 * alpha, depth + 1));
			b.unmakeMove(i);

			alpha = std::max(alpha, eval);

			if (alpha > beta)
				return eval;
		}
		return eval;
	}

	// Fail-soft alpha beta
	export double alphaBetaSearch(board::Board& b, double alpha, double beta, unsigned int depth)
	{
		if (depth == 0)
			return quiesceSearch(b, alpha, beta);
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