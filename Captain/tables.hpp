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

#ifndef TRANSPOSITION_TABLE_H
#define TRANSPOSITION_TABLE_H

#include <cstdint>
#include <array>
#include <algorithm>

#include "types.hpp"

namespace Tables
{
    enum : char {NONE = 0, PV = 1, ALL = 2, CUT = 3};
    
    struct Entry
    {
        std::uint64_t key = 0;
        std::int16_t depth = 0;
        Eval eval = 0;
        Move move = 0;
        char nodeType = NONE;
        unsigned char age = 0;
    };
    
    class TTable
    {
        Entry* table = nullptr;
        std::size_t sz = 0;

        void initRandom();
        static bool isBetterEntry(const Entry& curr, std::int16_t depth, unsigned char age);
    public:
        std::array<std::array<std::uint64_t, 64>, 6> whitePSQT;
        std::array<std::array<std::uint64_t, 64>, 6> blackPSQT;
        std::uint64_t wToMove;
        std::array<std::uint64_t, 4> castling_first;
        std::array<std::uint64_t, 16> castling;
        std::array<std::uint64_t, 8> enPassant;

        TTable(std::size_t);
        TTable() { initRandom(); }
        ~TTable();
        
        void clear();
        
        void tryStore(std::uint64_t hash, std::int16_t depth, Eval eval, Move m, char nodetype, unsigned char age, bool anyPruning);
        void store(std::uint64_t hash, std::int16_t depth, Eval eval, Move m, char nodetype, unsigned char age);

        Entry& operator[](std::uint64_t hash) noexcept;

        void resize(std::size_t);

        //double capturePct(const board::QBB& b) const;
        //double nodeTypePct(char nodetype) const;
        //double usedPct() const;
        TTable(const TTable&) = delete;
        TTable& operator=(const TTable&) = delete;
        TTable(TTable&&) = delete;
        TTable& operator=(TTable&&) = delete;
    };

    extern TTable tt;

    struct PawnEntry
    {
        Bitboard myPawns = 0;
        Bitboard theirPawns = 0;
        Eval eval = 0;
        constexpr bool valid(Bitboard mine, Bitboard theirs) const noexcept
        {
            return myPawns == mine && theirPawns == theirs;
        }
    };

    class PawnHashTable
    {
        std::array<PawnEntry, (1024 * 1024) / sizeof(PawnEntry)> entries;
        std::array<std::uint64_t, 64> pawnpositions;
    public:
        constexpr PawnEntry& operator[](std::uint64_t hash) noexcept
        {
            return entries[hash % entries.size()];
        }
        std::uint64_t initialHash(Bitboard pawns) const noexcept;
        std::uint64_t incrementalUpdate(Bitboard pawnsOld, Bitboard pawnsNew) const noexcept;
        constexpr void clear()
        {
            entries.fill(PawnEntry{.myPawns = 0, .theirPawns = 0, .eval = 0});
        }
    };

    class KillerTable
    {
        std::array<std::array<Move, 2>, 16> killers;
    public:
        KillerTable()
        {
            for (auto& i : killers)
            {
                i[0] = 0;
                i[1] = 0;
            }
        }

        Move getKiller(std::size_t depth, std::size_t killer)
        {
            depth -= 1;
            if (depth <= 15)
            {
                return killers[depth][killer];
            }
            return 0;
        }

        void storeKiller(Move m, std::size_t depth)
        {
            depth -= 1;
            if (depth <= 15)
            {
                if (m != killers[depth][0])
                {
                    killers[depth][1] = killers[depth][0];
                    killers[depth][0] = m;
                }
            }
        }
    };

    class HistoryTable
    {
        std::array<std::array<std::uint32_t, 64>, 6> history;
    public:
        HistoryTable()
        {
            for (auto& i : history)
            {
                for (auto& j : i)
                {
                    j = 0;
                }
            }
        }
        constexpr std::int16_t getHistoryScore(unsigned int pieceCodeIdx, board::square to)
        {
            auto score = history[pieceCodeIdx][to];
            score = std::clamp(score, 0U, static_cast<unsigned>(std::numeric_limits<std::int16_t>::max()));
            return static_cast<std::int16_t>(score);
        }
        constexpr void updateHistory(unsigned int pieceCodeIdx, board::square to, int depth)
        {
            history[pieceCodeIdx][to] += depth * depth;
        }
    };

}
#endif