module Perft;

namespace perft
{
	Perft::Perft(){}

	Perft::Perft(board::Board& b, unsigned int t, unsigned int mm)
	{
		perft(b, t, mm);
	}

	unsigned int Perft::getResult()
	{
		return perftResult;
	}

	void Perft::perft(board::Board& b, unsigned int t, unsigned int movesMade)
	{
		if (movesMade >= t)
		{
			++perftResult;
			return;
		}

		auto ml = movegen::genMoves(b);
		if (ml.size() == 0)
			++perftResult;

		for (auto i : ml)
		{
			b.makeMove(i);
			perft(b, t, movesMade + 1);
			b.unmakeMove(i);
		}
	}
}