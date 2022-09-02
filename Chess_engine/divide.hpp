#ifndef DIVIDE_H
#define DIVIDE_H

#include <cstddef>
#include <iostream>
#include <string>
#include <cassert>

#include "board.hpp"
#include "movegen.hpp"
#include "constants.hpp"
#include "auxiliary.hpp"
#include "perft.hpp"

namespace divide
{
	std::string prettyPrintMove(board::Move m);

	template<bool wToMove>
	std::size_t perftDivide(board::Board& b, std::size_t t, std::size_t firstMove)
	{
		board::Move moves[2048];
		std::size_t lastMove = movegen::genMoves<wToMove>(b, moves, firstMove);
		std::size_t total = 0;

		for (std::size_t i = firstMove; i != lastMove; ++i)
		{
			b.makeMove<wToMove>(moves[i]);
			perft::Perft p{ b, t - 1, !wToMove };
			total += p.getResult();
			std::cout << prettyPrintMove(moves[i]) << ": " << p.getResult() << std::endl;
			p.reset();
			b.unmakeMove<wToMove>(moves[i]);
		}
		std::cout << "total: " << total << std::endl;
		return total;
	}
}
#endif