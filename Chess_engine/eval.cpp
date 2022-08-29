module Eval;

import Board;
import aux;

namespace eval
{
	using namespace aux;
	double evaluate(board::Board b)
	{
		double eval = 0;
		int pcCount = 0;
		for (int i = 0; i != 64 ; ++i)
		{
			switch (b.mailbox[i].piece)
			{
			case board::Piece::king:
				eval += PSQTking[rank(i) - 1][file(i) - 1];
				break;
			case board::Piece::queen:
				++pcCount;
				eval += static_cast<int>(b.mailbox[i].color) * aux::queen_val * PSQTqueen[rank(i) - 1][file(i) - 1];
				break;
			case board::Piece::rook:
				++pcCount;
				eval += static_cast<int>(b.mailbox[i].color) * aux::rook_val * PSQTrookfile[file(i) - 1] 
					* (b.mailbox[i].color == board::Color::white ? PSQTrookrankW[rank(i) - 1] : PSQTrookrankB[rank(i) - 1]);
				break;
			case board::Piece::bishop:
				++pcCount;
				eval += static_cast<int>(b.mailbox[i].color) * aux::bishop_val * PSQTbishop[rank(i) - 1][file(i) - 1];
				break;
			case board::Piece::knight:
				++pcCount;
				eval += static_cast<int>(b.mailbox[i].color) * aux::knight_val * PSQTknight[rank(i) - 1][file(i) - 1];
				break;
			case board::Piece::pawn:
				++pcCount;
				eval += static_cast<int>(b.mailbox[i].color) * aux::pawn_val
					* (b.mailbox[i].color == board::Color::white ? PSQTpawnrankW[rank(i) - 1] : PSQTpawnrankB[rank(i) - 1]);
				break;
			}
		}
		return static_cast<int>(b.toMove)*eval;
	}
}