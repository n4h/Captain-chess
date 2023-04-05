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

#include <immintrin.h>
#include <intrin.h>

#pragma intrinsic(__popcnt64)
#pragma intrinsic(_BitScanForward64)

#include <cstddef>

#include "moves.hpp"
#include "board.hpp"
#include "auxiliary.hpp"

namespace moves
{
    bool isLegalMove(const board::QBB& b, board::Move m)
    {
        const auto moveType = board::getMoveInfo<constants::moveTypeMask>(m);
        if (moveType > constants::queenPromo)
            return false;

        board::square fromSq = board::getMoveFromSq(m);
        auto fromPcType = b.getPieceType(fromSq);

        if (!(fromPcType & 1))
        {
            return false;
        }
        const Bitboard occ = b.getOccupancy();
        auto toSq = board::getMoveToSq(m);
        switch (fromPcType >> 1)
        {
        case constants::pawnCode:
        {
            AttackMap pAttacks = pawnAttacks(fromSq) & b.their(occ);
            AttackMap pEpAttacks = pawnAttacks(fromSq) & b.getEp();
            Bitboard pMoves = forwardPawnMoves(occ, fromSq);

            if (!((pMoves | pEpAttacks | pAttacks) & aux::setbit(toSq)))
            {
                return false;
            }

            if (toSq >= 56)
            {
                if (moveType < constants::knightPromo)
                {
                    return false;
                }
            }
            else if (aux::setbit(toSq) & pEpAttacks)
            {
                if (moveType != constants::enPCap)
                {
                    return false;
                }
            }
            else if (moveType != constants::QMove)
            {
                return false;
            }

            break;
        }
        case constants::knightCode:
        {
            AttackMap nAttacks = knightAttacks(fromSq) & ~b.my(occ);
            if (!(nAttacks & aux::setbit(toSq)))
            {
                return false;
            }
            if (moveType != constants::QMove)
            {
                return false;
            }
            break;
        }
        case constants::bishopCode:
        {
            AttackMap bAttacks = hypqAllDiag(occ, fromSq) & ~b.my(occ);
            if (!(bAttacks & aux::setbit(toSq)))
            {
                return false;
            }
            if (moveType != constants::QMove)
            {
                return false;
            }
            break;
        }
        case constants::rookCode:
        {
            AttackMap rAttacks = hypqAllOrth(occ, fromSq) & ~b.my(occ);
            if (!(rAttacks & aux::setbit(toSq)))
            {
                return false;
            }
            if (moveType != constants::QMove)
            {
                return false;
            }
            break;
        }
        case constants::queenCode:
        {
            AttackMap qAttacks = (hypqAllOrth(occ, fromSq) | hypqAllDiag(occ, fromSq)) & ~b.my(occ);
            if (!(qAttacks & aux::setbit(toSq)))
            {
                return false;
            }
            if (moveType != constants::QMove)
            {
                return false;
            }
            break;
        }
        case constants::kingCode:
        {
            if (moveType == constants::KSCastle)
            {
                if (fromSq != board::e1 || toSq != board::g1)
                    return false;
                if (!b.canCastleShort())
                    return false;
                if (occ & (aux::setbit(board::f1) | aux::setbit(board::g1)))
                    return false;
                if (getTheirAttackersBB(b, occ, board::e1) || getTheirAttackersBB(b, occ, board::f1) || getTheirAttackersBB(b, occ, board::g1))
                    return false;
                return true;
            }
            else if (moveType == constants::QSCastle)
            {
                if (fromSq != board::e1 || toSq != board::c1)
                    return false;
                if (!b.canCastleLong())
                    return false;
                if (occ & (aux::setbit(board::d1) | aux::setbit(board::c1) | aux::setbit(board::b1)))
                    return false;
                if (getTheirAttackersBB(b, occ, board::e1) || getTheirAttackersBB(b, occ, board::d1) || getTheirAttackersBB(b, occ, board::c1))
                    return false;
                return true;
            }
            else if (moveType == constants::QMove)
            {
                AttackMap kAttacks = kingAttacks(fromSq) & ~b.my(occ);
                if (!(kAttacks & aux::setbit(toSq)))
                    return false;
                auto attackers = kingAttacks(toSq) & b.their(b.getKings());
                attackers |= knightAttacks(toSq) & b.their(b.getKnights());
                attackers |= hypqAllDiag(occ & ~aux::setbit(fromSq), toSq) & b.their(b.getDiagSliders());
                attackers |= hypqAllOrth(occ & ~aux::setbit(fromSq), toSq) & b.their(b.getOrthSliders());
                attackers |= pawnAttacks(toSq) & b.their(b.getPawns());
                if (attackers)
                {
                    return false;
                }
                return true;
            }
            else
            {
                return false;
            }
            break;
        }
        default:
            return false;
        }
        Bitboard newOcc = (occ | aux::setbit(toSq)) & ~aux::setbit(fromSq);
        if (moveType == constants::enPCap)
            newOcc &= ~(aux::setbit(toSq) >> 8);
        Bitboard myKing = b.my(b.getKings());
        auto attackers = kingAttacks(myKing) & b.their(b.getKings());
        attackers |= knightAttacks(myKing) & b.their(b.getKnights());
        attackers |= hypqAllDiag(newOcc, myKing) & b.their(b.getDiagSliders());
        attackers |= hypqAllOrth(newOcc, myKing) & b.their(b.getOrthSliders());
        attackers |= pawnAttacks(myKing) & b.their(b.getPawns());
        attackers &= ~aux::setbit(toSq);
        
        return !attackers;
    }

