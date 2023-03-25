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

namespace Tuning
{
    template<typename Agent, typename Fitness>
    using Population = std::vector<std::pair<Agent, Fitness>>;


    struct GeneticOperators
    {

    };

    template<typename Agent, 
        typename Fitness>
    class Tuner
    {
        Population<Agent, Fitness> pop;
    public:
        Tune(Population<Agent, Fitness> population) 
            : pop(std::move(population)) {}

        Agent get_best() const
        {
            return pop[0];
        }

        template<typename Stoppage>
        tune(Stoppage stop)
        {
            while (!stop())
            {
                std::for_each(std::execution::par, pop.begin(), pop.end(), [this](auto& i) {
                    i.second = FitnessEvaluator<Fitness>(i.first);
                    });
            }
        }
    };
}


#endif
