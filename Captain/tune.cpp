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

namespace Tuning
{
	Evaluator Tuner::tune()
	{
		std::pair<Evaluator, Fitness> peakFitness;
		while (generations < maxGenerations)
		{
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

			p.resize(4000);

			std::random_device rd;
			std::mt19937_64 g(rd());

			std::shuffle(p.begin(), p.end(), g);

			Population pnew;
			for (auto j = p.begin() + 1; j != p.end(); j += 2)
			{
				auto i = j - 1;
				for (std::size_t n = 0; n != 5; ++n)
				{
					pnew.push_back(this->crossover(*i, *j));
				}
			}
			p = std::move(pnew);
			++generations;
		}
		return peakFitness.first;
	}
}
