#ifndef ENGINE_H
#define ENGINE_H

#include <algorithm>
#include <limits>
#include <utility>
#include <cstdint>
#include <chrono>
#include <atomic>

#include "board.hpp"
#include "movegen.hpp"
#include "eval.hpp"
#include "auxiliary.hpp"
#include "searchFlags.hpp"

namespace engine
{
	using namespace std::literals::chrono_literals;
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
		void playBestMove(board::Board b, std::chrono::time_point<std::chrono::steady_clock> s);
		double getEval();
		Engine() {}
		void setSettings(SearchSettings ss) noexcept { settings = ss; }
	private:
		SearchSettings settings;
		std::chrono::time_point<std::chrono::steady_clock> searchStart;
		std::size_t nodes = 0;
		bool engineW = true;
		bool shouldStop() noexcept;


		template<bool wToMove>
		board::Move rootSearch(board::Board& b)
		{
			std::size_t j = movegen::genMoves<wToMove>(b, rootMoves, 0);

			for (std::size_t i = 0; i != j; ++i)
			{
				rootScoredMoves[i].first = rootMoves[i];
				rootScoredMoves[i].second = std::numeric_limits<std::int32_t>::min();
			}

			std::int32_t worstCase = std::numeric_limits<std::int32_t>::min();

			for (unsigned int k = 1; k <= std::numeric_limits<unsigned int>::max(); ++k)
			{
				worstCase = std::numeric_limits<std::int32_t>::min();
				std::int32_t score = std::numeric_limits<std::int32_t>::min();

				for (std::size_t i = 0; i != j; ++i)
				{
					if (!searchFlags::searching.test())
						return rootScoredMoves[0].first;
					b.makeMove<wToMove>(rootScoredMoves[i].first);
					score = std::max(score, rootScoredMoves[i].second = -1 * alphaBetaSearch<!wToMove>(b, std::numeric_limits<std::int32_t>::min(), -1 * worstCase, 1, 0));
					b.unmakeMove<wToMove>(rootScoredMoves[i].first);
					if (score > worstCase)
						worstCase = score;
				}

				std::stable_sort(&rootScoredMoves[0], &rootScoredMoves[j], [](const auto& a, const auto& b) {
					return a.second > b.second;
					});
			}
			return rootScoredMoves[0].first;
		}

		template<bool wToMove>
		std::int32_t quiesceSearch(board::Board& b, std::int32_t alpha, std::int32_t beta, int depth, std::size_t i)
		{
			if (shouldStop())
				searchFlags::searching.clear();
			const auto checkPos = eval::evaluate<wToMove>(b);
			if (checkPos > beta)
				return checkPos;

			alpha = std::max(alpha, checkPos);

			std::size_t j = movegen::genMoves<wToMove>(b, internalMoves, i);
			auto new_j = std::remove_if(&internalMoves[i], &internalMoves[j], [](auto k) {return !board::isCapture(k); });
			j = i + (new_j - &internalMoves[i]);

			std::sort(&internalMoves[i], &internalMoves[j], [&b](const board::Move& a, const board::Move& c) {
				const auto atA = aux::setbit(board::getMoveInfo<constants::fromMask>(a));
				const auto atC = aux::setbit(board::getMoveInfo<constants::fromMask>(c));

				return (eval::getCaptureValue(a) - board::getPieceValue(b.getPieceType(atA)))
					> (eval::getCaptureValue(c) - board::getPieceValue(b.getPieceType(atC)));
				});

			auto currEval = alpha;

			for (; i != j; ++i)
			{
				if (!searchFlags::searching.test())
				{
					return std::max(checkPos, currEval);
				}
				if ((checkPos + (std::int32_t)eval::getCaptureValue(internalMoves[i])) < alpha)
					continue;

				b.makeMove<wToMove>(internalMoves[i]);
				currEval = std::max(currEval, -1 * quiesceSearch<!wToMove>(b, -1 * beta, -1 * alpha, depth + 1, j));
				b.unmakeMove<wToMove>(internalMoves[i]);

				alpha = std::max(alpha, currEval);

				if (alpha > beta)
					return currEval;
			}
			
			return currEval;
		}

		template<bool wToMove>
		std::int32_t alphaBetaSearch(board::Board& b, std::int32_t alpha, std::int32_t beta, int depth, std::size_t i)
		{
			if (shouldStop())
				searchFlags::searching.clear();
			if (depth >= 5)
				return quiesceSearch<wToMove>(b, alpha, beta, 1, i);

			std::size_t j = movegen::genMoves<wToMove>(b, internalMoves, i);

			std::sort(&internalMoves[i], &internalMoves[j], [](const board::Move& a, const board::Move& b) {
				return a > b;
				});

			std::int32_t currEval = std::numeric_limits<std::int32_t>::min();

			for (; i != j; ++i)
			{
				if (!searchFlags::searching.test())
					return std::max(alpha, currEval);
				b.makeMove<wToMove>(internalMoves[i]);
				currEval = std::max(currEval, -1 * alphaBetaSearch<!wToMove>(b, -1 * beta, -1 * alpha, depth + 1, j));
				b.unmakeMove<wToMove>(internalMoves[i]);
				alpha = std::max(currEval, alpha);
				if (alpha > beta)
					break;
			}
			return currEval;
		}
		
		std::int32_t eval = 0;
		// 218 = current max number of moves in chess position
		// 256 = leeway for pseudolegal move generation
		board::Move rootMoves[256];
		std::pair<board::Move, std::int32_t> rootScoredMoves[256];
		board::Move internalMoves[2048];
	};
}
#endif