    Bitboard getVertPinnedPieces(Bitboard occ, Bitboard king, Bitboard orth)
    {
        return KSFile(occ, king) & KSFile(occ, orth) & occ;
    }
    Bitboard getHorPinnedPieces(Bitboard occ, Bitboard king, Bitboard orth)
    {
        return KSRank(occ, king) & KSRank(occ, orth) & occ;
    }
    Bitboard getDiagPinnedPieces(Bitboard occ, Bitboard king, Bitboard diag)
    {
        return KSDiag(occ, king) & KSDiag(occ, diag) & occ;
    }
    Bitboard getAntiDiagPinnedPieces(Bitboard occ, Bitboard king, Bitboard diag)
    {
        return KSAntiDiag(occ, king) & KSAntiDiag(occ, diag) & occ;
    }
    Bitboard getAllPinnedPieces(Bitboard occ, Bitboard king, Bitboard diag, Bitboard orth)
    {
        Bitboard pinned = 0;
        AttackMap diagAttacks = KSDiag(occ, diag);
        AttackMap antiDiagAttacks = KSAntiDiag(occ, diag);
        AttackMap fileAttacks = KSFile(occ, orth);
        AttackMap rankAttacks = KSRank(occ, orth);
        pinned |= KSDiag(occ, king) & diagAttacks & occ;
        pinned |= KSAntiDiag(occ, king) & antiDiagAttacks & occ;
        pinned |= KSFile(occ, king) & fileAttacks & occ;
        pinned |= KSRank(occ, king) & rankAttacks & occ;
        return pinned;
    }
    AttackMap genEnemyAttacks(Bitboard occ, const board::QBB& b)
    {
        Bitboard attacks = KSAllDiag(occ, b.their(b.getDiagSliders()));
        attacks |= KSAllOrth(occ, b.their(b.getOrthSliders()));
        attacks |= enemyPawnAttacksLeft(b.their(b.getPawns()));
        attacks |= enemyPawnAttacksRight(b.their(b.getPawns()));
        attacks |= knightAttacks(b.their(b.getKnights()));
        attacks |= kingAttacks(b.their(b.getKings()));
        return attacks;
    }
    Bitboard getBetweenChecks(const board::QBB& b, Bitboard checkers)
    {
        const Bitboard myKing = b.my(b.getKings());
        Bitboard occ = b.getOccupancy();

        checkers &= b.getDiagSliders() | b.getOrthSliders();
        Bitboard between = KSDiag(occ, myKing) & KSDiag(occ, checkers);
        between |= KSAntiDiag(occ, myKing) & KSAntiDiag(occ, checkers);
        between |= KSFile(occ, myKing) & KSFile(occ, checkers);
        between |= KSRank(occ, myKing) & KSRank(occ, checkers);

        return between;
    }
    Bitboard isInCheck(const board::QBB& b)
    {
        Bitboard myKing = b.my(b.getKings());
        Bitboard occ = b.getOccupancy();

        Bitboard checkers = b.their(b.getDiagSliders()) & hypqAllDiag(occ, myKing);
        checkers |= b.their(b.getOrthSliders()) & hypqAllOrth(occ, myKing);
        checkers |= b.their(b.getKnights()) & knightAttacks(myKing);
        checkers |= b.their(b.getPawns()) & (pawnAttacksLeft(myKing) | pawnAttacksRight(myKing));

        return checkers;
    }
    bool moveGivesCheck(const board::QBB& b, board::Move m)
    { // TODO detect more kinds of checks
        auto oppKing = b.their(b.getKings());
        auto occ = b.getOccupancy();
        const auto fromSq = board::getMoveFromSq(m);
        const auto movetype = board::getMoveInfo<constants::moveTypeMask>(m);
        switch (b.getPieceCode(fromSq))
        {
        case constants::pawnCode:
        {
            if (board::isPromo(m))
            {
                occ ^= aux::setbit(fromSq);
                occ |= aux::setbit(board::getMoveToSq(m));
                auto [diag, orth] = std::make_pair(b.my(b.getDiagSliders()), b.my(b.getOrthSliders()));

                
                if (movetype == constants::queenPromo || movetype == constants::bishopPromo)
                    diag |= aux::setbit(board::getMoveToSq(m));
                if (movetype == constants::queenPromo || movetype == constants::rookPromo)
                    orth |= aux::setbit(board::getMoveToSq(m));

                auto orthAttacks = KSAllOrth(occ, orth);
                auto diagAttacks = KSAllDiag(occ, diag);

                if (movetype == constants::knightPromo)
                {
                    auto promotedPawnAttack = knightAttacks(aux::setbit(board::getMoveToSq(m)));
                    return (orthAttacks | diagAttacks | promotedPawnAttack) & oppKing;
                }
                else
                {
                    auto orthAttacks = KSAllOrth(occ, orth);
                    auto diagAttacks = KSAllDiag(occ, diag);
                    return (orthAttacks | diagAttacks) & oppKing;
                }
            }
            else
            {
                occ ^= aux::setbit(fromSq);
                occ |= aux::setbit(board::getMoveToSq(m));
                occ ^= movetype == constants::enPCap ? aux::setbit(board::getMoveToSq(m)) >> 8 : 0ULL;
                auto [diag, orth] = std::make_pair(b.my(b.getDiagSliders()), b.my(b.getOrthSliders()));

                auto orthAttacks = KSAllOrth(occ, orth);
                auto diagAttacks = KSAllDiag(occ, diag);
                auto movedPawnAttack = pawnAttacks(board::getMoveToSq(m));

                return (orthAttacks | diagAttacks | movedPawnAttack) & oppKing;
            }
        }
        case constants::knightCode:
        {
            occ ^= aux::setbit(fromSq);
            occ |= aux::setbit(board::getMoveToSq(m));
            auto [diag, orth] = std::make_pair(b.my(b.getDiagSliders()), b.my(b.getOrthSliders()));

            auto orthAttacks = KSAllOrth(occ, orth);
            auto diagAttacks = KSAllDiag(occ, diag);
            auto movedKnightAttack = knightAttacks(board::getMoveToSq(m));

            return (orthAttacks | diagAttacks | movedKnightAttack) & oppKing;
        }
        case constants::bishopCode:
        {
            occ ^= aux::setbit(fromSq);
            occ |= aux::setbit(board::getMoveToSq(m));
            auto [diag, orth] = std::make_pair(b.my(b.getDiagSliders()), b.my(b.getOrthSliders()));
            diag ^= aux::setbit(fromSq);
            diag |= aux::setbit(board::getMoveToSq(m));

            auto orthAttacks = KSAllOrth(occ, orth);
            auto diagAttacks = KSAllDiag(occ, diag);

            return (orthAttacks | diagAttacks) & oppKing;
        }
        case constants::rookCode:
        {
            occ ^= aux::setbit(fromSq);
            occ |= aux::setbit(board::getMoveToSq(m));
            auto [diag, orth] = std::make_pair(b.my(b.getDiagSliders()), b.my(b.getOrthSliders()));
            orth ^= aux::setbit(fromSq);
            orth |= aux::setbit(board::getMoveToSq(m));

            auto orthAttacks = KSAllOrth(occ, orth);
            auto diagAttacks = KSAllDiag(occ, diag);

            return (orthAttacks | diagAttacks) & oppKing;
        }
        case constants::queenCode:
        {
            occ ^= aux::setbit(fromSq);
            occ |= aux::setbit(board::getMoveToSq(m));
            auto [diag, orth] = std::make_pair(b.my(b.getDiagSliders()), b.my(b.getOrthSliders()));
            diag ^= aux::setbit(fromSq);
            diag |= aux::setbit(board::getMoveToSq(m));
            orth ^= aux::setbit(fromSq);
            orth |= aux::setbit(board::getMoveToSq(m));

            auto orthAttacks = KSAllOrth(occ, orth);
            auto diagAttacks = KSAllDiag(occ, diag);

            return (orthAttacks | diagAttacks) & oppKing;
        }
        case constants::kingCode:
        {
            occ ^= aux::setbit(fromSq);
            occ |= aux::setbit(board::getMoveToSq(m));
            auto [diag, orth] = std::make_pair(b.my(b.getDiagSliders()), b.my(b.getOrthSliders()));
            if (movetype == constants::KSCastle)
            {
                orth ^= aux::setbit(board::h1) | aux::setbit(board::f1);
                occ ^= aux::setbit(board::h1) | aux::setbit(board::f1);
            }
            else if (movetype == constants::QSCastle)
            {
                orth ^= aux::setbit(board::a1) | aux::setbit(board::d1);
                occ ^= aux::setbit(board::a1) | aux::setbit(board::d1);
            }


            auto orthAttacks = KSAllOrth(occ, orth);
            auto diagAttacks = KSAllDiag(occ, diag);

            return (orthAttacks | diagAttacks) & oppKing;
        }
        }
        return false;
    }

}
