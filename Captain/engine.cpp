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

#pragma intrinsic(_BitScanForward64)

#include <algorithm>
#include <iostream>
#include <chrono>
#include <string>
#include <sstream>
#include <cassert>
#include <ranges>

#include "engine.hpp"
#include "board.hpp"
#include "moves.hpp"
#include "eval.hpp"
#include "auxiliary.hpp"
#include "uci.hpp"
#include "moveorder.hpp"

namespace engine
{

    double Engine::getEval()
    {
        return eval;
    }

    std::string Engine::getPVuciformat(board::QBB b)
    {
        std::ostringstream PVString;
        for (auto& i : MainPV)
        {
            PVString << move2uciFormat(b, i) << " ";
            b.makeMove(i);
        }
        std::string s = PVString.str();
        if (!s.empty() && s.back() == ' ')
        {
            s.pop_back();
        }
        return s;
    }

    void Engine::printPV(const board::QBB& b)
    {
        if (!settings.quiet)
        {
            engine_out << "info depth " << currIDdepth << " "
                << "score cp " << eval << " "
                << "time " << elapsed().count() << " "
                << "nodes " << nodes << " "
                << "nps " << nodes / std::max(aux::castsec(elapsed()).count(), 1LL) << " "
                << "pv " << getPVuciformat(b) << std::endl;
            engine_out.emit();
        }
    }

    std::string Engine::line2string(const std::vector<Move>& moves)
    {
        std::size_t k = initialPos - 1;
        std::string s = move2uciFormat(b.boards[k++], moves[0]);
        for (std::size_t i = 1; i != moves.size(); ++i)
        {
            s.append(" ").append(move2uciFormat(b.boards[k++], moves[i]));
        }
        return s;
    }

    std::string Engine::getCurrline()
    {
        std::vector<Move> moves{ b.moves.begin() + initialMove, b.moves.end() };
        return line2string(moves);
    }

    std::size_t Engine::ply() const
    {
        return b.boards.size() - initialPos;
    }

    std::string Engine::move2uciFormat(const board::QBB& b, Move m)
    {
        if (m == 0)
            return "0000";
        std::ostringstream oss;
        auto from = board::getMoveInfo<constants::fromMask>(m);
        auto to = board::getMoveInfo<constants::toMask>(m);
        auto fromfile = aux::file(from);
        auto fromrank = b.getColorToPlay() == board::Color::White ? aux::rank(from) + 1 : 7 - aux::rank(from) + 1;
        auto tofile = aux::file(to);
        auto torank = b.getColorToPlay() == board::Color::White ? aux::rank(to) + 1 : 7 - aux::rank(to) + 1;

        oss << aux::file2char(fromfile);
        oss << fromrank;
        oss << aux::file2char(tofile);
        oss << torank;
        if (board::getPromoPiece(m) != board::none)
        {
            oss << board::promoFlag2char(m);
        }
        return oss.str();
    }

    std::chrono::milliseconds Engine::elapsed() const
    {
        return aux::castms(std::chrono::steady_clock::now() - searchStart);
    }

    bool Engine::threeFoldRep() const
    {
        std::size_t cnt = 0;
        std::uint64_t currHash = b.hashes.back();
        for (int i = b.hashes.size() - 1; i >= 0; i -= 2)
        {
            if (b.hashes[i] == currHash)
                ++cnt;
        }
        return cnt >= 3;
    }

    bool Engine::insufficientMaterial(const board::QBB& b) const
    {
        switch (_popcnt64(b.getOccupancy()))
        {
        case 0:
        case 1:
        case 2:
            // Only kings
            return true;
        case 3:
            // Two of the pieces are kings, so presence of minor piece
            // indicates that the third piece is a minor piece
            return b.getKnights() | b.getBishops();
        case 4:
        {
            const auto myBishops = b.my(b.getBishops());
            const auto theirBishops = b.their(b.getBishops());
            const auto bishops = b.getBishops();
            if (myBishops && theirBishops)
            {
                if (!(constants::whiteSquares & bishops) != !(constants::blackSquares & bishops))
                {
                    return true;
                }
                else
                {
                    return false;
                }
            }
            else
            {
                return false;
            }
        }
        default:
            return false;
        }
    }

    bool Engine::isPVNode(Eval alpha, Eval beta)
    {
        return alpha + 1 != beta;
    }

