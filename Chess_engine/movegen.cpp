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
		int i = -1;
		auto s = board::Square{ true,b.toMove,board::Piece::king };
		for (int j = 0; j != 64; ++j)
		{
			if (b.mailbox[j] == s)
			{
				i = j;
				break;
			}
		}
		return i == -1 ? false : isAttacked(b, i);
	}

	bool isAttacked(board::Board b, unsigned int i)
	{
		// if square S1 is attacked by a bishop onS2, 
		// this is equivalent to a bishop on S1 attacking
		// S2. This symmetry (+ special cases for pawns)
		// is used to tell if square i is attacked
		std::vector<board::Move> ml = {};

		// delete the current color's king in order to avoid 
		// pinned piece complications
		for (int j = 0; j != 64; ++j)
		{
			if (b.mailbox[j] == board::Square{ true, b.toMove , board::Piece::king })
				b.mailbox[j] = board::Square{ false, static_cast<board::Color>(-1 * static_cast<int>(b.toMove)), board::Piece::none };
		}

		ml = genBishopMoves(b, i);
		for (const auto& j : ml)
		{
			if (auto s = std::get<board::simpleMove>(j); s.cap == board::Piece::bishop || s.cap == board::Piece::queen)
				return true;
		}

		ml = genRookMoves(b, i);
		for (const auto& j : ml)
		{
			if (auto s = std::get<board::simpleMove>(j); s.cap == board::Piece::rook || s.cap == board::Piece::queen)
				return true;
		}

		ml = genKnightMoves(b, i);
		for (const auto& j : ml)
		{
			if (auto s = std::get<board::simpleMove>(j); s.cap == board::Piece::knight)
				return true;
		}

		ml = genKingMoves(b, i);
		for (const auto& j : ml)
		{
			if (std::holds_alternative<board::simpleMove>(j))
				if (auto s = std::get<board::simpleMove>(j); s.cap == board::Piece::king)
					return true;
		}

		return false;
	}

	std::vector<board::Move> genKnightMoves(board::Board b, unsigned int i)
	{
		std::vector<board::Move> ml = {};

		if (b.mailbox[i].piece != board::Piece::rook)
			return ml;

		auto f = [&ml, &b, i](int r, int f) {
			auto k = index2index(i, r, f);
			if (!(rank(i) + r > 8 || rank(i) + r < 1 || file(i) + f > 8 || file(i) + f < 1) && b.mailbox[k].color != b.toMove)
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
		for (int k = 1; !(rank(i) + k * r > 8 || rank(i) + k * r < 1 || file(i) + k * f > 8 || file(i) + k * f < 1); ++k)
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

		if (b.mailbox[i].piece != board::Piece::rook)
			return ml;

		genMoveInDirection(ml, b, i, 1, 1);
		genMoveInDirection(ml, b, i, 1, -1);
		genMoveInDirection(ml, b, i, -1, 1);
		genMoveInDirection(ml, b, i, -1, -1);

		return ml;
	}

	std::vector<board::Move> genRookMoves(board::Board b, unsigned int i)
	{
		std::vector<board::Move> ml = {};

		if (b.mailbox[i].piece != board::Piece::rook)
			return ml;

		genMoveInDirection(ml, b, i, 1, 0);
		genMoveInDirection(ml, b, i, -1, 0);
		genMoveInDirection(ml, b, i, 0, 1);
		genMoveInDirection(ml, b, i, 0, -1);

		return ml;
	}

	std::vector<board::Move> genQueenMoves(board::Board b, unsigned int i)
	{
		std::vector<board::Move> ml = {};

		if (b.mailbox[i].piece != board::Piece::rook)
			return ml;

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
		if (b.mailbox[i].piece != board::Piece::king)
			return ml;

		// king can move one square around itself (r and f are rank and file offsets)
		for (int r = -1; r != 2; ++r)
		{
			for (int f = -1; f != 2; ++f)
			{
				if (rank(i) + r > 8 || rank(i) + r < 1 || file(i) + f > 8 || file(i) + f < 1)
					continue;
				if (r == 0 && f == 0) //skip because this is the current square
					continue;
				if (b.mailbox[index2index(i, r, f)].color != b.toMove)
				{
					auto s = board::simpleMove{ i, index2index(i, r,f), b.mailbox[index2index(i, r, f)].piece };
					b.makeMove(s);
					if (!isInCheck(b))
						ml.push_back(s);
					b.unmakeMove(s);
				}
			}
		}

		if (b.toMove == board::Color::white)
		{
			if (b.gameState.top().wk)
				if (!isAttacked(b, wk_start) && !isAttacked(b, wk_start + 1) && !isAttacked(b, wk_start + 2)
					&& !b.mailbox[wk_start + 1].occupied && !b.mailbox[wk_start + 2].occupied)
					ml.push_back(board::castleMove{ board::CastleSide::king });
			if (b.gameState.top().wq)
				if (!isAttacked(b, wk_start) && !isAttacked(b, wk_start - 1) && !isAttacked(b, wk_start - 2)
					&& !b.mailbox[wk_start - 1].occupied && !b.mailbox[wk_start - 2].occupied && !b.mailbox[wk_start - 3].occupied)
					ml.push_back(board::castleMove{board::CastleSide::queen});
		}
		else if (b.toMove == board::Color::black)
		{
			if (b.gameState.top().bk)
				if (!isAttacked(b, bk_start) && !isAttacked(b, bk_start + 1) && !isAttacked(b, bk_start + 2)
					&& !b.mailbox[bk_start + 1].occupied && !b.mailbox[bk_start + 2].occupied)
					ml.push_back(board::castleMove{ board::CastleSide::king });
			if (b.gameState.top().bq)
				if (!isAttacked(b, bk_start) && !isAttacked(b, bk_start - 1) && !isAttacked(b, bk_start - 2)
					&& !b.mailbox[bk_start - 1].occupied && !b.mailbox[bk_start - 2].occupied && !b.mailbox[bk_start - 3].occupied)
					ml.push_back(board::castleMove{ board::CastleSide::queen });
		}

		return ml;
	}

	std::vector<board::Move> genPawnMoves(board::Board b, unsigned int i)
	{
		std::vector<board::Move> ml = {};
		
		// 1 (up the board) if pawn is white, -1 (down the board) if pawn is
		// black
		const int direction = static_cast<int>(b.mailbox[i].color);

		const bool seventhRank = (direction == 1 && rank(i) == 7) || (direction == -1 && rank(i) == 2);

		auto f = [&ml, &b](board::Move m) {
			b.makeMove(m);
			if (!isInCheck(b))
				ml.push_back(m);
			b.unmakeMove(m);
		};

		for (int j = -1; j != 2; ++j)
		{
			if (j + file(i) > 8 || j + file(i) < 1 || rank(i) + direction > 8 || rank(i) + direction < 1)
				continue;
			auto dest = index2index(i, direction, j);
			if (j == 0 && !b.mailbox[dest].occupied)
			{
				if (seventhRank)
				{
					auto p = board::promoMove{ board::simpleMove{i, dest}, board::Piece::knight}; f(p);
					p.promo = board::Piece::bishop; f(p);
					p.promo = board::Piece::rook; f(p);
					p.promo = board::Piece::queen; f(p);
				}
				else
				{
					auto s = board::simpleMove{ i, dest };
					f(s);
				}
			}
			else if ((j == -1 || j == 1) && b.mailbox[dest].color == static_cast<board::Color>(-1*static_cast<int>(b.mailbox[i].color)))
			{
				if (seventhRank)
				{
					auto p = board::promoMove{ board::simpleMove{i, dest, b.mailbox[dest].piece}, board::Piece::knight}; f(p);
					p.promo = board::Piece::bishop; f(p);
					p.promo = board::Piece::rook; f(p);
					p.promo = board::Piece::queen; f(p);
				}
				else
				{
					auto s = board::simpleMove{ i, dest, b.mailbox[dest].piece};
					f(s);
				}
			}
			else if ((j == -1 || j == 1) && b.mailbox[dest].color == board::Color::empty && unsigned(b.gameState.top().enP) == dest)
			{
				auto s = board::enPMove{ int(dest), file(i) > file(dest) };
				f(s);
			}
		}

		// a pawn on the starting rank can move two squares forward
		if ((rank(i) == 2 && direction == 1) || (rank(i) == 7 && direction == -1))
		{
			auto s2 = board::simpleMove{ i, index2index(i,2 * direction), b.mailbox[index2index(i,2*direction)].piece}; // move 2 squares forward
			if (!b.mailbox[index2index(i, 2 * direction)].occupied && !b.mailbox[index2index(i,direction)].occupied)
				f(s2);
		}
		return ml;
	}

	std::vector<board::Move> genMoves(const board::Board& b)
	{
		std::vector<board::Move> ml = {};
		for (int i = 0; i != 64; ++i)
		{
			std::vector<board::Move> moves = {};
			if (b.mailbox[i].color != b.toMove)
				continue;
			switch (b.mailbox[i].piece)
			{
			case board::Piece::pawn:
				moves = genPawnMoves(b, i);
				ml.insert(ml.end(), moves.begin(), moves.end());
				break;
			case board::Piece::bishop:
				moves = genBishopMoves(b, i);
				ml.insert(ml.end(), moves.begin(), moves.end());
				break;
			case board::Piece::knight:
				moves = genKnightMoves(b, i);
				ml.insert(ml.end(), moves.begin(), moves.end());
				break;
			case board::Piece::rook:
				moves = genRookMoves(b, i);
				ml.insert(ml.end(), moves.begin(), moves.end());
				break;
			case board::Piece::queen:
				moves = genQueenMoves(b, i);
				ml.insert(ml.end(), moves.begin(), moves.end());
				break;
			case board::Piece::king:
				moves = genKingMoves(b, i);
				ml.insert(ml.end(), moves.begin(), moves.end());
				break;
			default:
				break;
			}
		}
		return ml;
	}

}