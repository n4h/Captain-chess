export module Engine;

import Board;
import Search;

namespace engine
{
	export class Engine
	{
	public:
		void playBestMove(board::Board& b);
		double getEval();
		Engine(board::Color s);
		Engine();
		board::Color getSide();
	private:
		board::Move rootSearch(board::Board b, unsigned int depth);
		double eval = 0;
		board::Color side = board::Color::empty;
	};
}