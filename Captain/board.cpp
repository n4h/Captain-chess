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

#include <string>
#include <iostream>
#include <cctype>
#include <vector>
#include <tuple>
#include <cstdlib>
#include <cassert>

#include "board.hpp"
#include "auxiliary.hpp"
#include "constants.hpp"
#include "tables.hpp"

namespace board
{
    using namespace aux;
    using namespace constants;

    std::tuple<bool, unsigned int> makeSquare(const char i)
    {
        bool w = i >= 'A' && i <= 'Z';

        switch (i)
        {
        case 'K':
        case 'k':
            return std::make_tuple(w, king);
        case 'Q':
        case 'q':
            return std::make_tuple(w, queens);
        case 'R':
        case 'r':
            return std::make_tuple(w, rooks);
        case 'B':
        case 'b':
            return std::make_tuple(w, bishops);
        case 'N':
        case 'n':
            return std::make_tuple(w, knights);
        case 'P':
        case 'p':
            return std::make_tuple(w, pawns);
        default:
            assert(false);
            return std::make_tuple(w, 50);
        }
    }

    std::vector<std::string> splitString(std::string s, const char d)
    {
        std::vector<std::string> split = {};
        std::stringstream stream(s);
        std::string word = "";
        while (std::getline(stream, word, d))
        {
            split.push_back(word);
        }
        return split;
    }

    // see https://en.wikipedia.org/wiki/Forsyth%E2%80%93Edwards_Notation
    QBB::QBB(const std::string& fen, bool moveNumInfo)
    {
        unsigned int currFile = 0;
        
        auto splitfen = splitString(fen, ' ');
        auto splitboard = splitString(splitfen[0], '/');
        
        assert(splitfen[1] == "w" || splitfen[1] == "b");
        bool wToMove = splitfen[1] == "w";

        for (unsigned int i = 0; i != 8; ++i)
        {
            for (auto j : splitboard[i])
            {
                if (isPiece(j))
                {
                    auto [color, pieceType] = makeSquare(j);

                    switch (pieceType)
                    {
                    case pawns:
                        pbq |= setbit(7 - i, currFile);
                        break;
                    case knights:
                        nbk |= setbit(7 - i, currFile);
                        break;
                    case bishops:
                        pbq |= setbit(7 - i, currFile);
                        nbk |= setbit(7 - i, currFile);
                        break;
                    case rooks:
                        rqk |= setbit(7 - i, currFile);
                        break;
                    case queens:
                        pbq |= setbit(7 - i, currFile);
                        rqk |= setbit(7 - i, currFile);
                        break;
                    case king:
                        rqk |= setbit(7 - i, currFile);
                        nbk |= setbit(7 - i, currFile);
                        break;
                    }
                    if (wToMove == color)
                        side |= setbit(7 - i, currFile);

                    currFile = incFile(currFile, 1);
                }
                else if (isNumber(j))
                {
                    currFile = incFile(currFile, j - '0');
                }
            }
        }

        for (auto i : splitfen[2])
        {
            if (i == 'K') epc |= setbit(e1) | setbit(h1);
            else if (i == 'k') epc |= setbit(e8) | setbit(h8);
            else if (i == 'q') epc |= setbit(e8) | setbit(a8);
            else if (i == 'Q') epc |= setbit(e1) | setbit(a1);
            else assert(i == '-');
        }

        if (splitfen[3] != "-")
        {
            const std::size_t file = fileNumber(splitfen[3][0]);
            assert(splitfen[3][1] >= '1' && splitfen[3][1] <= '8');
            const std::size_t rank = splitfen[3][1] - '0' - 1;
            epc |= setbit(rank, file);
        }

        if (splitfen[1] == "w") epc |= 0x80'80'00'00'00U;
        if (!wToMove) flipQBB();

        if (moveNumInfo)
        {
            Bitboard halfMoves = static_cast<Bitboard>(std::stoi(splitfen[4])) << 24;
            halfMoves |= halfMoves << 8;
            epc += halfMoves;
        }
    }

