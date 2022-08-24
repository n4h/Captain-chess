module Engine;

import <algorithm>;

import Board;
import Movegen;
import Search;
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

	board::Move Engine::rootSearch(board::Board b, unsigned int depth)
	{
		std::vector<board::Move> ml = movegen::genMoves(b);
		double worstCase = -100000;
		unsigned int whichMove = 0;
		double score = -100000;
		for (int i = 0; i != ml.size(); ++i)
		{
			b.makeMove(ml[i]);
			score = std::max(score, -1*search::alphaBetaSearch(b, -100000, -1*worstCase, depth - 1));
			b.unmakeMove(ml[i]);
			if (score > worstCase)
			{
				worstCase = score;
				whichMove = i;
			}
		
		}
		eval = static_cast<int>(b.toMove)*worstCase;
		return ml[whichMove];
	}

	board::Color Engine::getSide()
	{
		return side;
	}
}