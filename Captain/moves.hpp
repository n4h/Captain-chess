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

#ifndef MOVEGEN_H
#define MOVEGEN_H

#include <immintrin.h>
#include <intrin.h>

#pragma intrinsic(__popcnt64)
#pragma intrinsic(_BitScanForward64)


#include <cstdint>
#include <cstddef>
#include <type_traits>
#include <array>
#include <cassert>
#include "board.hpp"
#include "types.hpp"
#include "auxiliary.hpp"
#include "constants.hpp"

namespace moves
{

    using AttackMap = Bitboard;

    template<typename T>
    concept BitboardOrSquare = std::is_same_v <T, Bitboard> || std::is_same_v<T, board::square>;

    constexpr Bitboard getBB(BitboardOrSquare auto x)
    {
        if constexpr (std::is_same_v<decltype(x), Bitboard>)
        {
            return x;
        }
        else if constexpr (std::is_same_v<decltype(x), board::square>)
        {
            return aux::setbit(x);
        }
        else
        {
            return 0ULL;
        }
    }

    struct ScoredMove
    {
        ScoredMove(Move _m): m(_m), score(0) {}
        ScoredMove() {}
        Move m = 0;
        std::int16_t score = 0;
    };
    constexpr bool operator<(const ScoredMove& s1, const ScoredMove& s2)
    {
        return s1.score < s2.score;
    }
    constexpr bool operator>(const ScoredMove& s1, const ScoredMove& s2)
    {
        return s1.score > s2.score;
    }
    constexpr bool operator==(const ScoredMove& s1, const ScoredMove& s2)
    {
        return s1.score == s2.score;
    }
    constexpr bool operator<=(const ScoredMove& s1, const ScoredMove& s2)
    {
        return s1.score <= s2.score;
    }
    constexpr bool operator>=(const ScoredMove& s1, const ScoredMove& s2)
    {
        return s1.score >= s2.score;
    }

    template<typename T = Move, std::size_t N = 218>
    struct Movelist
    {
        static_assert(std::is_same_v<T, Move> || std::is_same_v<T, ScoredMove>);
    private:
        std::array<T, N> ml;
        std::size_t i = 0;
    public:
        void push_back(T m)
        {
            ml[i++] = m;
        }
        T& operator[](std::size_t k)
        {
            return ml[k];
        }
        auto begin()
        {
            return ml.begin();
        }
        auto end()
        {
            return ml.begin() + i;
        }
        auto back()
        {
            return this->end() - 1;
        }
        void clear()
        {
            i = 0;
        }
        void remove_moves_if(auto begin, auto end, auto criteria)
        {
            auto last = std::remove_if(begin, end, criteria);
            i -= std::distance(last, end);
        }
        constexpr std::size_t size() const noexcept
        {
            return i;
        }
        constexpr std::size_t max_size() const noexcept
        {
            return N;
        }
    };

    constexpr Move constructMove(board::square from, board::square to, std::uint32_t type = constants::QMove)
    {
        Move m = from;
        m |= to << constants::toMaskOffset;
        m |= type << constants::moveTypeOffset;
        return m;
    }

    constexpr Move constructKSCastle()
    {
        Move m = board::e1;
        m |= board::g1 << constants::toMaskOffset;
        m |= constants::KSCastle << constants::moveTypeOffset;
        return m;
    }

    constexpr Move constructQSCastle()
    {
        Move m = board::e1;
        m |= board::c1 << constants::toMaskOffset;
        m |= constants::QSCastle << constants::moveTypeOffset;
        return m;
    }

    bool isLegalMove(const board::QBB& b, Move m);

    // trying out "Kogge Stone" algorithms
    // https://www.chessprogramming.org/Kogge-Stone_Algorithm
    constexpr AttackMap KSNorth(Bitboard occ, BitboardOrSquare auto idx)
    {
        Bitboard g = getBB(idx);
        Bitboard p = ~occ;
        g |= p & (g << 8);
        p &= p << 8;
        g |= p & (g << 16);
        p &= p << 16;
        g |= p & (g << 32);
        return g << 8;
    }

    constexpr AttackMap KSSouth(Bitboard occ, BitboardOrSquare auto idx)
    {
        Bitboard g = getBB(idx);
        Bitboard p = ~occ;
        g |= p & (g >> 8);
        p &= p >> 8;
        g |= p & (g >> 16);
        p &= p >> 16;
        g |= p & (g >> 32);
        return g >> 8;
    }

    constexpr AttackMap KSWest(Bitboard occ, BitboardOrSquare auto idx)
    {
        Bitboard g = getBB(idx);
        Bitboard p = ~occ;
        Bitboard H = board::fileMask(board::h1);
        p &= ~H;
        g |= p & (g >> 1);
        p &= p >> 1;
        g |= p & (g >> 2);
        p &= p >> 2;
        g |= p & (g >> 4);
        return (g >> 1) & ~H;
    }

    constexpr AttackMap KSEast(Bitboard occ, BitboardOrSquare auto idx)
    {
        Bitboard g = getBB(idx);
        Bitboard p = ~occ;
        Bitboard A = board::fileMask(board::a1);
        p &= ~A;
        g |= p & (g << 1);
        p &= p << 1;
        g |= p & (g << 2);
        p &= p << 2;
        g |= p & (g << 4);
        return (g << 1) & ~A;
    }