    // adapted from https://www.chessprogramming.org/AVX2#VerticalNibble
    unsigned QBB::getPieceType(square s) const noexcept
    {
        __m256i qbb = _mm256_set_epi64x(rqk, nbk, pbq, side);
        __m128i  shift = _mm_cvtsi32_si128(s ^ 63U);
        qbb = _mm256_sll_epi64(qbb, shift);
        std::uint32_t qbbsigns = _mm256_movemask_epi8(qbb);
        return _pext_u32(qbbsigns, 0x80808080);
    }

    unsigned QBB::getPieceCode(square s) const noexcept
    {
        return getPieceType(s) >> 1;
    }

    unsigned QBB::getPieceCodeIdx(square s) const noexcept
    {
        return getPieceCode(s) - 1;
    }

    bool QBB::isMyPiece(square s) const noexcept
    {
        auto piecetype = getPieceType(s);
        return piecetype & 0b1U;
    }

    void QBB::flipQBB()
    {
        side = _byteswap_uint64(side);
        pbq = _byteswap_uint64(pbq);
        nbk = _byteswap_uint64(nbk);
        rqk = _byteswap_uint64(rqk);
        epc = _byteswap_uint64(epc);
    }

    unsigned QBB::getEnpFile() const noexcept
    {
        Bitboard enpbb = epc & rankMask(a6);
        return file(_tzcnt_u64(enpbb));
    }

    void QBB::makeMove(const Move m)
    {
        const auto fromSq = getMoveInfo<fromMask>(m);
        const auto toSq = getMoveInfo<toMask>(m);
        const auto fromBB = setbit(fromSq);
        const auto toBB = setbit(toSq);
        const auto fromPcType = getPieceType(static_cast<square>(fromSq)) >> 1;
        const auto toPcType = getPieceType(static_cast<square>(toSq)) >> 1;
        
        // 50 move rule counter and color flip
        constexpr Bitboard counter50 = 0x7f7f000000ULL;
        Bitboard add1 = 0x1'01'00'00'00ULL + (epc & counter50);
        add1 *= 1U >> toPcType;
        add1 *= 1U - (2U >> fromPcType);
        epc = add1 + (epc & ~counter50);
        epc ^= 0x80'80'00'00'00U;

        side &= ~fromBB;
        side |= toBB;
        
        pbq &= ~toBB;
        pbq |= toBB * ((fromBB & pbq) >> fromSq);
        pbq ^= (fromBB & pbq);

        nbk &= ~toBB;
        nbk |= toBB * ((fromBB & nbk) >> fromSq);
        nbk ^= (fromBB & nbk);

        rqk &= ~toBB;
        rqk |= toBB * ((fromBB & rqk) >> fromSq);
        rqk ^= (fromBB & rqk);

        epc &= ~(fromBB & (rankMask(a1) | rankMask(a8)));
        epc &= ~(toBB & (rankMask(a1) | rankMask(a8)));
        constexpr Bitboard rank6 = 0x00'00'FF'00'00'00'00'00U;
        epc &= ~rank6;

        constexpr Bitboard rank3 = 0x00'00'00'00'00'FF'00'00U;
        
        const Bitboard enPassant = (rank3 & (fromBB << 8) & (toBB >> 8)) << 8 * (fromPcType - 1);
        epc |= enPassant & rank3;

        const auto moveType = getMoveInfo<moveTypeMask>(m);

        constexpr std::uint32_t promoUpdateRules[8] = { 0, 0, 0, 0, 0x00'01'01'00U, 0x00'01'00'00U, 0x01'00'01'00U, 0x01'00'00'00U };
        const std::uint32_t promoUpdate = promoUpdateRules[moveType] << file(toSq);

        rqk ^= _bextr_u64(promoUpdate, 24U, 8U) << 56;
        nbk ^= _bextr_u64(promoUpdate, 16U, 8U) << 56;
        pbq ^= _bextr_u64(promoUpdate, 8U, 8U) << 56;

        constexpr std::uint64_t castleUpdate = 0x00'00'00'00'00'A0'09'00U;
        rqk ^= _bextr_u64(castleUpdate, moveType * 8, 8U);
        side ^= _bextr_u64(castleUpdate, moveType * 8, 8U);

        constexpr std::uint64_t enPUpdate = 0x00'00'00'00'01'00'00'00U;
        pbq ^= _bextr_u64(enPUpdate, moveType * 8, 8U) << (file(toSq)+32);

        side = ~side & (pbq | nbk | rqk);
        flipQBB();
    }
    void QBB::doNullMove()
    {
        side = ~side & (pbq | nbk | rqk);
        epc &= ~rankMask(a6);
        epc ^= 0x80'80'00'00'00U;
        flipQBB();
    }
    // b1 is white to move and b2 is black to move
    Bitboard getCastlingDiff(const board::QBB& b1, const board::QBB& b2)
    {
        Bitboard b1castling = b1.getCastling();
        Bitboard b16 = _pext_u64(b1castling, 0x9100000000000091ULL);
        Bitboard b2castling = _byteswap_uint64(b2.getCastling());
        Bitboard b26 = _pext_u64(b2castling, 0x9100000000000091ULL);
        
        Bitboard b16king = b16 & 18ULL;
        Bitboard b26king = b26 & 18ULL;

        b16 = (b16 & (b16king >> 1)) | (b16 & (b16king << 1));
        b26 = (b26 & (b26king >> 1)) | (b26 & (b26king << 1));
        
        Bitboard b14 = _pext_u64(b16, 45ULL);
        Bitboard b24 = _pext_u64(b26, 45ULL);
        
        return b14 ^ b24;
    }

