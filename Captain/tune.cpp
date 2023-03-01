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
#include <chrono>
#include <cmath>
#include <fstream>
#include <limits>

#include "tune.hpp"
#include "engine.hpp"
#include "tables.hpp"
#include "auxiliary.hpp"
#include "searchflags.hpp"

namespace Tuning
{
    void Tuner::loadTestPositions(std::string s)
    {
        std::ifstream fenData{ s };
        std::string fen = "";
        while (std::getline(fenData, fen))
        {
            if (!fen.empty())
            {
                testpositions.push_back(std::make_pair(board::QBB{fen}, 0));
            }
        }
        evalTestPositions();
    }
    Evaluator Tuner::tune()
    {
        std::pair<Evaluator, Fitness> peakFitness = std::make_pair(Evaluator(), std::numeric_limits<Fitness>::max());
        peakFitness.second = computeFitness(peakFitness.first);
        std::size_t generations = 0;
        std::mt19937_64 g(aux::seed);
        while (generations < maxGenerations)
        {
            std::shuffle(testpositions.begin(), testpositions.end(), g);
            std::for_each(std::execution::par, pop->begin(), pop->end(), [this](auto& i) {
                i.second = computeFitness(i.first);
                });

            std::sort(std::execution::par, pop->begin(), pop->end(), [](const auto& a, const auto& b) {
                return a.second < b.second;
                });

            if ((*pop)[0].second < peakFitness.second)
            {
                peakFitness = (*pop)[0];
            }

            std::shuffle(pop->begin(), pop->begin() + 4000, g);

            std::unique_ptr<Population> newpop = std::make_unique<Population>();
            auto k = newpop->begin();
            for (auto j = pop->begin() + 1; j != pop->begin() + 4001; j += 2)
            {
                auto i = j - 1;
                for (std::size_t n = 0; n != 5; ++n)
                {
                    *k++ = std::make_pair(Evaluator::crossover(i->first, j->first).mutate(false), 0);
                }
            }
            pop = std::move(newpop);
            ++generations;
        }
        return peakFitness.first;
    }

    Fitness Tuner::computeFitness(const Evaluator& ev)
    {
        auto testposcount = testpositions.size();
        auto Tests = testposcount / 10;
        Fitness f = 0;
        for (std::size_t i = 0; i != Tests; ++i)
        {
            auto error = std::pow(std::abs(testpositions[i].second - ev(testpositions[i].first)), 4);
            f += error;
        }
        return f;
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
            std::vector<std::uint64_t> posHash = { Tables::tt.initialHash(position) };
            std::vector<board::Move> moves = {};
            SearchFlags::searching.test_and_set();
            e->rootSearch(position, std::chrono::steady_clock::now(), moves, posHash);
            eval = e->eval;
        }
    }
}
