module Board;

import <string>;
import <iostream>;
import <algorithm>;
import <iterator>;
import <cctype>;
import <algorithm>;
import <variant>;

import aux;

namespace board
{
	using namespace aux;

	bool operator==(const Square& s1, const Square& s2)
	{
		return (s1.occupied == s2.occupied && s1.color == s2.color && s1.piece == s2.piece);
	}

	bool operator!=(const Square& s1, const Square& s2)
	{
		return (s1.occupied != s2.occupied || s1.color != s2.color || s1.piece != s2.piece);
	}

	Color oppositeColor(Color c)
	{
		return static_cast<Color>(-1 * static_cast<int>(c));
	}

	// FEN strings use the symbols r,R,q,Q, etc. to specify chess pieces
	// with capital letters indicating the white pieces
	// this function returns a Square that contains the appropriate piece
	// data
	Square makeSquare(const char& i)
	{
		switch (i)
		{
		case 'r':
			return Square{ true, Color::black,Piece::rook };
		case 'R':
			return Square{ true, Color::white,Piece::rook };
		case 'k':
			return Square{ true, Color::black,Piece::king };
		case 'K':
			return Square{ true, Color::white,Piece::king };
		case 'q':
			return Square{ true, Color::black,Piece::queen };
		case 'Q':
			return Square{ true, Color::white,Piece::queen };
		case 'b':
			return Square{ true, Color::black,Piece::bishop };
		case 'B':
			return Square{ true, Color::white,Piece::bishop };
		case 'n':
			return Square{ true, Color::black,Piece::knight };
		case 'N':
			return Square{ true, Color::white,Piece::knight };
		case 'p':
			return Square{ true, Color::black,Piece::pawn };
		case 'P':
			return Square{ true, Color::white,Piece::pawn };
		default:
			return Square{ false, Color::empty, Piece::none };
		}
	}

	std::string Square::getSymbol() noexcept
	{
		std::string c = ".";
		switch (piece)
		{
		case Piece::bishop:
			c = "b";
			break;
		case Piece::knight:
			c = "n";
			break;
		case Piece::king:
			c = "k";
			break;
		case Piece::pawn:
			c = "p";
			break;
		case Piece::queen:
			c = "q";
			break;
		case Piece::rook:
			c = "r";
			break;
		default:
			c = ".";
			break;
		}
		if (color == Color::white)
			std::transform(c.cbegin(), c.cend(), c.begin(), [](char c) {return static_cast<char>(std::toupper(c)); });
		return c;
	}

	//unfortunately long function that processes a FEN string
	// and converts it to a board representation
	Board::Board(const std::string& fen)
	{
		irrState is;
		int currRank = 8;
		int currFile = 1;
		bool processingGrid = true;
		bool next = false;
		std::string pos = "";
		std::string last = "";
		for (auto& i : fen)
		{
			if (currRank == 0)
			{
				processingGrid = false;
			}
			if (processingGrid)
			{
				// "/" and " " signals that we are done processing a rank
				if (i == '/' || i == ' ')
				{
					--currRank;
					continue;
				}
				// encountering a number at this stage represents consecutive empty squares
				if (isNumber(i))
				{
					const int number = i - '0';
					for (int j = currFile; j != currFile + number; ++j)
					{
						mailbox[index(currRank, j)] = Square{ false,Color::empty,Piece::none };
					}
					currFile = incFile(currFile, number);
				}
				else // if it is not a number, use makeSquare to generate the right contents
				{
					mailbox[index(currRank, currFile)] = makeSquare(i);
					currFile = incFile(currFile, 1);
				}
			}
			else // note: this part only executes when we are done processing the board
			{
				switch (i)
				{
				case ' ':
					last += i;
					break;
				case 'w':
					toMove = Color::white;
					break;
					// the appearance of b can refer to either the side to move or
					// to the b file
					// The side to move always occurs first, so first check
					// if toMove is empty, and if so, fill it in with black
					// if not empty, that is because we have already filled
					// it in, and we can be certain that "b" refers to a file
				case 'b':
					if (toMove == Color::empty)
						toMove = Color::black;
					break;
				case 'k':
					is.bk = true;
					break;
				case 'K':
					is.wk = true;
					break;
				case 'q':
					is.bq = true;
					break;
				case 'Q':
					is.wq = true;
					break;
				default:
					break;
				}
				if (next)
				{
					pos += i;
					next = false;
					continue; // to avoid the isMoveNumber if statement below
				}
				// the en passant square is given in algebraic notation
				// by file letters and rank numbers e.g. a3, b6, c3, etc.
				// so first check if i is a file letter (a-h).
				// if so, set next = true to signal that the next
				// iteration will be a number. Note: if black is to
				// move, then the first two characters of pos will
				// be "b ". This will be checked for and ignored later
				if (isFile(i))
				{
					pos += i;
					next = true;
				}
				if (isMoveNumber(i))
				{
					last += i; // the space between the last two numbers is added in the switch statement
				}
			}
		}
		// check that pos has at least two elements and then access the last two elements
		if (pos.length() >= 2)
		{
			if (isMoveNumber(pos[pos.length() - 1]) && isFile(pos[pos.length() - 2]))
				is.enP = index(pos[pos.length() - 1] - '0', fileNumber(pos[pos.length() - 2]));
		}
		bool encounteredNumber = false;
		bool encounteredSpace = false;
		std::string ply = "";
		std::string mov = "";
		// last contains leading spaces and two numbers separated by a space
		for (auto& i : last)
		{
			if (i == ' ' && !encounteredNumber)
				continue;
			if (isMoveNumber(i) && !encounteredSpace)
			{
				encounteredNumber = true;
				ply += i;
				continue;
			}
			if (i == ' ')
				encounteredSpace = true;
			if (isMoveNumber(i))
			{
				mov += i;
			}
		}
		is.ply50 = std::stoi(ply);
		currMove = std::stoi(mov);
		gameState.push(is);
	}

