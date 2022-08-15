module Movegen;

import <variant>;
import <vector>;

import Board;
import aux;

namespace movegen
{
	using namespace aux;

	bool isInCheck(const board::Board& b)
	{
		b;
		return true;
	}

	std::vector<board::Move> genKnightMoves(board::Board b, unsigned int i)
	{
		std::vector<board::Move> ml = {};
		auto f = [&ml, &b, i](int r, int f) {
			auto k = index2index(i, r, f);
			if (isIndex(k) && b.mailbox[k].color != b.toMove)
			{
				auto s = board::simpleMove{ i, k, b.mailbox[k].piece };
				b.makeMove(s);
				if (!isInCheck(b))
				{
					ml.push_back(s);
				}
				b.unmakeMove(s);
			}
		};

		// knight move offsets: knight moves 2 moves
		// in one of the cardinal directions and then
		// 1 move orthogonal to that cardinal direction
		f(2, 1); f(2, -1); f(-2, 1); f(-2, -1);
		f(1, 2); f(-1, 2); f(1, -2); f(-1, -2);

		return ml;
	}

	std::vector<board::Move> genBishopMoves(board::Board b, unsigned int i)
	{
		
	}

}