    int Engine::LMR(std::size_t i, const board::QBB& before, Move m, const board::QBB& after, int currDepth, bool PV, bool isKiller)
    {
        if (moves::isInCheck(before)
            || moves::isInCheck(after)
            || currDepth < 3
            || PV
            || isKiller
            || before.isCapture(m)
            || board::isPromo(m)
            || i < 4)
        {
            return 0;
        }
        else
        {
            return ply() >= 6 ? 2 : 1; 
        }
    }

    void Engine::uciUpdate()
    {
        if (!settings.quiet)
        {
            if (aux::castsec(std::chrono::steady_clock::now() - lastUpdate).count() >= 2)
            {
                lastUpdate = std::chrono::steady_clock::now();
                auto seconds = aux::castsec(elapsed()).count();
                if (seconds > 0)
                {
                    engine_out << "info depth " << currIDdepth << " nodes " << nodes << " nps " << nodes / seconds << std::endl;
                    engine_out.emit();
                }
            }
        }
    }

    bool Engine::shouldStop() noexcept
    {
        if (settings.ponder)
            return false;

        bool overtime = !settings.infiniteSearch && (elapsed() > moveTime || elapsed() > settings.maxTime);

        return overtime || nodes > settings.maxNodes || currIDdepth > settings.maxDepth;
    }
    
    void Engine::newGame()
    {
        killers = Tables::KillerTable();
        historyHeuristic = Tables::HistoryTable();
    }

    void Engine::rootSearch(board::Board _b, std::chrono::time_point<std::chrono::steady_clock> s)
    {
        searchStart = s;
        lastUpdate = s;
        b = _b;
        initialMove = b.moves.size();
        initialPos = b.boards.size();
        engineW = b.boards.back().isWhiteToPlay();
        currIDdepth = 0;
        nodes = 0;
        auto mytime = engineW ? settings.wmsec : settings.bmsec;
        [[maybe_unused]] auto myinc = engineW ? settings.winc : settings.binc;
        auto moveNumber = (initialPos + 2) / 2;
        if (settings.movestogo == std::numeric_limits<std::size_t>::max() || settings.movestogo == 0)
        {
            moveTime = aux::castms(moveNumber < 12 ? mytime / 40 : 0.05 * mytime);
        }
        else
        {
            moveTime = aux::castms((0.95*mytime) / settings.movestogo);
        }
        rootMoves.clear();
        moves::genMoves(b, rootMoves);
        
        for (auto& i : rootMoves)
        {
            i.score = rootMinBound;
        }
        Eval worstCase = rootMinBound;
        this->eval = rootMinBound;
        for (unsigned int k = 0; k <= 128; ++k)
        {
            currIDdepth = k;
            worstCase = rootMinBound;
            PrincipalVariation pv;
            std::size_t i = 0;
            
            for (auto& [move, score] : rootMoves)
            {
                if (!SearchFlags::searching.test())
                    goto endsearch;
                b.makeMove(move);
                try 
                {
                    if (i == 0)
                    {
                        score = -alphaBetaSearch(pv, rootMinBound, -worstCase, k - 1, false);
                    }
                    else
                    {
                        auto tmp = -alphaBetaSearch(pv, -worstCase - 1, -worstCase, k - 1, false);
                        if (tmp > worstCase)
                        {
                            tmp = -alphaBetaSearch(pv, rootMinBound, -worstCase, k - 1, false);
                        }
                        score = tmp;
                    }
                }
                catch (const Timeout&)
                {
                    goto endsearch;
                }
                if (score > worstCase)
                {
                    MainPV.clear();
                    MainPV.splice_after(MainPV.before_begin(), pv);
                    MainPV.push_front(move);
                    worstCase = score;
                }
                b.unmakeMove(move);
                ++i;
            }

            std::stable_sort(rootMoves.begin(), rootMoves.end(), [](const auto& a, const auto& b) {
                return a > b;
                });
            eval = rootMoves[0].score;
            printPV(b);
        }
    endsearch:
        SearchFlags::searching.clear();
        /*
        engine_out << "info string capturePct " << tt->capturePct(b) << std::endl;
        engine_out << "info string usedPct " << tt->usedPct() << std::endl;
        engine_out << "info string PVNode " << tt->nodeTypePct(Tables::PV) << std::endl;
        engine_out << "info string CNode " << tt->nodeTypePct(Tables::CUT) << std::endl;
        engine_out << "info string ANode " << tt->nodeTypePct(Tables::ALL) << std::endl;
        */
        eval = rootMoves[0].score;
        if (!settings.quiet)
        {
            engine_out << "bestmove " << move2uciFormat(b.boards[initialPos - 1], rootMoves[0].m) << std::endl;
        }
        engine_out.emit();
    }

