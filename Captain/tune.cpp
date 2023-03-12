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
    std::pair<Evaluator, Fitness> Tuner::tune()
    {
        std::pair<Evaluator, Fitness> peakFitness = std::make_pair(Evaluator(), std::numeric_limits<Fitness>::max());
        peakFitness.second = computeFitness(peakFitness.first);
        (*pop)[0] = peakFitness;
        std::size_t generations = 0;
        std::mt19937_64 g(aux::seed);
        while (generations < maxGenerations || peakFitness.second < 10)
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

            std::shuffle(pop->begin(), pop->begin() + 200, g);

            std::unique_ptr<Population> newpop = std::make_unique<Population>();
            auto k = newpop->begin();
            for (auto j = pop->begin() + 1; j != pop->begin() + 201; j += 2)
            {
                auto i = j - 1;
                for (std::size_t n = 0; n != 3; ++n)
                {
                    *k++ = std::make_pair(Evaluator::crossover(i->first, j->first).mutate(false), 0);
                }
                *k++ = std::make_pair(i->first.mutate(false), 0);
                *k++ = std::make_pair(j->first.mutate(false), 0);
            }
            pop = std::move(newpop);
            ++generations;
        }
        return peakFitness;
    }

    Fitness Tuner::computeFitness(const Evaluator& ev)
    {
        auto testposcount = testpositions.size();
        auto Tests = testposcount / 10;
        Fitness f = 0;
        for (std::size_t i = 0; i != Tests; ++i)
        {
            auto error = std::abs(testpositions[i].second - ev(testpositions[i].first));
            f += error;
        }
        return f / Tests;
    }

    std::pair<bool, eval::Eval> Tuner::searchPosition(const board::QBB& b)
    {
        engine::SearchSettings ss;
        ss.maxDepth = 2;
        ss.infiniteSearch = true;
        e->setSettings(ss);
        e->newGame();
        std::vector<std::uint64_t> posHash = { Tables::tt.initialHash(b) };
        std::vector<board::Move> moves = {};
        SearchFlags::searching.test_and_set();
        e->rootSearch(b, std::chrono::steady_clock::now(), moves, posHash);
        if (std::abs(e->eval - eval::Evaluator{}(b)) > 150)
            return std::make_pair(false, 0);
        else
            return std::make_pair(true, e->eval);
    }

    void Tuner::evalTestPositions()
    {
        decltype(testpositions) quietPositions;
        for (auto& [position, eval] : testpositions)
        {
            auto result = searchPosition(position);
            if (result.first)
                quietPositions.emplace_back(position, result.second);
        }
        testpositions = quietPositions;
    }
}
