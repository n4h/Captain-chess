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
#include <limits>

namespace Tuning
{
    template<typename Agent, typename Fitness>
    using Population = std::vector<std::pair<Agent, Fitness>>;

    template<typename Agent, 
        typename Fitness, 
        typename GeneticOperators = typename Agent::GAOps>
    class GeneticTuner : private GeneticOperators
    {
        Population<Agent, Fitness> pop;
        std::optional<typename Population<Agent, Fitness>::value_type> historical_best;
        using GeneticOperators::mutate;
        using GeneticOperators::crossover;
    public:
        GeneticTuner(Population<Agent, Fitness> population)
            : pop(std::move(population))
        {
            assert(pop.size() > 0);
        }

        auto get_current_best() const
        {
            return pop[0];
        }

        auto get_historical_best() const noexcept
        {
            return historical_best.value();
        }

        template<typename FitnessPolicy>
        void tune(double mutation_rate, double selectivity, std::size_t max_generations, FitnessPolicy fitness)
        {
            assert(0 <= mutation_rate);
            assert(mutation_rate <= 1);
            assert(0 < selectivity);
            assert(selectivity < 1);
            std::random_device rd;
            std::minstd_rand g{rd()};
            std::size_t generations = 0;
            while (generations < max_generations)
            {
                std::for_each(std::execution::par, pop.begin(), pop.end(), [&fitness](auto& i) {
                    i.second = fitness(i.first);
                    });

                std::sort(std::execution::par, pop.begin(), pop.end(), [](const auto& a, const auto& b) {
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

                std::for_each(std::execution::par, pop.begin(), pop.end(), [this, mutation_rate](auto& i) {
                    this->mutate(i.first, mutation_rate);
                    });

                if (pop.size() < popsize)
                {
                    std::shuffle(pop.begin(), pop.end(), g);
                    auto i = pop.begin();
                    auto j = pop.begin() + 1;
                    assert(j != pop.end());
                    for (std::size_t missing = popsize - pop.size(); missing != 0; --missing)
                    {
                        assert(i < pop.begin() + numSelected && j < pop.begin() + numSelected);
                        const auto joffset = std::distance(pop.begin(), j);
                        pop.push_back(std::make_pair(crossover(i->first, j->first), std::numeric_limits<Fitness>::max()));
                        i = pop.begin() + joffset + 1;
                        j = i + 1;
                        if (i == pop.begin() + numSelected)
                        {
                            i = pop.begin();
                            j = pop.begin() + 1;
                        }
                        else if (j == pop.begin() + numSelected)
                        {
                            j = pop.begin();
                        }
                    }
                }
                ++generations;
            }
        }
    };

    template<typename Evaluator, typename Error>
    double find_best_K(Evaluator e, Error compute_error)
    {
        double K = 0.01;
        using ErrorType = decltype(compute_error(std::declval<Evaluator>(), std::declval<double>()));
        ErrorType error = compute_error(e, K);
        constexpr double h = 0.01;
        do
        {
            ErrorType new_error = compute_error(e, K + h);
            if (new_error > error)
                return K;
            error = new_error;
            K += h;
        } while (K < 55);
        return K;
    }

    // see https://www.chessprogramming.org/Texel%27s_Tuning_Method#Pseudo_Code
    template<typename Evaluator, typename Error>
    auto local_search(Evaluator e, Error compute_error, const double K)
    {
        using ErrorType = decltype(compute_error(std::declval<Evaluator>(), std::declval<double>()));
        std::pair<Evaluator, ErrorType > best = { e, compute_error(e, K) };
        bool improved = true;
        while (improved)
        {
            improved = false;
            for (std::size_t i = 0; i != e.evalTerms.size(); ++i)
            {
                e = best.first;
                e.evalTerms[i] += 1;
                ErrorType error = compute_error(e, K);
                if (error < best.second)
                {
                    best.first = e;
                    best.second = error;
                    improved = true;
                }
                else
                {
                    e.evalTerms[i] -= 2;
                    ErrorType error = compute_error(e, K);
                    if (error < best.second)
                    {
                        best.first = e;
                        best.second = error;
                        improved = true;
                    }
                }
            }
        }
        return best;
    }

    template<typename Evaluator, typename Error>
    auto local_search_one_iteration(Evaluator e, Error compute_error, const double K)
    {
        using ErrorType = decltype(compute_error(std::declval<Evaluator>(), std::declval<double>()));
        std::pair<Evaluator, ErrorType > best = { e, compute_error(e, K) };

        for (std::size_t i = 0; i != e.evalTerms.size(); ++i)
        {
            e = best.first;
            e.evalTerms[i] += 1;
            ErrorType error = compute_error(e, K);
            if (error < best.second)
            {
                best.first = e;
                best.second = error;
            }
            else
            {
                e.evalTerms[i] -= 2;
                ErrorType error = compute_error(e, K);
                if (error < best.second)
                {
                    best.first = e;
                    best.second = error;
                }
            }
        }
        return best;
    }
}


#endif