    constexpr AttackMap KSNorthEast(Bitboard occ, BitboardOrSquare auto idx)
    {
        Bitboard g = getBB(idx);
        Bitboard p = ~occ;
        Bitboard A = board::fileMask(board::a1);
        p &= ~A;
        g |= p & (g << 9);
        p &= p << 9;
        g |= p & (g << 18);
        p &= p << 18;
        g |= p & (g << 36);
        return (g << 9) & ~A;
    }

    constexpr AttackMap KSSouthEast(Bitboard occ, BitboardOrSquare auto idx)
    {
        Bitboard g = getBB(idx);
        Bitboard p = ~occ;
        Bitboard A = board::fileMask(board::a1);
        p &= ~A;
        g |= p & (g >> 7);
        p &= p >> 7;
        g |= p & (g >> 14);
        p &= p >> 14;
        g |= p & (g >> 28);
        return (g >> 7) & ~A;
    }

    constexpr AttackMap KSNorthWest(Bitboard occ, BitboardOrSquare auto idx)
    {
        Bitboard g = getBB(idx);
        Bitboard p = ~occ;
        Bitboard H = board::fileMask(board::h1);
        p &= ~H;
        g |= p & (g << 7);
        p &= p << 7;
        g |= p & (g << 14);
        p &= p << 14;
        g |= p & (g << 28);
        return (g << 7) & ~H;
    }

    constexpr AttackMap KSSouthWest(Bitboard occ, BitboardOrSquare auto idx)
    {
        Bitboard g = getBB(idx);
        Bitboard p = ~occ;
        Bitboard H = board::fileMask(board::h1);
        p &= ~H;
        g |= p & (g >> 9);
        p &= p >> 9;
        g |= p & (g >> 18);
        p &= p >> 18;
        g |= p & (g >> 36);
        return (g >> 9) & ~H;
    }

    constexpr AttackMap KSRank(Bitboard occ, BitboardOrSquare auto idx)
    {
        return KSEast(occ, idx) | KSWest(occ, idx);
    }

    constexpr AttackMap KSFile(Bitboard occ, BitboardOrSquare auto idx)
    {
        return KSNorth(occ, idx) | KSSouth(occ, idx);
    }

    constexpr AttackMap KSDiag(Bitboard occ, BitboardOrSquare auto idx)
    {
        return KSNorthWest(occ, idx) | KSSouthEast(occ, idx);
    }

    constexpr AttackMap KSAntiDiag(Bitboard occ, BitboardOrSquare auto idx)
    {
        return KSNorthEast(occ, idx) | KSSouthWest(occ, idx);
    }

    constexpr AttackMap KSAllOrth(Bitboard occ, BitboardOrSquare auto idx)
    {
        return KSRank(occ, idx) | KSFile(occ, idx);
    }

    constexpr AttackMap KSAllDiag(Bitboard occ, BitboardOrSquare auto idx)
    {
        return KSDiag(occ, idx) | KSAntiDiag(occ, idx);
    }

    constexpr AttackMap KSAll(Bitboard occ, BitboardOrSquare auto idx)
    {
        return KSAllOrth(occ, idx) | KSAllDiag(occ, idx);
    }

    // move generation is based on "Hyperbola Quintessence" algorithm
    // https://www.chessprogramming.org/Hyperbola_Quintessence

    template<typename T>
    AttackMap hypqDiag(Bitboard o, T idx)
    {
        if constexpr (std::is_same_v<T, board::square>)
        {
            o &= board::diagMask(idx);
            Bitboard r = aux::setbit(idx);
            Bitboard orev = _byteswap_uint64(o);
            Bitboard rrev = _byteswap_uint64(r);
            return board::diagMask(idx) & ((o - 2 * r) ^ _byteswap_uint64(orev - 2 * rrev));
        }
        else if constexpr (std::is_same_v<T, Bitboard>)
        {
            o &= board::multiDiagMask(idx);
            Bitboard orev = _byteswap_uint64(o);
            Bitboard rrev = _byteswap_uint64(idx);
            return board::multiDiagMask(idx) & ((o - 2 * idx) ^ _byteswap_uint64(orev - 2 * rrev));
        }
        else
        {
            return 0ULL;
        }
    }

    template<typename T>
    AttackMap hypqAntiDiag(Bitboard o, T idx)
    {
        if constexpr (std::is_same_v<T, board::square>)
        {
            o &= board::antiDiagMask(idx);
            Bitboard r = aux::setbit(idx);
            Bitboard orev = _byteswap_uint64(o);
            Bitboard rrev = _byteswap_uint64(r);
            return board::antiDiagMask(idx) & ((o - 2 * r) ^ _byteswap_uint64(orev - 2 * rrev));
        }
        else if constexpr (std::is_same_v<T, Bitboard>)
        {
            o &= board::multiAntiDiagMask(idx);
            Bitboard orev = _byteswap_uint64(o);
            Bitboard rrev = _byteswap_uint64(idx);
            return board::multiAntiDiagMask(idx) & ((o - 2 * idx) ^ _byteswap_uint64(orev - 2 * rrev));
        }
        else
        {
            return 0ULL;
        }
    }

