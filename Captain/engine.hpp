/*
Copyright 2022, Narbeh Mouradian

Captain is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Captain is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <https://www.gnu.org/licenses/>.

*/

#ifndef ENGINE_H
#define ENGINE_H

#include <algorithm>
#include <limits>
#include <utility>
#include <cstdint>
#include <chrono>
#include <atomic>
#include <iterator>
#include <array>
#include <string>

#include "board.hpp"
#include "movegen.hpp"
#include "eval.hpp"
#include "auxiliary.hpp"
#include "searchFlags.hpp"
#include "transpositiontable.hpp"

namespace engine
{
	using namespace std::literals::chrono_literals;

	constexpr auto negInf = std::numeric_limits<std::int32_t>::min() + 1;
	constexpr auto posInf = std::numeric_limits<std::int32_t>::max();

	struct SearchSettings
	{
		std::size_t maxDepth = std::numeric_limits<std::size_t>::max();
		std::size_t maxNodes = std::numeric_limits<std::size_t>::max();
		std::size_t movestogo = std::numeric_limits<std::size_t>::max();
		bool infiniteSearch = false;
		bool ponder = false;
		std::chrono::milliseconds maxTime = std::chrono::milliseconds::max();
		std::chrono::milliseconds wmsec = 0ms;
		std::chrono::milliseconds bmsec = 0ms;
		std::chrono::milliseconds winc = 0ms;
		std::chrono::milliseconds binc = 0ms;
	};

	class Engine
	{
	public:
		void playBestMove(const board::Board& bCopy, std::chrono::time_point<std::chrono::steady_clock> s);
		double getEval();
		Engine() {}
		void setSettings(SearchSettings ss) noexcept { settings = ss; }
		void setTTable(TTable::TTable*);
	private:
		std::string move2uciFormat(board::Move);
		SearchSettings settings;
		std::chrono::time_point<std::chrono::steady_clock> searchStart;
		std::size_t nodes = 0;
		std::size_t hash = 0;
		bool engineW = true;
		board::Board b;
		std::chrono::milliseconds moveTime = 0ms;
		TTable::TTable* tt = nullptr;
		bool shouldStop() noexcept;
		void initialHash();

		enum SearchType {ABSearch, QSearch};

		template<bool wToMove>
		board::Move rootSearch()
		{
			std::array<board::Move, 256> moves;
			std::size_t j = movegen::genMoves<wToMove>(b, moves, 0);

			for (std::size_t i = 0; i != j; ++i)
			{
				rootMoves[i].first = moves[i];
				rootMoves[i].second = negInf;
			}

			std::int32_t worstCase = negInf;

			for (unsigned int k = 1; k <= posInf; ++k)
			{
				sync_cout << "info string iterative deepening " << k << sync_endl;
				worstCase = negInf;
				std::int32_t score = negInf;



				for (std::size_t i = 0; i != j; ++i)
				{
					if (!searchFlags::searching.test())
					{
						// if the search ends too early then rootMoves[0].first
						// may not be a legal move, so loop through until first legal
						// move
						for (std::size_t mv = 0; mv != j; ++mv)
						{
							b.makeMove<wToMove>(rootMoves[mv].first);
							if (movegen::isInCheck<wToMove>(b))
							{
								b.unmakeMove<wToMove>(rootMoves[mv].first);
							}
							else
							{
								return rootMoves[mv].first;
							}
						}
					}

					b.makeMove<wToMove>(rootMoves[i].first);
					if (movegen::isInCheck<wToMove>(b))
					{
						b.unmakeMove<wToMove>(rootMoves[i].first);
						continue;
					}

					sync_cout << "info currmove " << move2uciFormat(rootMoves[i].first) << sync_endl;
					sync_cout << "info nodes " << nodes << sync_endl;
					rootMoves[i].second = -1 * alphaBetaSearch<!wToMove, ABSearch>(negInf, -1 * worstCase, k - 1);
					score = std::max(score, rootMoves[i].second);
					b.unmakeMove<wToMove>(rootMoves[i].first);
					if (score > worstCase)
						worstCase = score;
				}

				std::stable_sort(rootMoves.begin(), rootMoves.begin() + j, [](const auto& a, const auto& b) {
					return a.second > b.second;
					});
			}
			return rootMoves[0].first;
		}

