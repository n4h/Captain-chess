export module Board;

import <array>;
import <vector>;
import <string>;
import <cstdint>;
import <stack>;
import <variant>;

export namespace board
{
	enum class Piece
	{
		// the numbers correspond to the material value of the pieces
		pawn = 1,
		queen = 9,
		rook = 5,
		bishop = 4,
		knight = 3,
		king = 500, // the king is essentially infinitely valuable
		none = 0 // no piece = no value
	};

	enum class Color
	{
		// 1 and -1 can be convenient (e.g. when reversing the evaluation of a piece/position)
		white = 1,
		empty = 0,
		black = -1
	};

	struct Square
	{
		bool occupied = false;
		Color color = Color::empty;
		Piece piece = Piece::none;
		Square() {}
		std::string getSymbol() noexcept;
		Square(bool b, Color c, Piece p) : occupied(b), color(c), piece(p) {}
	};
	bool operator==(const Square& s1, const Square& s2);
	bool operator!=(const Square& s1, const Square& s2);

	Square makeSquare(const char& i);
	
	// used to track elements of the game state that can't be simply
	// reversed by an unmake move function. For example, when a pawn
	// is moved, ply50 is reset to zero. But prior to that pawn move,
	// ply50 can be anything in the range (0-50). 
	struct irrState
	{
		int enP = -1;
		int ply50 = 0;
		bool wk = false; // castling rights for white kingside
		bool wq = false; // castling rights for white queenside
		bool bk = false; // castling rights for black kingside
		bool bq = false; // castling rights for black queenside
	};

	enum class CastleSide
	{ // no need to specify color since it will be clear from the move number
		none, king, queen
	};
	// motionMove, and all of the other move classes, must contain all of the info
	// necessary to both make and UNmake the move
	// a simpleMove is just a move from A to B - no castling, no promos
	struct simpleMove
	{
		std::uint_fast16_t from = 0;
		std::uint_fast16_t to = 0;
		Piece cap = Piece::none;

		simpleMove(std::uint_fast16_t f, std::uint_fast16_t t, Piece p)
			:from(f), to(t), cap(p) {}
		simpleMove(std::uint_fast16_t f, std::uint_fast16_t t)
			:from(f), to(t) {}
	};

	struct castleMove
	{
		CastleSide side = CastleSide::none;
		castleMove(CastleSide s) : side(s) {}
	};
	// en passant moves are very limited in what form they can take:
	// it can only be done by a pawn, it must capture a pawn, and 
	// it can only occur on specific locations on the board
	struct enPMove
	{
		int square = 0;
		bool right = false; // whether the en passant occurred from the left or the right
		// initializing to false is entirely arbitrary
		// left or right is based on the order 'a' < 'b' < ... < 'h'
		// < means "is to the left of"
		enPMove(int sq, bool side) : square(sq), right(side) {}
	};
	struct promoMove
	{
		simpleMove base; // this is the pawn move that results in promotion
		Piece promo = Piece::none;
	};
	using Move = std::variant<std::monostate, simpleMove, castleMove, enPMove, promoMove>;

	class Board
	{
		// called mailbox because each entry is a mailbox that you
		// open to check what's inside
		// Index 0 is the bottom left square (a1). Index always increases
		// when going to the right (from white's perspective) and when going
		// up (again from white's perspective).
		// see https://www.chessprogramming.org/File:Lerf.JPG for a visual
		
		// These bools keep track of castling status. bq = black queenside castling for example
	public:
		Color toMove = Color::empty;

		std::stack<irrState> gameState = {};

		int currMove = 1; // current move
	private:
		void makeSimpleMove(const Move& m) noexcept;
		void makeCastleMove(const Move& m) noexcept;
		void makeEnPMove(const Move& m) noexcept;
		void makePromoMove(const Move& m) noexcept;

		void unmakeSimpleMove(const Move& m) noexcept;
		void unmakeCastleMove(const Move& m) noexcept;
		void unmakeEnPMove(const Move& m) noexcept;
		void unmakePromoMove(const Move& m) noexcept;

		void changeColor() noexcept;
	public:
		std::array<Square, 64> mailbox{};
		Board(const std::string& fen); // construct mailbox from FEN string
		[[nodiscard]] std::vector<Move> genMoves();
		void printState();
		void makeMove(const Move& m) noexcept;
		void unmakeMove(const Move& m) noexcept;
	};
}