export module Perft;

import Board;
import Movegen;

namespace perft
{
	export class Perft
	{
	public:
		void perft(board::Board& b, unsigned int t, unsigned int movesMade = 0);
		unsigned int getResult();
		Perft();
		Perft(board::Board& b, unsigned int t, unsigned int mm = 0);
	private:
		unsigned int perftResult = 0;
	};
}