    template<typename T>
    AttackMap hypqFile(Bitboard o, T idx)
    {
        if constexpr (std::is_same_v<T, board::square>)
        {
            o &= board::fileMask(idx);
            Bitboard r = aux::setbit(idx);
            Bitboard orev = _byteswap_uint64(o);
            Bitboard rrev = _byteswap_uint64(r);
            return board::fileMask(idx) & ((o - 2 * r) ^ _byteswap_uint64(orev - 2 * rrev));
        }
        else if constexpr (std::is_same_v<T, Bitboard>)
        {
            o &= board::multiFileMask(idx);
            Bitboard orev = _byteswap_uint64(o);
            Bitboard rrev = _byteswap_uint64(idx);
            return board::multiFileMask(idx) & ((o - 2 * idx) ^ _byteswap_uint64(orev - 2 * rrev));
        }
        else
        {
            return 0ULL;
        }
    }
    
    template<typename T>
    AttackMap hypqRank(Bitboard o, T idx)
    {
        // No bit reversal: map to file 0 and calculate file attacks
        // before converting back to rank attacks
        if constexpr (std::is_same_v<T, board::square>)
        {
            Bitboard vertical = _pext_u64(o, board::rankMask(idx));
            vertical = _pdep_u64(vertical, board::fileMask(board::a1));
            Bitboard attacks = hypqFile(vertical, static_cast<board::square>(8 * aux::file(idx)));
            attacks = _pext_u64(attacks, board::fileMask(board::a1));
            return _pdep_u64(attacks, board::rankMask(idx));
        }
        else if constexpr (std::is_same_v<T, Bitboard>)
        {
            Bitboard vertical = _pext_u64(o, board::multiRankMask(idx));
            vertical = _pdep_u64(vertical, board::fileMask(board::a1));
            Bitboard attacks = hypqFile(vertical, static_cast<board::square>(8 * aux::file(_tzcnt_u64(idx))));
            attacks = _pext_u64(attacks, board::fileMask(board::a1));
            return _pdep_u64(attacks, board::multiRankMask(idx));
        }
        else
        {
            return 0ULL;
        }
    }

    template<typename T>
    AttackMap hypqAllOrth(Bitboard o, T idx)
    {
        return hypqFile(o, idx) | hypqRank(o, idx);
    }

    template<typename T>
    AttackMap hypqAllDiag(Bitboard o, T idx)
    {
        return hypqDiag(o, idx) | hypqAntiDiag(o, idx);
    }

    template<typename T>
    constexpr AttackMap kingAttacks(T idx)
    {
        Bitboard king;
        if constexpr (std::is_same_v<T, board::square>)
        {
            king = aux::setbit(idx);
        }
        else if constexpr (std::is_same_v<T, Bitboard>)
        {
            king = idx;
        }
        else
        {
            return 0ULL;
        }

        Bitboard n = (king << 8);
        Bitboard s = (king >> 8);
        Bitboard w = (king >> 1) & ~board::fileMask(board::h1);
        Bitboard e = (king << 1) & ~board::fileMask(board::a1);

        Bitboard nw = (king << 7) & ~board::fileMask(board::h1);
        Bitboard ne = (king << 9) & ~board::fileMask(board::a1);
        Bitboard sw = (king >> 9) & ~board::fileMask(board::h1);
        Bitboard se = (king >> 7) & ~board::fileMask(board::a1);
        return n | s | e | w | nw | ne | sw | se;
    }

    template<typename T>
    constexpr AttackMap knightAttacks(T idx)
    {
        Bitboard knight;
        if constexpr (std::is_same_v<T, board::square>)
        {
            knight = aux::setbit(idx);
        }
        else if constexpr (std::is_same_v<T, Bitboard>)
        {
            knight = idx;
        }
        else
        {
            return 0ULL;
        }
        // n = north, s = south, etc.
        Bitboard nnw = (knight << 15) & ~board::fileMask(board::h1);
        Bitboard nne = (knight << 17) & ~board::fileMask(board::a1);
        Bitboard nww = (knight << 6) & ~(board::fileMask(board::g1) | board::fileMask(board::h1));
        Bitboard nee = (knight << 10) & ~(board::fileMask(board::b1) | board::fileMask(board::a1));

        Bitboard ssw = (knight >> 17) & ~board::fileMask(board::h1);
        Bitboard sse = (knight >> 15) & ~board::fileMask(board::a1);
        Bitboard sww = (knight >> 10) & ~(board::fileMask(board::g1) | board::fileMask(board::h1));
        Bitboard see = (knight >> 6) & ~(board::fileMask(board::b1) | board::fileMask(board::a1));
        return nnw | nne | nww | nee | ssw | sse | sww | see;
    }

    template<typename T>
    constexpr AttackMap pawnAttacksLeft(T pawns)
    {
        if constexpr (std::is_same_v<T, Bitboard>)
        {
            return (pawns << 7) & ~board::fileMask(board::h1);
        }
        else if constexpr(std::is_same_v<T, board::square>)
        {
            return pawnAttacksLeft(static_cast<Bitboard>(aux::setbit(pawns)));
        }
        else
        {
            return 0;
        }
    }

    template<typename T>
    constexpr AttackMap pawnAttacksRight(T pawns)
    {
        if constexpr (std::is_same_v<T, Bitboard>)
        {
            return (pawns << 9) & ~board::fileMask(board::a1);
        }
        else if constexpr (std::is_same_v<T, board::square>)
        {
            return pawnAttacksRight(static_cast<Bitboard>(aux::setbit(pawns)));
        }
        else
        {
            return 0;
        }
    }