    Eval Engine::quiesceSearch(Eval alpha, Eval beta, int depth)
    {
        if (insufficientMaterial(b) || threeFoldRep() || b.boards.back().get50() == 50)
            return 0;
        if (shouldStop())
            SearchFlags::searching.clear();
        ++nodes;

        if (Tables::tt[b.hashes.back()].key == b.hashes.back() && Tables::tt[b.hashes.back()].depth >= depth)
        {
            auto nodetype = Tables::tt[b.hashes.back()].nodeType;
            auto eval = Tables::tt[b.hashes.back()].eval;
            if (nodetype == Tables::PV)
                return eval;
            else if (nodetype == Tables::ALL && eval < alpha)
                return eval;
            else if (nodetype == Tables::CUT && eval > beta)
                return eval;
        }


        moves::Movelist<moves::ScoredMove> ml;
        moves::genMoves<moves::QSearch>(b, ml);
        auto captureIterations = ml.size();
        bool check = moves::isInCheck(b);
        Eval standpat = negInf;

        if (!check)
        {
            if (ml.size())
            {
                standpat = evaluate(b);
                if (standpat >= beta)
                {
                    return standpat;
                }
                else if (standpat >= alpha)
                {
                    alpha = standpat;
                }
            }
            else
            {
                moves::genMoves<!moves::QSearch, moves::Quiets>(b, ml);
                if (ml.size())
                {
                    return evaluate(b);
                }
                else
                {
                    return 0;
                }
            }
        }
        else
        {
            if (!ml.size())
            {
                moves::genMoves<!moves::QSearch, moves::Quiets>(b, ml);
                if (!ml.size())
                {
                    return negInf;
                }
            }
        }


        Eval currEval = standpat;

        for (std::size_t i = 0; auto& [move, score] : ml)
        {
            if (i < captureIterations)
                score = eval::see(b, move);
            ++i;
        }

        for (std::size_t i = 0; i != ml.size(); ++i)
        {
            if (i + 1 < captureIterations)
            {
                std::iter_swap(ml.begin() + i, std::max_element(ml.begin() + i, ml.end()));
            }
            if (!check && i < captureIterations)
            {
                if (ml[i].score < 0 || eval::getCaptureValue(b, ml[i].m) + 200 + standpat <= alpha)
                {
                    continue;
                }
            }
            if (!SearchFlags::searching.test())
                throw Timeout();
            b.makeMove(ml[i].m);
            
            currEval = std::max<Eval>(currEval, -quiesceSearch(-beta, -alpha, depth - 1));
            b.unmakeMove(ml[i].m);
            alpha = std::max(currEval, alpha);
            if (alpha >= beta)
            {
                return currEval;
            }
            if (check && i + 1 == captureIterations)
            {
                moves::genMoves<!moves::QSearch, moves::Quiets>(b, ml);
            }
        }
        return currEval;
    }