	void Board::changeColor() noexcept
	{
		toMove = oppositeColor(toMove);
	}

	void Board::makeMove(const Move& m) noexcept
	{
		if (std::holds_alternative<simpleMove>(m))
			makeSimpleMove(m);
		if (std::holds_alternative<castleMove>(m))
			makeCastleMove(m);
		if (std::holds_alternative<enPMove>(m))
			makeEnPMove(m);
		if (std::holds_alternative<promoMove>(m))
			makePromoMove(m);
		if (toMove == Color::black)
			++currMove;
		changeColor();
	}

	void Board::unmakeMove(const Move& m) noexcept
	{
		if (std::holds_alternative<simpleMove>(m))
			unmakeSimpleMove(m);
		if (std::holds_alternative<castleMove>(m))
			unmakeCastleMove(m);
		if (std::holds_alternative<enPMove>(m))
			unmakeEnPMove(m);
		if (std::holds_alternative<promoMove>(m))
			unmakePromoMove(m);
		if (toMove == Color::white)
			--currMove;
		changeColor();
	}

	void Board::makeSimpleMove(const Move& m) noexcept
	{
		auto sm = std::get<simpleMove>(m);
		auto is = gameState.top();
		// step #1 is to check if this move opens up en passant.
		// the nice thing about en passant is that the active en
		// passant square (the square which is passed over) can 
		// only be on the 3rd rank (white) or 6th rank (black)
		if (mailbox[sm.from].piece == Piece::pawn)
		{
			if (toMove == Color::white && isWhiteDoublePawnMove(sm.from, sm.to))
				is.enP = index(3, file(sm.from));
			if (toMove == Color::black && isBlackDoublePawnMove(sm.from, sm.to))
				is.enP = index(6, file(sm.from));
			if (!isWhiteDoublePawnMove(sm.from, sm.to) && !isBlackDoublePawnMove(sm.from, sm.to))
				is.enP = -1;
		}
		else
		{
			is.enP = -1;
		}

		// update moves since last capture or pawn move
		if (sm.cap == Piece::none && mailbox[sm.from].piece != Piece::pawn)
		{
			is.ply50 += 1;
		}
		else
		{
			is.ply50 = 0;
		}
		// finally, make the move
		mailbox[sm.to] = mailbox[sm.from];
		mailbox[sm.from] = Square{false, Color::empty, Piece::none};

		// update castling rights
		if (mailbox[wk_start] != Square{ true, Color::white, Piece::king }) { is.wk = false; is.wq = false; }
		if (mailbox[wkr_start] != Square{ true, Color::white, Piece::rook }) { is.wk = false; }
		if (mailbox[wqr_start] != Square{ true, Color::white, Piece::rook }) { is.wq = false; }
		if (mailbox[bk_start] != Square{ true, Color::black, Piece::king }) { is.bk = false; is.bq = false; }
		if (mailbox[bkr_start] != Square{ true, Color::black, Piece::rook }) { is.bk = false; }
		if (mailbox[bqr_start] != Square{ true, Color::black, Piece::rook }) { is.bq = false; }

		gameState.push(is);
	}

