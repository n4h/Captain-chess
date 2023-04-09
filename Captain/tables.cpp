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

#include <intrin.h>

#pragma intrinsic(_BitScanForward64)

#include <random>
#include <cstddef>
#include <cstdlib>

#include "tables.hpp"
#include "types.hpp"
#include "auxiliary.hpp"


namespace Tables
{
    TTable tt{(1024 * 1024) / sizeof(Entry)};
    void TTable::initRandom()
    {
        std::random_device rd{};
        std::mt19937_64 rnum{rd()};

        for (std::size_t i = 0; i != 6; ++i)
        {
            for (std::size_t j = 0; j != 64; ++j)
            {
                whitePSQT[i][j] = rnum();
                blackPSQT[i][j] = rnum();
            }
        }

        wToMove = rnum();

        for (std::size_t i = 0; i != 4; ++i)
        {
            castling_first[i] = rnum();
        }

        for (std::size_t i = 0; i != 16; ++i)
        {
            castling[i] = 0;
            if (i & 0b1U) castling[i] ^= castling_first[0];
            if (i & 0b10U) castling[i] ^= castling_first[1];
            if (i & 0b100U) castling[i] ^= castling_first[2];
            if (i & 0b1000U) castling[i] ^= castling_first[3];
        }

        for (std::size_t i = 0; i != 8; ++i)
        {
            enPassant[i] = rnum();
        }
    }

    bool TTable::isBetterEntry(const Entry& curr, std::int16_t depth, unsigned char age)
    {
        if (curr.age < age)
        {
            return true;
        }

        return curr.depth < depth;
    }

    void TTable::tryStore(std::uint64_t hash, std::int16_t depth, Eval eval, Move m, char nodetype, unsigned char age, bool anyPruning)
    {
        (void)anyPruning;
        //if (anyPruning)
           // return;
        const auto& currEntry = (*this)[hash];
        if (isBetterEntry(currEntry, depth, age))
        {
            store(hash, depth, eval, m, nodetype, age);
        }
    }

    void TTable::store(std::uint64_t hash, std::int16_t depth, Eval eval, Move m, char nodetype, unsigned char age)
    {
        (*this)[hash].key = hash;
        (*this)[hash].depth = depth;
        (*this)[hash].eval = eval;
        (*this)[hash].move = m;
        (*this)[hash].nodeType = nodetype;
        (*this)[hash].age = age;
    }

    TTable::TTable(std::size_t N)
    {
        if (N > 0)
        {
            table = new Entry[N];
            this->sz = N;
        }
        else
        {
            table = nullptr;
            this->sz = 0;
        }
        initRandom();
    }

    TTable::~TTable()
    {
        delete[] table;
    }

    void TTable::clear()
    {
        for (std::size_t i = 0; i != sz; ++i)
        {
            table[i].eval = 0;
            table[i].key = 0;
            table[i].depth = 0;
            table[i].move = 0;
            table[i].nodeType = NONE;
            table[i].age = 0;
        }
    }

    Entry& TTable::operator[](std::uint64_t hash) noexcept
    {
        return table[hash % sz];
    }

    void TTable::resize(std::size_t newSize)
    {
        delete[] table;

        if (newSize > 0)
        {
            table = new Entry[newSize];
            sz = newSize;
        }
        else
        {
            table = nullptr;
            sz = 0;
        }
    }

    /*
    double TTable::capturePct(const board::QBB& b) const
    {
        std::size_t total = 0;
        std::size_t count = 0;
        for (std::size_t i = 0; i != sz; ++i)
        {
            if (table[i].age)
            {
                ++total;
                if (table[i].move && b.isCapture(table[i].move))
                {
                    ++count;
                }
            }
        }
        if (!total)
        {
            return 0;
        }
        return static_cast<double>(count * 100) / total;
    }

    double TTable::nodeTypePct(char nodetype) const
    {
        std::size_t total = 0;
        std::size_t count = 0;
        for (std::size_t i = 0; i != sz; ++i)
        {
            if (table[i].age)
            {
                ++total;
                if (table[i].nodeType == nodetype)
                {
                    ++count;
                }
            }
        }
        if (!total)
        {
            return 0;
        }
        return static_cast<double>(count * 100) / total;
    }

    double TTable::usedPct() const
    {
        std::size_t used = 0;
        for (std::size_t i = 0; i != sz; ++i)
        {
            if (table[i].age)
                ++used;
        }
        return sz ? static_cast<double>(used * 100) / sz : 0;
    }
    */

    std::uint64_t PawnHashTable::initialHash(Bitboard pawns) const noexcept
    {
        std::uint64_t hash = 0;
        aux::GetNextBit<board::square> nextpawn(pawns);
        while (nextpawn())
        {
            hash ^= pawnpositions[nextpawn.next];
        }
        return hash;
    }

    std::uint64_t PawnHashTable::incrementalUpdate(Bitboard pawnsOld, Bitboard pawnsNew) const noexcept
    {
        auto change = pawnsOld ^ pawnsNew;
        return initialHash(change);
    }
}