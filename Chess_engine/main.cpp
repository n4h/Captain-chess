import <iostream>;
import <string>;
import <vector>;
import  <chrono>;

import Board;
import Inputs;
import Engine;
import Perft;
import Precomputed;

int main()
{
	precomputed::xrays = precomputed::initXRays();
	std::string fen = "";
	std::cout << "Enter valid Fen string or enter 's' to load the starting position" << std::endl;
	std::getline(std::cin, fen);
	board::Board b{ "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1" };
	if (fen != "s")
	{
		b = board::Board{ fen };
	}

	/*auto start = std::chrono::steady_clock::now();
	perft::Perft p{ b, 5 };
	auto end = std::chrono::steady_clock::now();
	std::chrono::duration<double> elapsed = end - start;
	std::cout << p.getResult() << " nodes in " << elapsed.count() << " seconds. " << p.getResult() / elapsed.count() << " nodes/s." << std::endl;
	*/
	std::cout << "Enter which color you would like to play as (w for white, b for black)" << std::endl;
	std::string color = "";
	std::cin >> color;
	engine::Engine e;
	if (color == "w")
		e = engine::Engine{ board::Color::black };
	else if (color == "b")
		e = engine::Engine{ board::Color::white };

	std::cout << "Enter 'quit' to leave" << std::endl;
	if (color == "w")
		b.printState();
	std::string s = "";
	while (s != "quit")
	{
		if (b.toMove == e.getSide())
		{
			e.playBestMove(b);
			b.printState();
			std::cout << "Eval: " << e.getEval() << std::endl;
		}
		else
		{
			std::cin >> s;
			inputs::processAndPlay(b, s);
		}
	}
	return 0;
}