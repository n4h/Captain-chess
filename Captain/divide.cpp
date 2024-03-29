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

#include <cstddef>
#include <iostream>
#include <sstream>
#include <string>
#include <cassert>
#include <syncstream>

#include "divide.hpp"
#include "board.hpp"
#include "constants.hpp"
#include "auxiliary.hpp"
#include "perft.hpp"


namespace divide
{
    std::string printMove(Move m, board::Color stm)
    {
        bool wmove = stm == board::Color::White;
        board::square from = static_cast<board::square>(board::getMoveInfo<constants::fromMask>(m));
        board::square to = static_cast<board::square>(board::getMoveInfo<constants::toMask>(m));
        std::ostringstream p;

        auto fromFile = aux::file(from);
        auto toFile = aux::file(to);
        auto fromRank = wmove ? aux::rank(from) : 7 - aux::rank(from);
        auto toRank = wmove ? aux::rank(to) : 7 - aux::rank(to);

        p << aux::file2char(fromFile);
        p << fromRank + 1;
        p << aux::file2char(toFile);
        p << toRank + 1;

        switch (board::getPromoPiece(m))
        {
        case board::queens:
            p << 'Q';
            return p.str();
        case board::rooks:
            p << 'R';
            return p.str();
        case board::bishops:
            p << 'B';
            return p.str();
        case board::knights:
            p << 'N';
            return p.str();
        default:
            return p.str();
        }
    }
    std::size_t perftDivide(const board::QBB& b, std::size_t t)
    {
        moves::Movelist moves;
        moves::genMoves<moves::QSearch>(b, moves);
        moves::genMoves<!moves::QSearch, moves::Quiets>(b, moves);
        std::size_t total = 0;
        std::osyncstream perftdivide_out(std::cout);
        board::QBB bcopy = b;
        for (std::size_t i = 0; i != moves.size(); ++i)
        {
            bcopy.makeMove(moves[i]);
            perft::Perft p{ bcopy, t - 1 };
            total += p.getResult();
            board::Color stm = b.isWhiteToPlay() ? board::Color::White : board::Color::Black;
            perftdivide_out << printMove(moves[i], stm) << ": " << p.getResult() << std::endl;
            perftdivide_out.emit();
            p.reset();
            bcopy = b;
        }
        perftdivide_out << "total: " << total << std::endl;
        perftdivide_out.emit();
        return total;
    }
}