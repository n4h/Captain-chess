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
		std::vector<board::Move> ml = {};

		auto f = [&ml, &b, i](int r, int l) {
			for (int k = 1; isIndex(index2index(i, k * r, k * l)); ++k)
			{
				if (b.mailbox[index2index(i, k * r, k * l)].color == b.toMove)
				{
					break;
				}
				else
				{
					auto s = board::simpleMove{ i, index2index(i, k * r, k * l), b.mailbox[index2index(i, k * r, k * l)].piece };
					b.makeMove(s);
					if (!isInCheck(b))
						ml.push_back(s);
					b.unmakeMove(s);
				}
				if (b.mailbox[index2index(i, k * r, k * l)].color == static_cast<board::Color>(-1 * static_cast<int>(b.toMove)))
					break;
			}
		};
		// arguments represent directions that the bishop can move in
		f(1, 1); f(1, -1); f(-1, 1); f(-1, -1);
	}



}