    template<typename T>
    constexpr AttackMap enemyPawnAttacksLeft(T pawns)
    {
        if constexpr (std::is_same_v<T, Bitboard>)
        {
            return (pawns >> 9) & ~board::fileMask(board::h1);
        }
        else if constexpr (std::is_same_v<T, board::square>)
        {
            return enemyPawnAttacksLeft(static_cast<Bitboard>(aux::setbit(pawns)));
        }
        else
        {
            return 0;
        }
    }

    template<typename T>
    constexpr AttackMap enemyPawnAttacksRight(T pawns)
    {
        if constexpr (std::is_same_v<T, Bitboard>)
        {
            return (pawns >> 7) & ~board::fileMask(board::a1);
        }
        else if constexpr (std::is_same_v<T, board::square>)
        {
            return enemyPawnAttacksRight(static_cast<Bitboard>(aux::setbit(pawns)));
        }
        else
        {
            return 0;
        }
    }

    template<typename T>
    constexpr AttackMap pawnAttacks(T pawns)
    {
        return pawnAttacksLeft(pawns) | pawnAttacksRight(pawns);
    }

    template<typename T>
    constexpr AttackMap enemyPawnAttacks(T pawns)
    {
        return enemyPawnAttacksLeft(pawns) | enemyPawnAttacksRight(pawns);
    }

    constexpr Bitboard pawnMovesUp(Bitboard pawns)
    {
        return (pawns << 8);
    }

    constexpr Bitboard pawn2MovesUp(Bitboard pawns, Bitboard occ)
    {
        Bitboard empty = ~occ;
        pawns &= board::rankMask(board::a2);
        Bitboard upOnce = (pawns << 8) & empty;
        return (upOnce << 8) & empty;
    }
    // All squares to the North of pawns
    constexpr Bitboard pawnNorthSpan(BitboardOrSquare auto pawns)
    {
        auto pawnsBB = getBB(pawns);
        return KSNorth(0ULL,pawnsBB);
    }
    // All squares to the south of pawns
    constexpr Bitboard pawnSouthSpan(BitboardOrSquare auto pawns)
    {
        auto pawnsBB = getBB(pawns);
        return KSSouth(0ULL, pawnsBB);
    }

    constexpr Bitboard pawnNAttackSpan(BitboardOrSquare auto pawns)
    {
        auto pawnsBB = getBB(pawns);
        auto mypawnAttacks = pawnAttacks(pawnsBB);
        return pawnNorthSpan(mypawnAttacks) | mypawnAttacks;
    }

    constexpr Bitboard pawnSAttackSpan(BitboardOrSquare auto pawns)
    {
        auto pawnsBB = getBB(pawns);
        auto pawnAttacks = enemyPawnAttacks(pawnsBB);
        return pawnSouthSpan(pawnAttacks) | pawnAttacks;
    }

    constexpr std::pair<Bitboard, Bitboard> backwardPawns(Bitboard myPawns, Bitboard theirPawns)
    {
        const auto myStops = myPawns << 8;
        const auto theirStops = theirPawns >> 8;

        const auto myAttacks = pawnAttacks(myPawns);
        const auto theirAttacks = enemyPawnAttacks(theirPawns);

        const auto myAttackSpan = pawnNAttackSpan(myPawns);
        const auto theirAttackSpan = pawnSAttackSpan(theirPawns);
        
        auto possMyBackwards = (myStops & theirAttacks & ~myAttackSpan) >> 8;
        auto possTheirBackwards = (theirStops & myAttacks & ~theirAttackSpan) << 8;

        auto myBackwards = possMyBackwards & (board::rankMask(board::a2) | board::rankMask(board::a3));
        auto theirBackwards = possTheirBackwards & (board::rankMask(board::a6) | board::rankMask(board::a7));

        possMyBackwards -= myBackwards;
        possTheirBackwards -= theirBackwards;

        possMyBackwards &= (myStops & enemyPawnAttacks(theirPawns & ~theirBackwards)) >> 8;
        possTheirBackwards &= (theirStops & pawnAttacks(myPawns & ~myBackwards)) << 8;

        return { myBackwards | possMyBackwards, theirBackwards | possTheirBackwards};
    }

    template<typename T>
    constexpr Bitboard forwardPawnMoves(Bitboard occ, T pawns)
    {
        if constexpr (std::is_same_v<T, Bitboard>)
        {
            Bitboard moves = (pawns << 8) & ~occ;
            Bitboard moves2 = moves & board::rankMask(board::a3);
            moves2 = (moves2 << 8) & ~occ;
            return moves | moves2;
        }
        else if constexpr (std::is_same_v<T, board::square>)
        {
            return forwardPawnMoves(occ, aux::setbit(pawns));
        }
        else
        {
            return 0;
        }
    }

    Bitboard getVertPinnedPieces(Bitboard occ, Bitboard king, Bitboard orth);

    Bitboard getHorPinnedPieces(Bitboard occ, Bitboard king, Bitboard orth);

    Bitboard getDiagPinnedPieces(Bitboard occ, Bitboard king, Bitboard diag);

    Bitboard getAntiDiagPinnedPieces(Bitboard occ, Bitboard king, Bitboard diag);

