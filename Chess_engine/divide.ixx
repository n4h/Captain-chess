export module Divide;

import <vector>;

import Board;
import Movegen;
import Perft;

namespace divide
{

	export void perftDivide(board::Board& b, unsigned int perftDepth)
	{
		auto ml = movegen::genMoves(b);
		std::vector<perft::Perft> vp = {};
		for (int i = 0; i != ml.size(); ++i)
		{
			b.makeMove(ml[i]);
			perft::Perft p{ b, perftDepth - 1 };
			vp.push_back(p);
			b.unmakeMove(ml[i]);
		}
	}
}