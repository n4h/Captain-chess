#include <cstddef>

#include "perft.hpp"
#include "board.hpp"


namespace perft
{
	Perft::Perft(){}

	Perft::Perft(board::Board& b, std::size_t t, bool w)
	{
		w ? perft<true>(b, t, (std::size_t)0) : perft<false>(b, t, (std::size_t)0);
	}

	std::size_t Perft::getResult()
	{
		return perftResult;
	}

	void Perft::reset()
	{
		perftResult = 0;
	}
}