    Bitboard getAllPinnedPieces(Bitboard occ, Bitboard king, Bitboard diag, Bitboard orth);

    AttackMap genEnemyAttacks(Bitboard occ, const board::QBB& b);

    Bitboard getBetweenChecks(const board::QBB& b, Bitboard checkers);

    Bitboard isInCheck(const board::QBB& b);

    bool moveGivesCheck(const board::QBB& b, Move);

    constexpr auto getTheirAttackers(const board::QBB& b, Bitboard occ, BitboardOrSquare auto squares)
    {
        const auto bb = getBB(squares);

        auto pawnAttackers = pawnAttacks(bb) & b.their(b.getPawns());
        auto knightAttackers = knightAttacks(bb) & b.their(b.getKnights());
        auto bishopAttackers = KSAllDiag(occ, bb);
        auto rookAttackers = KSAllOrth(occ, bb);
        auto queenAttackers = (bishopAttackers | rookAttackers) & b.their(b.getQueens());
        bishopAttackers &= b.their(b.getBishops());
        rookAttackers &= b.their(b.getRooks());
        auto kingAttackers = kingAttacks(bb) & b.their(b.getKings());

        return std::make_tuple(pawnAttackers, knightAttackers, bishopAttackers, 
            rookAttackers, queenAttackers, kingAttackers);
    }

    constexpr Bitboard getTheirAttackersBB(const board::QBB& b, Bitboard occ, BitboardOrSquare auto squares)
    {
        return std::apply([](auto... args) {return (... | args); }, getTheirAttackers(b, occ, squares));
    }

    constexpr auto getMyAttackers(const board::QBB& b, Bitboard occ, BitboardOrSquare auto squares)
    {
        const auto bb = getBB(squares);

        auto pawnAttackers = enemyPawnAttacks(bb) & b.my(b.getPawns());
        auto knightAttackers = knightAttacks(bb) & b.my(b.getKnights());
        auto bishopAttackers = KSAllDiag(occ, bb);
        auto rookAttackers = KSAllOrth(occ, bb);
        auto queenAttackers = (bishopAttackers | rookAttackers) & b.my(b.getQueens());
        bishopAttackers &= b.my(b.getBishops());
        rookAttackers &= b.my(b.getRooks());
        auto kingAttackers = kingAttacks(bb) & b.my(b.getKings());

        return std::make_tuple(pawnAttackers, knightAttackers, bishopAttackers,
            rookAttackers, queenAttackers, kingAttackers);
    }

    constexpr Bitboard getMyAttackersBB(const board::QBB& b, Bitboard occ, BitboardOrSquare auto squares)
    {
        return std::apply([](auto... args) {return (... | args); }, getMyAttackers(b, occ, squares));
    }

    constexpr auto getAllAttackers(const board::QBB& b, Bitboard occ, BitboardOrSquare auto squares)
    {
        return std::apply([](auto... args) {return (...|args); },
            std::tuple_cat(getTheirAttackers(b, occ, squares), getMyAttackers(b, occ, squares)));
    }

    constexpr Bitboard getSliderAttackers(Bitboard occ, board::square s, Bitboard diag, Bitboard orth)
    {
        orth &= KSAllOrth(occ, s);
        diag &= KSAllDiag(occ, s);
        return orth | diag;
    }

    template<typename Attacks, typename T, std::size_t N>
    void addMoves(Bitboard pieces, Movelist<T, N>& ml, Attacks dest)
    {
        unsigned long index;
        while (_BitScanForward64(&index, pieces))
        {
            AttackMap pieceAttacks = dest(static_cast<board::square>(index));
            pieces = _blsr_u64(pieces);
            Move m = index;
            while (_BitScanForward64(&index, pieceAttacks))
            {
                pieceAttacks = _blsr_u64(pieceAttacks);
                m |= index << constants::toMaskOffset;
                ml.push_back(m);
                m &= constants::fromMask;
            }
        }
    }

    template<bool promos, std::size_t offset, typename Attacks, typename T, std::size_t N>
    void addPawnMoves(Bitboard pawns, Movelist<T, N>& ml, Attacks dest)
    {
        AttackMap attacks = dest(pawns);
        unsigned long index;
        AttackMap not8 = attacks;
        if constexpr (promos)
        {
            not8 &= ~board::rankMask(board::a8);
        }
        while (_BitScanForward64(&index, not8))
        {
            Move m = index - offset;
            not8 = _blsr_u64(not8);
            m |= index << constants::toMaskOffset;
            ml.push_back(m);
        }
        if constexpr (promos)
        {
            AttackMap on8 = attacks & board::rankMask(board::a8);
            while (_BitScanForward64(&index, on8))
            {
                Move m = index - offset;
                on8 = _blsr_u64(not8);
                m |= index << constants::toMaskOffset;
                m |= constants::queenPromo << constants::moveTypeOffset;
                ml.push_back(m);
                m &= 07777U;
                m |= constants::knightPromo << constants::moveTypeOffset;
                ml.push_back(m);
                m &= 07777U;
                m |= constants::rookPromo << constants::moveTypeOffset;
                ml.push_back(m);
                m &= 07777U;
                m |= constants::bishopPromo << constants::moveTypeOffset;
                ml.push_back(m);
            }
        }
    }
#define addLeftPMoves moves::addPawnMoves<true, 7>
#define addRightPMoves moves::addPawnMoves<true, 9>
#define addUpPMoves moves::addPawnMoves<true, 8>
#define addPinUpPMove moves::addPawnMoves<false, 8>
#define addUp2PMoves moves::addPawnMoves<false, 16>

