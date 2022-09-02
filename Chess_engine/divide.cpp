#include <cstddef>
#include <iostream>
#include <sstream>
#include <string>
#include <cassert>

#include "divide.hpp"
#include "board.hpp"
#include "constants.hpp"
#include "auxiliary.hpp"
#include "perft.hpp"


namespace divide
{
	std::string prettyPrintMove(board::Move m)
	{
		std::size_t from = board::getMoveInfo<constants::fromMask>(m);
		std::size_t to = board::getMoveInfo<constants::toMask>(m);
		std::ostringstream p;

		auto fromFile = aux::file(from);
		auto toFile = aux::file(to);
		auto fromRank = aux::rank(from);
		auto toRank = aux::rank(to);

		p << aux::file2char(fromFile);
		p << fromRank + 1;
		p << aux::file2char(toFile);
		p << toRank + 1;

		switch (board::getPromoPiece(m))
		{
		case board::queens:
			p << 'Q';
			return p.str();
		case board::rooks:
			p << 'R';
			return p.str();
		case board::bishops:
			p << 'B';
			return p.str();
		case board::knights:
			p << 'N';
			return p.str();
		default:
			return p.str();
		}
	}
}