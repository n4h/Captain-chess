import <iostream>;
import <string>;

import Board;
import Inputs;

int main()
{
	board::Board b{ "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1" };
	b.printState();
	std::string s = "n";
	while (s != "quit")
	{
		std::cin >> s;
		inputs::processAndPlay(b, s);
		b.printState();
	}
	return 0;
}