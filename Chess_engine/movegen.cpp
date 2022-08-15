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

		//not done
		(void)b;
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


	// generate moves by moving (r, f) steps at a time from the position i
	void genMoveInDirection(std::vector<board::Move>& ml, board::Board b, unsigned int i, int r, int f)
	{
		for (int k = 1; isIndex(index2index(i, k * r, k * f)); ++k)
		{
			if (b.mailbox[index2index(i, k * r, k * f)].color == b.toMove)
			{
				break;
			}
			else
			{
				auto s = board::simpleMove{ i, index2index(i, k * r, k * f), b.mailbox[index2index(i, k * r, k * f)].piece };
				b.makeMove(s);
				if (!isInCheck(b))
					ml.push_back(s);
				b.unmakeMove(s);
			}
			if (b.mailbox[index2index(i, k * r, k * f)].color == static_cast<board::Color>(-1 * static_cast<int>(b.toMove)))
				break;
		}
	}

	std::vector<board::Move> genBishopMoves(board::Board b, unsigned int i)
	{
		std::vector<board::Move> ml = {};

		genMoveInDirection(ml, b, i, 1, 1);
		genMoveInDirection(ml, b, i, 1, -1);
		genMoveInDirection(ml, b, i, -1, 1);
		genMoveInDirection(ml, b, i, -1, -1);

		return ml;
	}

	std::vector<board::Move> genRookMoves(board::Board b, unsigned int i)
	{
		std::vector<board:Move> ml = {};

		genMoveInDirection(ml, b, i, 1, 0);
		genMoveInDirection(ml, b, i, -1, 0);
		genMoveInDirection(ml, b, i, 0, 1);
		genMoveInDirection(ml, b, i, 0, -1);

		return ml;
	}

	std::vector<board::Move> genQueenMoves(board::Board b, unsigned int i)
	{
		std::vector<board::Move> ml = {};

		genMoveInDirection(ml, b, i, 1, 0);
		genMoveInDirection(ml, b, i, -1, 0);
		genMoveInDirection(ml, b, i, 0, 1);
		genMoveInDirection(ml, b, i, 0, -1);
		genMoveInDirection(ml, b, i, 1, 1);
		genMoveInDirection(ml, b, i, 1, -1);
		genMoveInDirection(ml, b, i, -1, 1);
		genMoveInDirection(ml, b, i, -1, -1);

		return ml;
	}

	std::vector<board::Move> genKingMoves(board::Board b, unsigned int i)
	{
		std::vector<board::Move> ml = {};


		return ml;
	}
}