	void Board::makeCastleMove(const Move& m) noexcept
	{
		auto cm = std::get<castleMove>(m);
		auto is = gameState.top();
		is.ply50 += 1;
		is.enP = -1;
		switch (toMove)
		{
		case Color::white:
			if (cm.side == CastleSide::king)
			{
				mailbox[wk_start + 2] = mailbox[wk_start];
				mailbox[wk_start + 1] = mailbox[wkr_start];
				mailbox[wk_start] = Square{ false, Color::empty, Piece::none };
				mailbox[wkr_start] = Square{ false, Color::empty, Piece::none };
				is.wk = false; is.wq = false;
			}
			else if (cm.side == CastleSide::queen)
			{
				mailbox[wk_start - 2] = mailbox[wk_start];
				mailbox[wk_start - 1] = mailbox[wqr_start];
				mailbox[wk_start] = Square{ false, Color::empty, Piece::none };
				mailbox[wqr_start] = Square{ false, Color::empty, Piece::none };
				is.wk = false; is.wq = false;
			}
			break;
		case Color::black:
			if (cm.side == CastleSide::king)
			{
				mailbox[bk_start + 2] = mailbox[bk_start];
				mailbox[bk_start + 1] = mailbox[bkr_start];
				mailbox[bk_start] = Square{ false, Color::empty, Piece::none };
				mailbox[bkr_start] = Square{ false, Color::empty, Piece::none };
				is.bk = false; is.bq = false;
			}
			else if (cm.side == CastleSide::queen)
			{
				mailbox[bk_start - 2] = mailbox[bk_start];
				mailbox[bk_start - 1] = mailbox[bqr_start];
				mailbox[bk_start] = Square{ false, Color::empty, Piece::none };
				mailbox[bqr_start] = Square{ false, Color::empty, Piece::none };
				is.bk = false; is.bq = false;
			}
			break;
		}
		gameState.push(is);
	}

	void Board::makeEnPMove(const Move& m) noexcept
	{
		auto epm = std::get<enPMove>(m);
		auto is = gameState.top();

		is.ply50 = 0;
		is.enP = -1;

		switch (toMove)
		{
		case Color::white:
			if (epm.right)
			{
				mailbox[epm.square] = mailbox[index2index(epm.square, -1, 1)];
				mailbox[index2index(epm.square, -1)] = Square{false, Color::empty, Piece::none};
				mailbox[index2index(epm.square, -1, 1)] = Square{ false, Color::empty, Piece::none };
			}
			else
			{
				mailbox[epm.square] = mailbox[index2index(epm.square, -1, -1)];
				mailbox[index2index(epm.square, -1)] = Square{ false, Color::empty, Piece::none };
				mailbox[index2index(epm.square, -1, -1)] = Square{ false, Color::empty, Piece::none };
			}
			break;
		case Color::black:
			if (epm.right)
			{
				mailbox[epm.square] = mailbox[index2index(epm.square, 1, 1)];
				mailbox[index2index(epm.square, 1)] = Square{ false, Color::empty, Piece::none };
				mailbox[index2index(epm.square, 1, 1)] = Square{ false, Color::empty, Piece::none };
			}
			else
			{
				mailbox[epm.square] = mailbox[index2index(epm.square, 1, -1)];
				mailbox[index2index(epm.square, 1)] = Square{ false, Color::empty, Piece::none };
				mailbox[index2index(epm.square, 1, -1)] = Square{ false, Color::empty, Piece::none };
			}
			break;
		}
		gameState.push(is);
	}
	
	void Board::makePromoMove(const Move& m) noexcept
	{
		auto pm = std::get<promoMove>(m);
		makeSimpleMove(pm.base);
		mailbox[pm.base.to].piece = pm.promo;
	}

	void Board::unmakeSimpleMove(const Move& m) noexcept
	{
		auto sm = std::get<simpleMove>(m);

		mailbox[sm.from] = mailbox[sm.to];
		if (sm.cap == Piece::none)
		{
			mailbox[sm.to] = Square{ false, Color::empty, Piece::none };
		}
		else
		{
			// if a piece was captured, then whoever currently has the turn
			// must have had their piece captured. Hence, toMove is the color
			// of the captured piece
			mailbox[sm.to] = Square{ true, toMove, sm.cap };
		}
		gameState.pop();
	}

