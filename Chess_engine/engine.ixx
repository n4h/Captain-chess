export module Engine;

import Board;

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
		int ABNodeCount = 0;
		int QNodeCount = 0;
		board::Move rootSearch(board::Board b, unsigned int depth);
		double getCaptureValue(board::Move& m);
		double quiesceSearch(board::Board& b, double alpha, double beta, int depth);
		double alphaBetaSearch(board::Board& b, double alpha, double beta, unsigned int depth);
		double eval = 0;
		board::Color side = board::Color::empty;
	};
}