module Eval;

import Board;
import aux;

namespace eval
{
	double evaluate(board::Board b)
	{
		double eval = 0;
		for (const auto i : b.mailbox)
		{
			switch (i.piece)
			{
			case board::Piece::king:
				break;
			case board::Piece::queen:
				eval += static_cast<int>(i.color) * aux::queen_val;
				break;
			case board::Piece::rook:
				eval += static_cast<int>(i.color) * aux::rook_val;
				break;
			case board::Piece::bishop:
				eval += static_cast<int>(i.color) * aux::bishop_val;
				break;
			case board::Piece::knight:
				eval += static_cast<int>(i.color) * aux::knight_val;
				break;
			case board::Piece::pawn:
				eval += static_cast<int>(i.color) * aux::pawn_val;
				break;
			}
		}
		return eval;
	}
}