	void Board::unmakeCastleMove(const Move& m) noexcept
	{
		auto cm = std::get<castleMove>(m);

		switch (toMove)
		{
		case Color::black:
			if (cm.side == CastleSide::king)
			{
				mailbox[wk_start] = mailbox[wk_start + 2];
				mailbox[wkr_start] = mailbox[wk_start + 1];
				mailbox[wk_start + 1] = Square{ false, Color::empty, Piece::none };
				mailbox[wk_start + 2] = Square{ false, Color::empty, Piece::none };
			}
			else if (cm.side == CastleSide::queen)
			{
				mailbox[wk_start] = mailbox[wk_start - 2];
				mailbox[wqr_start] = mailbox[wk_start - 1];
				mailbox[wk_start - 1] = Square{ false, Color::empty, Piece::none };
				mailbox[wk_start - 2] = Square{ false, Color::empty, Piece::none };
			}
			break;
		case Color::white:
			if (cm.side == CastleSide::king)
			{
				mailbox[bk_start] = mailbox[bk_start + 2];
				mailbox[bkr_start] = mailbox[bk_start + 1];
				mailbox[bk_start + 1] = Square{ false, Color::empty, Piece::none };
				mailbox[bk_start + 2] = Square{ false, Color::empty, Piece::none };
			}
			else if (cm.side == CastleSide::queen)
			{
				mailbox[bk_start] = mailbox[bk_start - 2];
				mailbox[bqr_start] = mailbox[bk_start - 1];
				mailbox[bk_start - 1] = Square{ false, Color::empty, Piece::none };
				mailbox[bk_start - 2] = Square{ false, Color::empty, Piece::none };
			}
			break;
		}
		gameState.pop();
	}

	void Board::unmakeEnPMove(const Move& m) noexcept
	{
		auto epm = std::get<enPMove>(m);

		switch (toMove)
		{
		case Color::black:
			if (epm.right)
			{
				mailbox[index2index(epm.square, -1, 1)] = mailbox[epm.square];
				mailbox[index2index(epm.square, -1)] = Square{ true, Color::black, Piece::pawn };
				mailbox[epm.square] = Square{false, Color::empty, Piece::none};
			}
			else
			{
				mailbox[index2index(epm.square, -1, -1)] = mailbox[epm.square];
				mailbox[index2index(epm.square, -1)] = Square{ true, Color::black, Piece::pawn };
				mailbox[epm.square] = Square{ false, Color::empty, Piece::none };
			}
			break;
		case Color::white:
			if (epm.right)
			{
				mailbox[index2index(epm.square, 1, 1)] = mailbox[epm.square];
				mailbox[index2index(epm.square, 1)] = Square{ true, Color::white, Piece::pawn };
				mailbox[epm.square] = Square{ false, Color::empty, Piece::none };
			}
			else
			{
				mailbox[index2index(epm.square, 1, -1)] = mailbox[epm.square];
				mailbox[index2index(epm.square, 1)] = Square{ true, Color::white, Piece::pawn };
				mailbox[epm.square] = Square{ false, Color::empty, Piece::none };
			}
			break;
		}
		gameState.pop();
	}

	void Board::unmakePromoMove(const Move& m) noexcept
	{
		auto pm = std::get<promoMove>(m);

		mailbox[pm.base.to].piece = Piece::pawn;
		unmakeSimpleMove(pm.base);
	}

	void Board::printState()
	{
		// First, print the board
		// we start by printing the top border
		std::cout << "O===============O" << std::endl;
		for (int j = 8; j != 0; --j)
		{
			std::cout << "|"; // left border of jth rank
			for (int i = 1; i != 9; ++i)
			{
				std::cout << mailbox[index(j, i)].getSymbol() << "|";
			}
			std::cout << std::endl;
		}
		// bottom border
		std::cout << "O===============O" << std::endl;
		std::cout << "Current move: " << currMove << ". ";
		switch (toMove)
		{
		case Color::black:
			std::cout << "Black to move." << std::endl;
			break;
		case Color::white:
			std::cout << "White to move." << std::endl;
		}
	}
}