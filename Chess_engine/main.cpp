#include <iostream>
#include <string>
#include <chrono>


#include "board.hpp"
#include "movegen.hpp"
#include "perft.hpp"
#include "divide.hpp"
#include "engine.hpp"
#include "uci.hpp"

int main()
{
	std::string init;
	std::getline(std::cin, init);

	if (init == "uci")
	{
		uci::UCIProtocol ucip;
		ucip.UCIStartup();
		ucip.UCIStartLoop();
	}
	
	return 0;
}