    template<typename T, std::size_t N>
    void addEPMoves(Movelist<T, N>& ml, Bitboard pawns, Bitboard enp)
    {
        AttackMap enpAttacksL = enemyPawnAttacksLeft(enp) & pawns;
        AttackMap enpAttacksR = enemyPawnAttacksRight(enp) & pawns;
        unsigned long index;
        if (_BitScanForward64(&index, enpAttacksL))
        {
            Move m = index;
            m |= (index + 9) << constants::toMaskOffset;
            m |= constants::enPCap << constants::moveTypeOffset;
            ml.push_back(m);
        }
        if (_BitScanForward64(&index, enpAttacksR))
        {
            Move m = index;
            m |= (index + 7) << constants::toMaskOffset;
            m |= constants::enPCap << constants::moveTypeOffset;
            ml.push_back(m);
        }
    }
    constexpr bool QSearch = true;
    constexpr bool Quiets = true;
    template<bool qSearch = false, bool quietsOnly = false, typename T, std::size_t N>
    void genMoves(const board::QBB& b, Movelist<T, N>& ml)
    {
        static_assert(!(qSearch && quietsOnly));
        Bitboard checkers = isInCheck(b);

        if (!checkers)
        {
            Bitboard occ = b.getOccupancy();
            Bitboard mine = b.side;
            Bitboard theirs = occ & ~mine;
            Bitboard myKing = b.my(b.getKings());
            Bitboard orth = b.my(b.getOrthSliders());
            Bitboard diag = b.my(b.getDiagSliders());
            Bitboard knights = b.my(b.getKnights());
            Bitboard pawns = b.my(b.getPawns());
            AttackMap enemyAttacks = genEnemyAttacks(occ, b);

            Bitboard horPinned = getHorPinnedPieces(occ, myKing, b.their(b.getOrthSliders())) & b.side;
            Bitboard vertPinned = getVertPinnedPieces(occ, myKing, b.their(b.getOrthSliders())) & b.side;
            Bitboard diagPinned = getDiagPinnedPieces(occ, myKing, b.their(b.getDiagSliders())) & b.side;
            Bitboard antiDiagPinned = getAntiDiagPinnedPieces(occ, myKing, b.their(b.getDiagSliders())) & b.side;
            Bitboard vertPinnedPawns = vertPinned & pawns;
            vertPinned -= vertPinnedPawns;
            Bitboard diagPinnedPawns = diagPinned & pawns;
            diagPinned -= diagPinnedPawns;
            Bitboard antiDiagPinnedPawns = antiDiagPinned & pawns;
            antiDiagPinned -= antiDiagPinnedPawns;

            pawns &= ~(horPinned | antiDiagPinnedPawns | diagPinnedPawns | vertPinnedPawns);
            knights &= ~(horPinned | vertPinned | diagPinned | antiDiagPinned);
            orth &= ~(diagPinned | antiDiagPinned);
            diag &= ~(horPinned | vertPinned);

            horPinned &= orth;
            orth -= horPinned;
            vertPinned &= orth;
            orth -= vertPinned;
            diagPinned &= diag;
            diag -= diagPinned;
            antiDiagPinned &= diag;
            diag -= antiDiagPinned;

            if constexpr (qSearch)
            {
                mine = ~theirs;
            }
            else if constexpr (quietsOnly)
            {
                mine |= theirs;
            }
            addMoves(horPinned, ml, [occ, mine](board::square idx) {
                return hypqRank(occ, idx) & ~mine; });

            addMoves(vertPinned, ml, [occ, mine](board::square idx) {
                return hypqFile(occ, idx) & ~mine; });

            addMoves(diagPinned, ml, [occ, mine](board::square idx) {
                return hypqDiag(occ, idx) & ~mine; });

            addMoves(antiDiagPinned, ml, [occ, mine](board::square idx) {
                return hypqAntiDiag(occ, idx) & ~mine; });

            if constexpr (!quietsOnly)
            {
                addLeftPMoves(diagPinnedPawns, ml, [theirs](Bitboard pawns)
                    {return pawnAttacksLeft(pawns) & theirs; });

                addRightPMoves(antiDiagPinnedPawns, ml, [theirs](Bitboard pawns) {
                    return pawnAttacksRight(pawns) & theirs; });

                Bitboard pinnedPawnEP = (enemyPawnAttacksLeft(b.getEp()) & antiDiagPinnedPawns) | (enemyPawnAttacksRight(b.getEp()) & diagPinnedPawns);
                addEPMoves(ml, pinnedPawnEP, b.getEp());
            }

            if constexpr (!qSearch)
            {
                addPinUpPMove(vertPinnedPawns, ml, [occ](Bitboard pawns) {
                    return pawnMovesUp(pawns) & ~occ; });

                addUp2PMoves(vertPinnedPawns, ml, [occ](Bitboard pawns) {
                    return pawn2MovesUp(pawns, occ); });
            }

            addMoves(knights, ml, [mine](board::square idx) {
                return knightAttacks(idx) & ~mine; });

            addMoves(orth, ml, [occ, mine](board::square idx) {
                return hypqAllOrth(occ, idx) & ~mine; });

            addMoves(diag, ml, [occ, mine](board::square idx) {
                return hypqAllDiag(occ, idx) & ~mine; });

            addMoves(myKing, ml, [mine, enemyAttacks](board::square idx) {
                return kingAttacks(idx) & ~mine & ~enemyAttacks; });
            
            if constexpr (!quietsOnly)
            {
                addLeftPMoves(pawns, ml, [theirs](Bitboard pawns) {
                    return pawnAttacksLeft(pawns) & theirs; });

                addRightPMoves(pawns, ml, [theirs](Bitboard pawns) {
                    return pawnAttacksRight(pawns) & theirs; });
            }

            if constexpr (!qSearch)
            {
                addUpPMoves(pawns, ml, [occ](Bitboard pawns) {
                    return pawnMovesUp(pawns) & ~occ; });

                addUp2PMoves(pawns, ml, [occ](Bitboard pawns) {
                    return pawn2MovesUp(pawns, occ); });
            }

            if constexpr (!quietsOnly)
            {
                Bitboard myKing5 = myKing & board::rankMask(board::a5);
                Bitboard theirOrth5 = b.their(b.getOrthSliders()) & board::rankMask(board::a5);
                Bitboard EPPin = getHorPinnedPieces(occ & ~(b.getEp() >> 8), myKing5, theirOrth5);
                addEPMoves(ml, pawns & ~EPPin, b.getEp());
            }
            if constexpr (!qSearch)
            {
                constexpr Bitboard betweenKSCastle = aux::setbit(board::f1) | aux::setbit(board::g1);
                if (b.canCastleShort() && !((enemyAttacks | occ) & betweenKSCastle))
                {
                    Move m = board::e1;
                    m |= board::g1 << constants::toMaskOffset;
                    m |= constants::KSCastle << constants::moveTypeOffset;
                    ml.push_back(m);
                }
                constexpr Bitboard betweenQSCastle = aux::setbit(board::d1) | aux::setbit(board::c1) | aux::setbit(board::b1);
                constexpr Bitboard QSCastleNoAttack = aux::setbit(board::d1) | aux::setbit(board::c1);
                if (b.canCastleLong() && !(enemyAttacks & QSCastleNoAttack) && !(occ & betweenQSCastle))
                {
                    Move m = board::e1;
                    m |= board::c1 << constants::toMaskOffset;
                    m |= constants::QSCastle << constants::moveTypeOffset;
                    ml.push_back(m);
                }
            }
        }
        else if (__popcnt64(checkers) == 1)
        {
            Bitboard myKing = b.my(b.getKings());
            Bitboard occ = b.getOccupancy();
            Bitboard enemyDiag = b.their(b.getDiagSliders());
            Bitboard enemyOrth = b.their(b.getOrthSliders());
            Bitboard pinned = getAllPinnedPieces(occ, myKing, enemyDiag, enemyOrth);

            // rare: checker can be captured by en passant
            Bitboard enpChecker = (checkers << 8) & b.getEp();
            
            checkers |= getBetweenChecks(b, checkers);
            
            Bitboard enemyAttacks = genEnemyAttacks(occ & ~myKing, b);

            Bitboard mine = b.side;
            Bitboard theirs = occ & ~mine;
            if constexpr (qSearch)
            {
                mine = ~theirs;
            }
            else if constexpr (quietsOnly)
            {
                mine |= theirs;
            }
            addMoves(myKing, ml, [enemyAttacks, mine](board::square idx) {
                return kingAttacks(idx) & ~enemyAttacks & ~mine; });

            addMoves(b.my(b.getKnights()) & ~pinned, ml, [checkers, mine](board::square idx) {
                return knightAttacks(idx) & checkers & ~mine; });

            addMoves(b.my(b.getDiagSliders()) & ~pinned, ml, [occ, checkers, mine](board::square idx) {
                return hypqAllDiag(occ, idx) & checkers & ~mine; });

            addMoves(b.my(b.getOrthSliders()) & ~pinned, ml, [occ, checkers, mine](board::square idx) {
                return hypqAllOrth(occ, idx) & checkers & ~mine; });

            if constexpr (!quietsOnly)
            {
                addLeftPMoves(b.my(b.getPawns()) & ~pinned, ml, [checkers, occ](Bitboard pawns) {
                    return pawnAttacksLeft(pawns) & checkers & occ; });

                addRightPMoves(b.my(b.getPawns()) & ~pinned, ml, [checkers, occ](Bitboard pawns) {
                    return pawnAttacksRight(pawns) & checkers & occ; });
            }

            if constexpr (!qSearch)
            {
                addUpPMoves(b.my(b.getPawns()) & ~pinned, ml, [checkers, occ](Bitboard pawns) {
                    return pawnMovesUp(pawns) & checkers & ~occ; });

                addUp2PMoves(b.my(b.getPawns()) & ~pinned, ml, [checkers, occ](Bitboard pawns) {
                    return pawn2MovesUp(pawns, occ) & checkers; });
            }
            if constexpr (!quietsOnly)
            {
                addEPMoves(ml, b.my(b.getPawns()) & ~pinned, enpChecker);
            }
        }
        else // >1 checkers (double check)
        {
            Bitboard myKing = b.my(b.getKings());
            Bitboard occ = b.getOccupancy();
            Bitboard mine = occ & b.side;
            occ &= ~myKing; // remove king to generate X rays through king

            AttackMap attacks = genEnemyAttacks(occ, b);
            if constexpr (qSearch)
            {
                Bitboard theirs = occ & ~mine;
                mine = ~theirs;
            }
            else if constexpr (quietsOnly)
            {
                Bitboard theirs = occ & ~mine;
                mine |= theirs;
            }
            addMoves(myKing, ml, [mine, attacks](board::square idx) {return kingAttacks(idx) & ~attacks & ~mine; });
        }
    }
    enum class Stage : unsigned {none, hash, captureStageGen, captureStage, killer1Stage, killer2Stage, quietsGen, quiets, losingCaptures };

