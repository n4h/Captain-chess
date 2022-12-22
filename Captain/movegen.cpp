/*
Copyright 2022, Narbeh Mouradian

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

#include <immintrin.h>
#include <intrin.h>

#pragma intrinsic(__popcnt64)
#pragma intrinsic(_BitScanForward64)
#pragma intrinsic(_pdep_u64)
#pragma intrinsic(_pext_u64)

#include <cstddef>

#include "movegen.hpp"
#include "board.hpp"
#include "auxiliary.hpp"

namespace movegen
{


    AttackMap hypqRank(Bitboard o, board::square idx)
    {
        // No bit reversal: map to file 0 and calculate file attacks
        // before converting back to rank attacks
        Bitboard vertical = _pext_u64(o, board::rankMask(idx));
        vertical = _pdep_u64(vertical, board::fileMask(board::a1));
        Bitboard attacks = hypqFile(vertical, static_cast<board::square>(8 * aux::file(idx)));
        attacks = _pext_u64(attacks, board::fileMask(board::a1));
        return _pdep_u64(attacks, board::rankMask(idx));
    }

    AttackMap hypqRankW(Bitboard o, board::square idx)
    {
        Bitboard vertical = _pext_u64(o, board::rankMask(idx));
        vertical = _pdep_u64(vertical, board::fileMask(board::a1));
        Bitboard attacks = hypqFileS(vertical, static_cast<board::square>(8 * aux::file(idx)));
        attacks = _pext_u64(attacks, board::fileMask(board::a1));
        return _pdep_u64(attacks, board::rankMask(idx));
    }
}