		template<bool wToMove, SearchType s, bool prevMoveNull = false>
		std::int32_t alphaBetaSearch(std::int32_t alpha, std::int32_t beta, int depth)
		{
			++nodes;
			if (shouldStop())
				searchFlags::searching.clear();
			std::int32_t checkPos;
			if ((b.flags & board::Board::ply50FlagMask) == 50)
				return 0;
			if constexpr (s == ABSearch)
			{
				if (depth <= 0)
					return alphaBetaSearch<wToMove, QSearch>(alpha, beta, depth - 1);
			}
			else // s == QSearch
			{
				checkPos = eval::evaluate<wToMove>(b);
				if (checkPos > beta)
					return checkPos;
				alpha = std::max(alpha, checkPos);
			}

			std::array<board::Move, 256> moves;

			std::size_t j = movegen::genMoves<wToMove>(b, moves, 0);

			if constexpr (s == ABSearch)
			{
				std::sort(moves.begin(), moves.begin() + j, [](const auto& a, const auto& b) {
					return a > b;
					});
			}
			else // s == QSearch
			{
				auto new_j = std::remove_if(moves.begin(), moves.begin() + j, [](auto k) {return !board::isCapture(k); });
				j = std::distance(moves.begin(), new_j);

				std::sort(moves.begin(), moves.begin() + j, [this](const auto& a, const auto& c) {
					const auto atA = aux::setbit(board::getMoveInfo<constants::fromMask>(a));
					const auto atC = aux::setbit(board::getMoveInfo<constants::fromMask>(c));

					return (eval::getCaptureValue(a) - board::getPieceValue(b.getPieceType(atA)))
					> (eval::getCaptureValue(c) - board::getPieceValue(b.getPieceType(atC)));
					});
			}

			std::int32_t currEval;
			if constexpr (s == ABSearch)
			{
				currEval = negInf;
			}
			else // s == QSearch
			{
				currEval = alpha;
			}
			
			if constexpr (s == ABSearch && !prevMoveNull)
			{
				if (!movegen::isInCheck<wToMove>(b))
				{
					board::Move m = b.getHeading();
					b.makeMove<wToMove, true>(m);
					currEval = std::max(currEval, -1 * alphaBetaSearch<!wToMove, ABSearch, true>(-1 * beta, -1 * alpha, depth - 2));
					b.unmakeMove<wToMove, true>(m);
					alpha = std::max(alpha, currEval);
					if (alpha > beta)
						return currEval;
				}
			}

			for (std::size_t i = 0; i != j; ++i)
			{
				if (!searchFlags::searching.test())
				{
					if constexpr (s == ABSearch)
					{
						return eval::evaluate<wToMove>(b);
					}
					else
					{
						return std::max(checkPos, currEval);
					}
				}

				if constexpr (s == QSearch)
				{
					if ((checkPos + (std::int32_t)eval::getCaptureValue(moves[i])) < alpha)
						continue;
				}
				
				b.makeMove<wToMove>(moves[i]);
				if (movegen::isInCheck<wToMove>(b))
				{
					b.unmakeMove<wToMove>(moves[i]);
					continue;
				}
				currEval = std::max(currEval, -1 * alphaBetaSearch<!wToMove, s>(-1 * beta, -1 * alpha, depth - 1));
				b.unmakeMove<wToMove>(moves[i]);

				alpha = std::max(currEval, alpha);
				if (alpha > beta)
					return currEval;
			}
			return currEval;
		}
		
		std::int32_t eval = 0;
		// 218 = current max number of moves in chess position
		// 256 = leeway for pseudolegal move generation
		std::array<std::pair<board::Move, std::int32_t>, 256> rootMoves;
		std::array<board::Move, 256> currline;
	};
}
#endif