    bool validPosition(const QBB& b)
    {
        Bitboard occ = b.getOccupancy();

        if ((occ | b.side) != occ)
            return false;

        aux::GetNextBit<board::square> nextPiece{ occ };
        while (nextPiece())
        {
            auto piecetype = b.getPieceType(nextPiece.next);
            if (piecetype == 1 || piecetype >= 14)
                return false;
        }

        if (_popcnt64(b.my(b.getKings())) != 1)
            return false;
        if (_popcnt64(b.their(b.getKings())) != 1)
            return false;

        if (b.canCastleShort() && (b.getPieceType(board::e1) != myKing || b.getPieceType(board::h1) != myRook))
            return false;
        if (b.canCastleLong() && (b.getPieceType(board::e1) != myKing || b.getPieceType(board::a1) != myRook))
            return false;
        if (b.oppCanCastleShort() && (b.getPieceType(board::e8) != myKing || b.getPieceType(board::h8) != myRook))
            return false;
        if (b.oppCanCastleLong() && (b.getPieceType(board::e8) != myKing || b.getPieceType(board::a8) != myRook))
            return false;

        auto ep = b.getEp();

        switch (_popcnt64(ep))
        {
        case 0:
            return true;
        case 1:
            ep >>= 8;
            return ep & b.their(b.getPawns()) ? true : false;
        default:
            return false;
        }

        return true;
    }

    bool operator==(const QBB& b1, const QBB& b2)
    {
        return b1.side == b2.side && b1.pbq == b2.pbq
            && b1.nbk == b2.nbk && b1.rqk == b2.rqk && b1.epc == b2.epc;
    }

    std::uint64_t Board::initialHash(const board::QBB& b)
    {
        std::uint64_t inithash = 0;

        for (std::size_t i = 0; i != 64; ++i)
        {
            auto piecetype = b.getPieceType(static_cast<board::square>(i));
            if (b.isWhiteToPlay())
            {
                if (piecetype)
                {
                    if (piecetype & 1)
                        inithash ^= Tables::tt.whitePSQT[(piecetype >> 1) - 1][i];
                    else
                        inithash ^= Tables::tt.blackPSQT[(piecetype >> 1) - 1][i];
                }
            }
            else
            {
                if (piecetype)
                {
                    if (piecetype & 1)
                        inithash ^= Tables::tt.blackPSQT[(piecetype >> 1) - 1][aux::flip(i)];
                    else
                        inithash ^= Tables::tt.whitePSQT[(piecetype >> 1) - 1][aux::flip(i)];
                }
            }
        }

        if (b.isWhiteToPlay())
            inithash ^= Tables::tt.wToMove;

        if (b.isWhiteToPlay())
        {
            inithash ^= b.canCastleLong() ? Tables::tt.castling_first[0] : 0;
            inithash ^= b.canCastleShort() ? Tables::tt.castling_first[1] : 0;
            inithash ^= b.oppCanCastleLong() ? Tables::tt.castling_first[2] : 0;
            inithash ^= b.oppCanCastleShort() ? Tables::tt.castling_first[3] : 0;
        }
        else
        {
            inithash ^= b.oppCanCastleLong() ? Tables::tt.castling_first[0] : 0;
            inithash ^= b.oppCanCastleShort() ? Tables::tt.castling_first[1] : 0;
            inithash ^= b.canCastleLong() ? Tables::tt.castling_first[2] : 0;
            inithash ^= b.canCastleShort() ? Tables::tt.castling_first[3] : 0;
        }

        if (b.enpExists())
            inithash ^= Tables::tt.enPassant[b.getEnpFile()];
        return inithash;
    }

