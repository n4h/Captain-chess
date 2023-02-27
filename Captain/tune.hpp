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

#include <cstddef>
#include <cstdint>
#include <utility>

#include "eval.hpp"
#include "engine.hpp"

namespace Tuning
{
	using eval::Evaluator;

	using Fitness = std::uint64_t;
	using Population = std::vector<std::pair<Evaluator, Fitness>>;
	using engine::Engine;

	class Tuner
	{
		Engine* e;
		std::vector<board::QBB> testpositions;
		Population p;
		std::size_t generations = 0;
		std::size_t maxGenerations = 0;
		Fitness computeFitness(const Evaluator& ev);
	public:
		void setParameters(std::size_t maxGenerations);
		Evaluator tune();
	};
}


#endif
