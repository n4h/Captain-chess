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

#ifndef CAPTAIN_TUNING_HPP
#define CAPTAIN_TUNING_HPP

#include <vector>
#include <utility>
#include <cstddef>
#include <cassert>
#include <algorithm>
#include <execution>
#include <random>
#include <optional>

namespace Tuning
{
    template<typename Agent, typename Fitness>
    using Population = std::vector<std::pair<Agent, Fitness>>;

    template<typename Agent, 
        typename Fitness, 
        typename GeneticOperators = typename Agent::GAOps>
    class Tuner : private GeneticOperators
    {
        Population<Agent, Fitness> pop;
        std::optional<typename Population<Agent, Fitness>::value_type> historical_best;
        using GeneticOperators::mutate;
        using GeneticOperators::crossover;
    public:
        Tuner(Population<Agent, Fitness> population)
            : pop(std::move(population))
        {
            assert(population.size() > 0);
            historical_best = pop[0];
        }

        Agent get_current_best() const
        {
            return pop[0].first;
        }

        Agent get_historical_best() const noexcept
        {
            return historical_best.value().first;
        }

        template<typename FitnessPolicy>
        void tune(double mutation_rate, double selectivity, std::size_t max_generations, FitnessPolicy fitness)
        {
            assert(0 <= mutation_rate <= 1 && 0 < selectivity < 1);
            std::random_device rd;
            std::minstd_rand g{rd()};
            std::size_t generations = 0;
            while (generations < max_generations || pop[0].second == 0)
            {
                std::for_each(std::execution::par, pop.begin(), pop.end(), [&fitness](auto& i) {
                    i.second = fitness(i.first);
                    });

                std::sort(std::execution::par, pop->begin(), pop->end(), [](const auto& a, const auto& b) {
                    return a.second < b.second;
                    });

                if (historical_best.has_value())
                {
                    if (pop[0].second < historical_best.value().second)
                    {
                        historical_best = pop[0];
                    }
                }
                else
                {
                    historical_best = pop[0];
                }

                const std::size_t popsize = pop.size();
                const std::size_t numSelected = pop.size() * selectivity;
                assert(numSelected >= 2);

                pop.resize(numSelected);
                const auto select_end = pop.end();

                std::for_each(std::execution::par, pop.begin(), pop.end(), [mutation_rate](auto& i) {
                    mutate(i, mutation_rate);
                    });

                if (pop.size() < popsize)
                {
                    std::shuffle(pop.begin(), pop.end(), g);
                    auto i = pop.begin();
                    auto j = pop.begin() + 1;
                    assert(j != pop.end());
                    std::size_t missing = popsize - pop.size();
                    for (std::size_t missing = popsize - pop.size(); missing != 0; --missing)
                    {
                        assert(i < select_end && i < select_end);
                        pop.push_back(crossover(*i, *j));
                        i = j + 1;
                        j = i + 1;
                        if (i == select_end)
                        {
                            i = pop.begin();
                            j = pop.begin() + 1;
                        }
                        else if (j == select_end)
                        {
                            j = pop.begin();
                        }
                    }
                }
                ++generations;
            }
        }
    };
}


#endif