    std::uint64_t Board::incrementalUpdate(Move m, const board::QBB& old, const board::QBB& newb)
    {
        std::uint64_t update = 0;
        update ^= Tables::tt.wToMove;

        const auto* myPSQT = &Tables::tt.whitePSQT;
        const auto* oppPSQT = &Tables::tt.blackPSQT;
        auto from = static_cast<board::square>(board::getMoveInfo<constants::fromMask>(m));
        auto to = static_cast<board::square>(board::getMoveInfo<constants::toMask>(m));
        auto fromPcType = (old.getPieceType(from) >> 1);
        auto toPcType = (old.getPieceType(to) >> 1);

        if (!old.isWhiteToPlay())
        {
            myPSQT = &Tables::tt.blackPSQT;
            oppPSQT = &Tables::tt.whitePSQT;
            from = static_cast<board::square>(aux::flip(from));
            to = static_cast<board::square>(aux::flip(to));
        }

        update ^= (*myPSQT)[fromPcType - 1][from];

        if (toPcType)
            update ^= (*oppPSQT)[toPcType - 1][to];

        std::size_t queenRook = old.isWhiteToPlay() ? board::a1 : board::a8;
        std::size_t kingRook = old.isWhiteToPlay() ? board::h1 : board::h8;

        switch (board::getMoveInfo<constants::moveTypeMask>(m))
        {
        case constants::QMove:
            update ^= (*myPSQT)[fromPcType - 1][to];
            break;
        case constants::QSCastle:
            update ^= (*myPSQT)[fromPcType - 1][to];
            update ^= (*myPSQT)[constants::rookCode - 1][queenRook];
            update ^= (*myPSQT)[constants::rookCode - 1][queenRook + 3];
            break;
        case constants::KSCastle:
            update ^= (*myPSQT)[fromPcType - 1][to];
            update ^= (*myPSQT)[constants::rookCode - 1][kingRook];
            update ^= (*myPSQT)[constants::rookCode - 1][kingRook - 2];
            break;
        case constants::enPCap:
            update ^= (*myPSQT)[fromPcType - 1][to];
            update ^= (*oppPSQT)[constants::pawnCode - 1][old.isWhiteToPlay() ? to - 8 : to + 8];
            break;
        case constants::knightPromo:
            update ^= (*myPSQT)[constants::knightCode - 1][to];
            break;
        case constants::bishopPromo:
            update ^= (*myPSQT)[constants::bishopCode - 1][to];
            break;
        case constants::rookPromo:
            update ^= (*myPSQT)[constants::rookCode - 1][to];
            break;
        case constants::queenPromo:
            update ^= (*myPSQT)[constants::queenCode - 1][to];
            break;
        }

        auto EPChange = _bextr_u64(old.getEp() ^ newb.getEp(), 40U, 8);
        unsigned long index = 0;
        while (_BitScanForward64(&index, EPChange))
        {
            EPChange = _blsr_u64(EPChange);
            update ^= Tables::tt.enPassant[index];
        }
        if (old.isWhiteToPlay())
            update ^= Tables::tt.castling[board::getCastlingDiff(old, newb)];
        else
            update ^= Tables::tt.castling[board::getCastlingDiff(newb, old)];

        return update;
    }

    std::uint64_t Board::nullUpdate(const board::QBB& b)
    {
        std::uint64_t update = 0;
        update ^= Tables::tt.wToMove;
        auto ep = b.getEp();
        unsigned long index;
        if (_BitScanForward64(&index, ep))
        {
            update ^= Tables::tt.enPassant[aux::file(index)];
        }
        return update;
    }
}