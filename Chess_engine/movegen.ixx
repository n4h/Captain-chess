export module Movegen;

import <vector>;
import <variant>;

import Board;

export namespace movegen
{

	bool isInCheck(const board::Board& b);

	void genMoveInDirection(std::vector<board::Move>& ml, board::Board b, unsigned int i, int r, int f);

	std::vector<board::Move> genKnightMoves(board::Board b, unsigned int i);
	std::vector<board::Move> genBishopMoves(board::Board b, unsigned int i);
	std::vector<board::Move> genRookMoves(board::Board b, unsigned int i);
	std::vector<board::Move> genQueenMoves(board::Board b, unsigned int i);
	std::vector<board::Move> genKingMoves(board::Board b, unsigned int i);
	std::vector<board::Move> genPawnMoves(board::Board b, unsigned int i);
}