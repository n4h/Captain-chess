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

#ifndef AUXILIARY_H
#define AUXILIARY_H

#include <intrin.h>

#pragma intrinsic(_BitScanForward64)

#include <concepts>
#include <cstdint>
#include <string_view>
#include <type_traits>
#include <chrono>
#include <cstdlib>
#include <random>
#include <cassert>
#include <cmath>
#include <utility>

namespace aux
{
    extern std::mt19937_64 seed;

    using std::integral;

    template<typename T>
    concept UintOrSquare = std::unsigned_integral<T> || std::unsigned_integral<std::underlying_type_t<T>>;

    // rank = rows, file = columns. Both rank and column
    // go from 0 to 7 inclusive. This translates (rank, file)
    // to the right index on a bitboard
    constexpr std::unsigned_integral auto index(std::unsigned_integral auto rank, std::unsigned_integral auto file)
    {
        return rank * 8 + file;
    }

    // converts square to new square after vertically flipping board
    // (file stays same, but rank is flipped)
    constexpr std::unsigned_integral auto flip(UintOrSquare auto x)
    {
        return x ^ 56;
    }

    constexpr std::uint64_t setbit(UintOrSquare auto pos)
    {
        return 0b1ULL << pos;
    }
    
    constexpr std::uint64_t setbit(std::unsigned_integral auto rank, std::unsigned_integral auto file)
    {
        return setbit(index(rank, file));
    }

    constexpr std::unsigned_integral auto rank(UintOrSquare auto x)
    {
        return x / 8;
    }

    // file and rank satisfy index(rank(x),file(x)) = x;
    // so we can convert between the index of a 64 elem
    // array and the rank/file associated with that index
    constexpr std::unsigned_integral auto file(UintOrSquare auto x)
    {
        return x - 8 * rank(x);
    }

    // getDiag and getAntiDiag index into diagMask and antidiagMask
    constexpr std::unsigned_integral auto getDiag(UintOrSquare auto x)
    {
        return rank(x) + file(x);
    }

    constexpr std::unsigned_integral auto getAntiDiag(UintOrSquare auto x)
    {
        return 7U + rank(x) - file(x);
    }

    // takes and index, and returns a new index
    // after offsetting the old index by r ranks
    // and f files
    constexpr std::unsigned_integral auto index2index(UintOrSquare auto index, integral auto r, integral auto f)
    {
        return aux::index( rank(index) + r, file(index) + f);
    }
    // offset i by rank r and file f and check if it is still on the board
    constexpr bool isIndex(UintOrSquare auto i, integral auto r, integral auto f)
    {
        return !(rank(i) + r > 8 || rank(i) + r < 1 || file(i) + f > 8 || file(i) + f < 1);
    }

    constexpr std::uint64_t shiftRightNoWrap(std::uint64_t b)
    {
        return (b >> 1) & 9187201950435737471ULL; // No H file
    }

    constexpr std::uint64_t shiftLeftNoWrap(std::uint64_t b)
    {
        return (b << 1) & 18374403900871474942ULL; // No A file
    }

    // increment the file number by c. There are 8 squares per
    // row, so if file + c is greater than 8, just start counting from
    // 1 again (e.g. 6 + 2 --> 1)
    constexpr unsigned int incFile(int file, int c)
    {
        return (file + c) % 8;
    }

    // isNumber only checks if i is in the range '1'-'8'
    // use isMoveNumber for '1'-'9'
    constexpr bool isNumber(const char& i)
    {
        return (i == '1' || i == '2' || i == '3' || i == '4' || i == '5' || i == '6' || i == '7' || i == '8');
    }

    constexpr bool isMoveNumber(const char& i)
    {
        return isNumber(i) || i == '9' || i == '0';
    }

    constexpr bool isFile(const char& i)
    {
        return (i == 'a' || i == 'b' || i == 'c' || i == 'd' || i == 'e' || i == 'f' || i == 'g' || i == 'h');
    }

    constexpr bool isMove(std::string_view mv)
    {
        return mv.size() >= 4 && isFile(mv[0]) && isNumber(mv[1]) && isFile(mv[2]) && isNumber(mv[3]);
    }

    constexpr std::size_t fileNumber(const char& i)
    {
        switch (i)
        {
        case 'a':
            return 0U;
        case 'b':
            return 1U;
        case 'c':
            return 2U;
        case 'd':
            return 3U;
        case 'e':
            return 4U;
        case 'f':
            return 5U;
        case 'g':
            return 6U;
        case 'h':
            return 7U;
        default:
            assert(false);
            return 0U;
        }
    }

    constexpr char file2char(std::size_t file)
    {
        switch (file)
        {
        case 0:
            return 'a';
        case 1:
            return 'b';
        case 2:
            return 'c';
        case 3:
            return 'd';
        case 4:
            return 'e';
        case 5:
            return 'f';
        case 6:
            return 'g';
        case 7:
            return 'h';
        default:
            return ' ';
        }
    }

    constexpr bool isPiece(const char i)
    {
        switch (i)
        {
        case 'r':
        case 'R':
        case 'k':
        case 'K':
        case 'n':
        case 'N':
        case 'q':
        case 'Q':
        case 'b':
        case 'B':
        case 'p':
        case 'P':
            return true;
        default:
            return false;
        }
    }

    std::chrono::milliseconds castms(auto s)
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(s);
    }

    std::chrono::seconds castsec(auto s)
    {
        return std::chrono::duration_cast<std::chrono::seconds>(s);
    }

    constexpr unsigned l1dist(int x1, int y1, int x2, int y2)
    {
        return std::abs(x1 - x2) + std::abs(y1 - y2);
    }
    // TODO rewrite loops to use GetNextBit
    template<typename T>
    struct GetNextBit
    {
        std::uint64_t bits = 0;
        T next;
        GetNextBit(std::uint64_t b) :bits(b) {}
        bool operator()()
        {
            if constexpr (!std::is_same_v<T, std::uint64_t>)
            {
                unsigned long index = 0;
                bool gotbit = _BitScanForward64(&index, bits);
                if (gotbit)
                {
                    bits = _blsr_u64(bits);
                    next = T(index);
                }
                return gotbit;
            }
            else
            {
                next = _blsi_u64(bits);
                bits = _blsr_u64(bits);
                return next;
            }
        }
    };

    struct Xor64rng
    {
        std::uint64_t x = 88172645463325252ULL;
        constexpr std::uint64_t operator()()
        {
            x ^= x << 13;
            x ^= x >> 7;
            x ^= x << 17;
            return x;
        }
        constexpr Xor64rng(std::uint64_t seed) : x(seed) {}
    };

    inline double sigmoid(double K, double s)
    {
        double x = -K * s;
        x /= 400;
        x = std::pow(10, x);
        x += 1;
        return 1 / x;
    }

    template<typename T>
    constexpr void sort3(T& e1, T& e2, T& e3)
    {
        if (e1 > e3)
        {
            std::swap(e1, e3);
        }
        if (e2 > e3)
        {
            std::swap(e2, e3);
            return;
        }
        if (e1 > e2)
        {
            std::swap(e1, e2);
        }
    }
}
#endif
