/*
Copyright 2022-2023, Narbeh Mouradian

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

#include <algorithm>
#include <execution>
#include <random>
#include <cstddef>
#include "tune.hpp"
#include "engine.hpp"

namespace Tuning
{
	Evaluator Tuner::tune()
	{
		std::pair<Evaluator, Fitness> peakFitness;
		std::size_t generations = 0;
		std::random_device rd;
		std::mt19937_64 g(rd());
		while (generations < maxGenerations)
		{
			std::shuffle(testpositions.begin(), testpositions.end(), g);
			std::for_each(std::execution::par, p.begin(), p.end(), [this](auto& i) {
				i.second = computeFitness(i.first);
				});

			std::sort(std::execution::par, p.begin(), p.end(), [](const auto& a, const auto& b) {
				return a.second < b.second;
				});

			if (p[0].second < peakFitness.second)
			{
				peakFitness = p[0];
			}

			std::shuffle(p.begin(), p.begin() + 4000, g);

			Population pnew;
			auto k = pnew.begin();
			for (auto j = p.begin() + 1; j != p.begin() + 4000; j += 2)
			{
				auto i = j - 1;
				for (std::size_t n = 0; n != 5; ++n)
				{
					*k++ = std::make_pair(Evaluator::crossover(i->first, j->first).mutate(false), 0);
				}
			}
			p = pnew;
			++generations;
		}
		return peakFitness.first;
	}

	Fitness Tuner::computeFitness(const Evaluator& ev)
	{
		return Fitness{};
	}
	void Tuner::evalTestPositions()
	{
		engine::SearchSettings ss;
		ss.maxDepth = 2;
		ss.infiniteSearch = true;
		e->setSettings(ss);
		for (auto& [position, eval] : testpositions)
		{
			e->newGame();
		}
	}
}
