export module aux;

// auxiliary functions that have broad applicability throughout
// the codebase, such as functions for converting between array
// indices and rank and file numbers.
export namespace aux
{
	// Used for storing the starting positions of all kings
	// and rooks. This is used for implementing castling without
	// any magic constants
	constexpr int wk_start = 4;
	constexpr int bk_start = 60;
	constexpr int wkr_start = 7; // white king's rook etc.
	constexpr int bkr_start = 63;
	constexpr int wqr_start = 56;
	constexpr int bqr_start = 0;

	// Piece values (according to AlphaZero)
	constexpr double king_val = 500;
	constexpr double queen_val = 9.5;
	constexpr double knight_val = 3.05;
	constexpr double bishop_val = 3.33;
	constexpr double rook_val = 5.63;
	constexpr double pawn_val = 1;

	// rank = rows, file = columns. Both rank and column
    // go from 1 to 8 inclusive. This translates (rank, file)
    // to the right index on a 64 square array
	constexpr unsigned int index(int rank, int file)
	{
		return (rank - 1) * 8 + (file - 1);
	}
	
	
	// increment the file number by c. There are 8 squares per
	// row, so if file + c is greater than 8, just start counting from
	// 1 again (e.g. 7 + 2 --> 1)
	constexpr unsigned int incFile(int file, int c)
	{
		return file + c < 9 ? file + c : (file + c) % 8;
	}

	// isNumber only checks if i is in the range '1'-'8'
	// use isMoveNumber for '1'-'9'
	constexpr bool isNumber(const char& i)
	{
		return (i == '1' || i == '2' || i == '3' || i == '4' || i == '5' || i == '6' || i == '7' || i == '8');
	}

	constexpr bool isMoveNumber(const char& i)
	{
		return isNumber(i) || i == '9' || i == '0';
	}

	constexpr bool isFile(const char& i)
	{
		return (i == 'a' || i == 'b' || i == 'c' || i == 'd' || i == 'e' || i == 'f' || i == 'g' || i == 'h');
	}

	constexpr int fileNumber(const char& i)
	{
		switch (i)
		{
		case 'a':
			return 1;
		case 'b':
			return 2;
		case 'c':
			return 3;
		case 'd':
			return 4;
		case 'e':
			return 5;
		case 'f':
			return 6;
		case 'g':
			return 7;
		case 'h':
			return 8;
		default:
			return 0;
		}
	}

	// file and rank satisfy index(rank(x),file(x)) = x;
	// so we can convert between the index of a 64 elem
	// array and the rank/file associated with that index
	constexpr int file(int x)
	{
		return ((x + 8) % 8) + 1;
	}

	constexpr int rank(int x)
	{
		return static_cast<int>((x + 8) / 4);
	}

	// takes and index, and returns a new index
	// after offsetting the old index by r ranks
	// and f files
	constexpr unsigned int index2index(int index, int r = 0, int f = 0)
	{
		return aux::index( rank(index) + r, file(index) + f);
	}

	constexpr bool isIndex(unsigned int i)
	{
		return i >= 0 && i <= 63;
	}

	constexpr bool isWhiteDoublePawnMove(int from, int to)
	{
		return (rank(from) == 2 && rank(to) == 4);
	}

	constexpr bool isBlackDoublePawnMove(int from, int to)
	{
		return (rank(from) == 7 && rank(to) == 5);
	}

	constexpr bool enPavail(int enP)
	{
		return enP >= 0 && enP <= 63;
	}
}