    Eval Engine::alphaBetaSearch(PrincipalVariation& pv, Eval alpha, Eval beta, int depth, bool nullBranch)
    {
#ifndef NDEBUG
        board::QBB currentBoard = b.boards.back();
#endif // !NDEBUG

        if (depth <= 0)
        {
            return quiesceSearch(alpha, beta, depth);
        }

        auto nodeType = Tables::ALL;

        if (shouldStop())
        {
            SearchFlags::searching.clear();
        }

        uciUpdate();

        if (insufficientMaterial(b) || threeFoldRep() || b.boards.back().get50() == 50)
            return 0;
        ++nodes;
        
        if (Tables::tt[b.hashes.back()].key == b.hashes.back() && Tables::tt[b.hashes.back()].depth >= depth)
        {
            auto nodetype = Tables::tt[b.hashes.back()].nodeType;
            auto eval = Tables::tt[b.hashes.back()].eval;
            if (nodetype == Tables::ALL && eval < alpha)
            {
                return eval;
            }
            else if (nodetype == Tables::CUT && eval > beta)
            {
                return eval;
            }
        }

        PrincipalVariation pvChild;

        const bool inCheck = moves::isInCheck(b);

        if (!isPVNode(alpha, beta) && !nullBranch && !inCheck)
        {
            b.makeMove(0);
            Eval nulleval = -alphaBetaSearch(pvChild, -beta, -beta + 1, depth - 3, true);
            b.unmakeMove(0);
            assert(b.boards.back() == currentBoard);
            if (nulleval >= beta)
            {
                return nulleval;
            }
        }

        if (inCheck)
        {
            ++depth;
        }

        pvChild.clear();

        Move topMove = 0;
        Eval currEval = negInf;
        moves::MoveOrder moves(&killers, &historyHeuristic, b.hashes.back(), ply());
        Move nextMove = 0;
        std::size_t i = 0;
        Eval besteval = negInf;
        const bool PVNode = isPVNode(alpha, beta);

        const bool doFPruning = (depth == 1 || depth == 2) && !moves::isInCheck(b);
        bool moveWasPruned = false;
        bool everythingPruned = true;

        auto materialBalance = evaluate.materialBalance(b);

        Eval margin = posInf;
        if (depth == 1)
            margin = 300;
        else if (depth == 2)
            margin = 500;

        for (; moves.next(b, nextMove); ++i)
        {
            assert(moves::isLegalMove(b, nextMove));
            if (!SearchFlags::searching.test())
            {
                throw Timeout();
            }
            
            bool isMovingTo7thRank = moves::getBB(board::getMoveToSq(nextMove)) & board::rankMask(board::a7);
            if (doFPruning 
                && !PVNode
                && i != 0
                && !moves::moveGivesCheck(b, nextMove)
                && std::abs(alpha) < 10000
                && std::abs(beta) < 10000
                && !board::isPromo(nextMove)
                && !(b.boards.back().getPieceType(board::getMoveFromSq(nextMove)) == constants::myPawn && isMovingTo7thRank)
                && materialBalance + eval::getCaptureValue(b, nextMove) + margin <= alpha)
            {
                assert(margin != posInf);
                moveWasPruned = true;
                continue;
            }

            everythingPruned = false;
            b.makeMove(nextMove);

            if (i == 0)
            {
                currEval = -alphaBetaSearch(pvChild, -beta, -alpha, depth - 1, nullBranch);
            }
            else
            {
                bool isKiller = moves.stageReturned == moves::Stage::killer1Stage || moves.stageReturned == moves::Stage::killer2Stage;
                auto LMRReduction = LMR(i, b.boards[b.boards.size() - 2], nextMove, b, depth, PVNode, isKiller);
                currEval = -alphaBetaSearch(pvChild, -alpha - 1, -alpha, depth - 1 - LMRReduction, nullBranch);
                if (LMRReduction && currEval > alpha)
                {
                    currEval = -alphaBetaSearch(pvChild, -alpha - 1, -alpha, depth - 1, nullBranch);
                }
                if (currEval > alpha && currEval < beta)
                {
                    currEval = -alphaBetaSearch(pvChild, -beta, -alpha, depth - 1, nullBranch);
                }
            }
            besteval = std::max(besteval, currEval);
            b.unmakeMove(nextMove);
            assert(b.boards.back() == currentBoard);
            if (besteval >= beta)
            {
                nodeType = Tables::CUT;
                Tables::tt.tryStore(b.hashes.back(), depth, besteval, nextMove, nodeType, initialPos, moveWasPruned);
                if (!b.boards.back().isCapture(nextMove))
                {
                    killers.storeKiller(nextMove, ply());
                    auto piececodeidx = b.boards.back().getPieceCodeIdx(board::getMoveFromSq(nextMove));
                    historyHeuristic.updateHistory(piececodeidx, board::getMoveToSq(nextMove), depth);
                }
                return besteval;
            }
            if (currEval > alpha)
            {
                nodeType = Tables::PV;
                topMove = nextMove;
                alpha = currEval;
                pv.clear();
                pv.splice_after(pv.before_begin(), pvChild);
                pv.push_front(topMove);
            }
        }
        if (i == 0)
        {
            return moves::isInCheck(b) ? negInf : 0;
        }

        if (nodeType == Tables::PV)
        {
            Tables::tt.tryStore(b.hashes.back(), depth, besteval, topMove, nodeType, initialPos, moveWasPruned);
        }
        else
        {
            Tables::tt.tryStore(b.hashes.back(), depth, besteval, topMove, nodeType, initialPos, moveWasPruned);
        }
        return everythingPruned ? alpha : besteval;
    }
}