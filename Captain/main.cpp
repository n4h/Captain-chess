/*
Copyright 2022-2023, Narbeh Mouradian 

Captain is free software: you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by 
the Free Software Foundation, either version 3 of the License, or 
(at your option) any later version.

Captain is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <https://www.gnu.org/licenses/>.

*/

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
	board::ExtraBoardInfo ebi;
	if (init == "uci")
	{
		uci::UCIProtocol ucip;
		ucip.UCIStartup();
		ucip.UCIStartLoop();
	}
	
	return 0;
}