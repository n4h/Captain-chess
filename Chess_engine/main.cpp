import <iostream>;
import <string>;
import Board;
int main()
{
	board::Board b{ "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1" };
	b.printState();
	std::string s;
	std::cin >> s;
	return 0;
}