    template<typename KillerTable, typename History>
    class MoveOrder
    {
    public:
        MoveOrder(KillerTable* _kt, History* _ht, const board::QBB& _b, std::uint64_t h, std::size_t depth)
            :kt(_kt), ht(_ht), b(_b), hash(h), d(depth){}
        bool next(const board::QBB& b, Move& m)
        {
            switch (stage)
            {
            case Stage::hash:
                hashmove = Tables::tt[hash].move;
                if (kt)
                {
                    k1move = kt->getKiller(d, 0);
                    k2move = kt->getKiller(d, 1);
                }
                if (Tables::tt[hash].key == hash && hashmove)
                {
                    if (isLegalMove(b, hashmove))
                    {
                        m = hashmove;
                        stage = Stage::captureStageGen;
                        stageReturned = Stage::hash;
                        return true;
                    }
                }
                stage = Stage::captureStageGen;
                [[fallthrough]];
            case Stage::captureStageGen:
                genMoves<QSearch>(b, ml);
                ml.remove_moves_if(ml.begin(), ml.end(), [this](auto sm) {return sm.m == hashmove; });
                for (auto& [move, score] : ml)
                {
                    score = eval::see(b, move);
                }
                captureBegin = ml.begin();
                captureEnd = ml.end();
                stage = ml.size() ? Stage::captureStage : Stage::killer1Stage;
                [[fallthrough]];
            case Stage::captureStage:
                if (captureBegin == captureEnd)
                {
                    stage = Stage::killer1Stage;
                }
                else
                {
                    auto bestCapture = std::max_element(captureBegin, captureEnd);
                    if (bestCapture->score < 0)
                    {
                        losingCapturesBegin = captureBegin;
                        stage = Stage::killer1Stage;
                    }
                    else
                    {
                        std::iter_swap(captureBegin, bestCapture);
                        m = captureBegin->m;
                        ++captureBegin;
                        stageReturned = Stage::captureStage;
                        return true;
                    }
                }
                [[fallthrough]];
            case Stage::killer1Stage:
                stage = Stage::killer2Stage;
                if (k1move != hashmove && isLegalMove(b, k1move))
                {
                    m = k1move;
                    stageReturned = Stage::killer1Stage;
                    return true;
                }
                [[fallthrough]];
            case Stage::killer2Stage:
                stage = Stage::quietsGen;
                if (k2move != hashmove && isLegalMove(b, k2move))
                {
                    m = k2move;
                    stageReturned = Stage::killer2Stage;
                    return true;
                }
                [[fallthrough]];
            case Stage::quietsGen:
                quietsCurrent = captureEnd;
                genMoves<!QSearch, Quiets>(b, ml);
                ml.remove_moves_if(quietsCurrent, ml.end(), [this](ScoredMove k) {return k.m == hashmove || k.m == k1move || k.m == k2move; });
                for (auto i = quietsCurrent; i != ml.end(); ++i)
                {
                    i->score = ht->getHistoryScore(b, i->m);
                }
                quietsEnd = ml.end();
                stage = Stage::quiets;
                [[fallthrough]];
            case Stage::quiets:
                if (quietsCurrent == quietsEnd)
                {
                    stage = Stage::losingCaptures;
                }
                else
                {
                    auto max = std::max_element(quietsCurrent, quietsEnd);
                    std::iter_swap(quietsCurrent, max);
                    m = quietsCurrent->m;
                    ++quietsCurrent;
                    stageReturned = Stage::quiets;
                    return true;
                }
                [[fallthrough]];
            case Stage::losingCaptures:
                if (losingCapturesBegin == captureEnd)
                {
                    return false;
                }
                else
                {
                    m = losingCapturesBegin->m;
                    ++losingCapturesBegin;
                    stageReturned = Stage::losingCaptures;
                    return true;
                }
            default:
                return false;
            }
        }
    private:
        MoveOrder() {}
        Movelist<ScoredMove, 218> ml;
        decltype(ml.begin()) captureBegin = ml.begin();
        decltype(ml.end()) captureEnd = ml.end();
        decltype(ml.begin()) quietsCurrent;
        decltype(ml.begin()) quietsEnd;
        decltype(ml.begin()) losingCapturesBegin = ml.begin();
        KillerTable* kt = nullptr;
        History* ht = nullptr;
        const board::QBB& b;
        std::uint64_t hash = 0;
        std::size_t d = 0;
        Stage stage = Stage::hash;
        Move hashmove = 0;
        Move k1move = 0;
        Move k2move = 0;
    public:
        Stage stageReturned = Stage::none;

    };

}

#endif