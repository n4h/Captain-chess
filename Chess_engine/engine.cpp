module Engine;

import <algorithm>;
import <utility>;

import Board;
import Movegen;
import Eval;

namespace engine
{

	Engine::Engine() : side(board::Color::empty) {}

	Engine::Engine(board::Color s) : side(s) {}

	double Engine::getEval()
	{
		return eval;
	}

	void Engine::playBestMove(board::Board& b)
	{
		board::Move bestMove = rootSearch(b, 4);
		b.makeMove(bestMove);
	}

	board::Color Engine::getSide()
	{
		return side;
	}

	board::Move Engine::rootSearch(board::Board b, unsigned int depth)
	{
		std::vector<board::Move> ml = movegen::genMoves(b);
		using MoveAndScore = std::pair<board::Move, double>;
		std::vector<MoveAndScore> movesAndScores = {};
		for (int i = 0; i != ml.size(); ++i)
			movesAndScores.push_back(std::make_pair(ml[i], -100000));

		double worstCase = -100000;

		for (unsigned int j = 1; j <= depth; ++j)
		{
			worstCase = -100000;
			double score = -100000;
			
			for (int i = 0; i != movesAndScores.size(); ++i)
			{
				b.makeMove(movesAndScores[i].first);
				++ABNodeCount;
				score = std::max(score, movesAndScores[i].second = -1 * alphaBetaSearch(b, -100000, -1 * worstCase, j - 1));
				b.unmakeMove(movesAndScores[i].first);
				if (score > worstCase)
					worstCase = score;
			}
			
			std::stable_sort(movesAndScores.begin(), movesAndScores.end(), [](const MoveAndScore& a, const MoveAndScore& b) {
				return a.second > b.second;
				});
		}

		eval = static_cast<int>(b.toMove)*worstCase;
		return movesAndScores[0].first;
	}

	double Engine::getCaptureValue(board::Move& m)
	{
		if (std::holds_alternative<board::enPMove>(m))
			return 1;
		if (std::holds_alternative<board::simpleMove>(m))
		{
			auto sm = std::get<board::simpleMove>(m);
			return ((double)(sm.cap)) / 100;
		}
		if (std::holds_alternative<board::promoMove>(m))
		{
			auto pm = std::get<board::promoMove>(m);
			return ((double)(pm.base.cap)) / 100;
		}
		return 0;
	}

	double Engine::quiesceSearch(board::Board& b, double alpha, double beta, int depth)
	{
		/*if (depth > 2)
			return eval::evaluate(b);*/
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
			
		double currEval = alpha;

		for (auto i : capturesOnly)
		{
			if (checkPos + getCaptureValue(i) < alpha)
				continue;
			b.makeMove(i);
			++QNodeCount;
			currEval = std::max(currEval, -1 * quiesceSearch(b, -1 * beta, -1 * alpha, depth + 1));
			b.unmakeMove(i);

			alpha = std::max(alpha, currEval);

			if (alpha > beta)
				return currEval;
		}
		return currEval;
	}

	double Engine::alphaBetaSearch(board::Board& b, double alpha, double beta, unsigned int depth)
	{
		if (depth <= 0)
			return quiesceSearch(b, alpha, beta, 1);
		std::vector<board::Move> ml = movegen::genMoves(b);

		double currEval = -100000;
		for (auto i : ml)
		{
			b.makeMove(i);
			++ABNodeCount;
			currEval = std::max(currEval, -1 * alphaBetaSearch(b, -1 * beta, -1 * alpha, depth - 1));
			b.unmakeMove(i);
			alpha = std::max(alpha, currEval);
			if (alpha > beta)
				break;
		}
		return currEval;
	}
}