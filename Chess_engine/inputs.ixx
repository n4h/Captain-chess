export module inputs;


import <vector>;
import <variant>;
import <cmath>;
import Board;
import Movegen;

import aux;

namespace inputs
{
	using namespace aux;

	board::Piece getPromo(char c)
	{
		switch (c)
		{
		case 'n':
		case 'N':
			return board::Piece::knight;
		case 'b':
		case 'B':
			return board::Piece::bishop;
		case 'r':
		case 'R':
			return board::Piece::queen;
		default:
			return board::Piece::none;
		}
	}

	void processAndPlay(board::Board& b, std::string move)
	{
		// user can only play a move from this list of valid moves
		std::vector<board::Move> ml = movegen::genMoves(b);
		int moveIndex = -1;
		int enP = b.gameState.top().enP;
		// A move is either a 4 character string (e.g. a2b3) that
		// specifies a from and to square, or it is one of K/k/Q/q
		// to specify king or queenside castling for white or black
		// or it is a 5 character string (e.g. c7c8Q) that describes a pawn promotion
		if (move.size() == 4 && fileNumber(move[0]) != 0 && fileNumber(move[2]) != 0 && isNumber(move[1]) && isNumber(move[3]))
		{
			unsigned int from = index(move[1] - '0', fileNumber(move[0]));
			unsigned int to = index(move[3] - '0', fileNumber(move[2]));
			if (b.mailbox[from].piece == board::Piece::none || b.mailbox[from].color != b.toMove)
				return;
			for (int i = 0; i != 64; ++i)
			{
				if (std::holds_alternative<board::simpleMove>(ml[i]))
				{
					auto sm = std::get<board::simpleMove>(ml[i]);
					if (sm.from == from && sm.to == to)
						moveIndex = i;
				}
				if (std::holds_alternative<board::enPMove>(ml[i]))
				{
					auto epm = std::get<board::enPMove>(ml[i]);
					if (b.mailbox[from].piece == board::Piece::pawn && (to - enP) == 0)
					{
						switch (b.toMove)
						{
						case board::Color::white:
							if (rank(from) == rank(enP) - 1 && std::abs(file(from) - file(enP)) == 1)
								if ((file(from) > file(enP)) == epm.right)
									moveIndex = i;
							break;
						case board::Color::black:
							if (rank(from) == rank(enP) + 1 && std::abs(file(from) - file(enP)) == 1)
								if ((file(from) > file(enP)) == epm.right)
									moveIndex = i;
							break;
						}
					}
				}
			}
		}
		if (move.size() == 1 && (move[0] == 'K' || move[0] == 'k' || move[0] == 'q' || move[0] == 'Q'))
		{
			auto s = move[0];
			if (b.toMove == board::Color::white && (s == 'k' || s == 'q'))
				return;
			if (b.toMove == board::Color::black && (s == 'K' || s == 'Q'))
				return;
			for (int i = 0; i != 64; ++i)
			{
				if (std::holds_alternative<board::castleMove>(ml[i]))
				{
					auto cm = std::get<board::castleMove>(ml[i]);
					if (cm.side == board::CastleSide::king && (s == 'k' || s == 'K'))
						moveIndex = i;
					if (cm.side == board::CastleSide::queen && (s == 'q' || s == 'Q'))
						moveIndex = i;
				}
			}
		}
		if (move.size() == 5 && fileNumber(move[0]) != 0 && fileNumber(move[2]) != 0 && isNumber(move[1]) && isNumber(move[3]))
		{
			unsigned int from = index(move[1] - '0', fileNumber(move[0]));
			unsigned int to = index(move[3] - '0', fileNumber(move[2]));
			for (int i = 0; i != 64; ++i)
			{
				if (std::holds_alternative<board::promoMove>(ml[i]))
				{
					auto pm = std::get<board::promoMove>(ml[i]);
					if (pm.base.from == from && pm.base.to == to && getPromo(move[4]) == pm.promo)
						moveIndex = i;
				}
			}
		}
		b.makeMove(ml[moveIndex]);
	}
}