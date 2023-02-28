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
#include "movegen.hpp"
#include "eval.hpp"
#include "auxiliary.hpp"
#include "uci.hpp"

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
        engine_out << "info depth " << currIDdepth << " "
            << "score cp " << eval << " "
            << "time " << elapsed().count() << " "
            << "nodes " << nodes << " "
            << "nps " << nodes / std::max(aux::castsec(elapsed()).count(), 1LL) << " "
            << "pv " << getPVuciformat(b) << std::endl;
        engine_out.emit();
    }

    std::string Engine::line2string(board::QBB b, const std::vector<board::Move>& moves)
    {
        std::string s = move2uciFormat(b, moves[0]);
        b.makeMove(moves[0]);
        for (std::size_t i = 1; i != moves.size(); ++i)
        {
            s.append(" ").append(move2uciFormat(b, moves[i]));
            b.makeMove(moves[i]);
        }
        return s;
    }

    std::string Engine::getCurrline(board::QBB b)
    {
        std::vector<board::Move> moves{ prevMoves.begin() + initialMove, prevMoves.end() };
        return line2string(b, moves);
    }

    std::size_t Engine::ply() const
    {
        return prevPos.size() - initialPos;
    }

    std::string Engine::move2uciFormat(const board::QBB& b, board::Move m)
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
        std::uint64_t currHash = prevPos.back();
        for (int i = prevPos.size() - 1; i >= 0; i -= 2)
        {
            if (prevPos[i] == currHash)
                ++cnt;
        }
        return cnt >= 3;
    }

    bool Engine::isPVNode(Eval alpha, Eval beta)
    {
        return alpha + 1 != beta;
    }

    int Engine::LMR(std::size_t i, const board::QBB& before, board::Move m, const board::QBB& after, int currDepth, bool PV)
    {
        if (movegen::isInCheck(before)
            || movegen::isInCheck(after)
            || currDepth < 3
            || PV
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

    bool Engine::shouldStop() noexcept
    {
        if (settings.ponder)
            return false;

        bool overtime = !settings.infiniteSearch && (elapsed() > moveTime || elapsed() > settings.maxTime);

        return overtime || nodes > settings.maxNodes || elapsed() > settings.maxTime || currIDdepth > settings.maxDepth;
    }
    
    void Engine::newGame()
    {
        killers = Tables::KillerTable();
        historyHeuristic = Tables::HistoryTable();
    }

    void Engine::rootSearch(const board::QBB& b, std::chrono::time_point<std::chrono::steady_clock> s,
        const MoveHistory& moveHist, const PositionHistory& posHist)
    {
        searchStart = s;
        lastUpdate = s;
        prevMoves = moveHist;
        initialMove = prevMoves.size();
        prevPos = posHist;
        initialPos = prevPos.size();
        engineW = b.isWhiteToPlay();
        currIDdepth = 0;
        nodes = 0;
        hash = prevPos.back();
        auto mytime = engineW ? settings.wmsec : settings.bmsec;
        [[maybe_unused]] auto myinc = engineW ? settings.winc : settings.binc;
        auto moveNumber = (prevPos.size() + 2) / 2;
        if (settings.movestogo == std::numeric_limits<std::size_t>::max() || settings.movestogo == 0)
        {
            moveTime = aux::castms(moveNumber < 12 ? mytime / 40 : 0.1 * mytime);
        }
        else
        {
            moveTime = aux::castms((0.95*mytime) / settings.movestogo);
        }
        rootMoves.clear();
        movegen::genMoves(b, rootMoves);
        
        for (auto& i : rootMoves)
        {
            i.score = negInf;
        }

        Eval worstCase = negInf;
        this->eval = negInf;
        board::QBB bcopy = b;
        for (unsigned int k = 1; k <= 128; ++k)
        {
            currIDdepth = k;
            worstCase = negInf;
            PrincipalVariation pv;
            std::size_t i = 0;
            for (auto& [move, score] : rootMoves)
            {
                if (!searchFlags::searching.test())
                    goto endsearch;
                bcopy.makeMove(move);
                StoreInfo recordMove(prevMoves, move);
                auto oldhash = hash;
                hash ^= Tables::tt.incrementalUpdate(move, b, bcopy);
                try 
                {
                    if (i == 0)
                    {
                        score = -alphaBetaSearch(bcopy, pv, negInf, -worstCase, k - 1, false);
                    }
                    else
                    {
                        auto tmp = -alphaBetaSearch(bcopy, pv, -worstCase - 1, -worstCase, k - 1, false);
                        if (tmp > worstCase)
                        {
                            tmp = -alphaBetaSearch(bcopy, pv, negInf, -worstCase, k - 1, false);
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
                bcopy = b;
                hash = oldhash;
                ++i;
            }

            std::stable_sort(rootMoves.begin(), rootMoves.end(), [](const auto& a, const auto& b) {
                return a > b;
                });
            eval = rootMoves[0].score;
            printPV(b);
        }
    endsearch:
        searchFlags::searching.clear();
        /*
        engine_out << "info string capturePct " << tt->capturePct(b) << std::endl;
        engine_out << "info string usedPct " << tt->usedPct() << std::endl;
        engine_out << "info string PVNode " << tt->nodeTypePct(Tables::PV) << std::endl;
        engine_out << "info string CNode " << tt->nodeTypePct(Tables::CUT) << std::endl;
        engine_out << "info string ANode " << tt->nodeTypePct(Tables::ALL) << std::endl;
        */
        eval = rootMoves[0].score;
        engine_out << "bestmove " << move2uciFormat(b, rootMoves[0].m) << std::endl;
        engine_out.emit();
    }

    Eval Engine::quiesceSearch(const board::QBB& b, Eval alpha, Eval beta, int depth)
    {
        StoreInfo recordNode(prevPos, hash);

        if (threeFoldRep())
            return 0;
        if (b.get50() == 50)
            return 0;
        if (shouldStop())
            searchFlags::searching.clear();
        ++nodes;

        if (Tables::tt[hash].key == hash && Tables::tt[hash].depth >= depth)
        {
            auto nodetype = Tables::tt[hash].nodeType;
            auto eval = Tables::tt[hash].eval;
            if (nodetype == Tables::PV)
                return eval;
            else if (nodetype == Tables::ALL && eval < alpha)
                return eval;
            else if (nodetype == Tables::CUT && eval > beta)
                return eval;
        }


        movegen::Movelist<movegen::ScoredMove> ml;
        movegen::genMoves<movegen::QSearch>(b, ml);
        auto captureIterations = ml.size();
        bool check = movegen::isInCheck(b);
        Eval standpat = negInf;
        if (!check)
        {
            standpat = eval::evaluate(b);
            if (standpat >= beta)
            {
                return standpat;
            }
            else if (standpat >= alpha)
            {
                alpha = standpat;
            }
        }
        if (!check && !ml.size())
        {
            movegen::genMoves<!movegen::QSearch, movegen::Quiets>(b, ml);
            if (!ml.size())
            {
                return 0;
            }
            else
            {
                return standpat;
            }
        }
        else if (check && !ml.size())
        {
            movegen::genMoves<!movegen::QSearch, movegen::Quiets>(b, ml);
            if (!ml.size())
            {
                return negInf;
            }
        }


        Eval currEval = standpat;

        board::QBB bcopy = b;

        for (std::size_t i = 0; auto& [move, score] : ml)
        {
            if (i < captureIterations)
                score = eval::mvvlva(b, move);
            ++i;
        }

        for (std::size_t i = 0; i != ml.size(); ++i)
        {
            if (i + 1 < captureIterations)
            {
                std::iter_swap(ml.begin() + i, std::max_element(ml.begin() + i, ml.end()));
            }
            if (i < captureIterations)
            {
                if (!check && eval::getCaptureValue(b, ml[i].m) + 200 + standpat <= alpha)
                {
                    continue;
                }
                if (ml[i].score < 0)
                {
                    ml[i].score = eval::see(b, ml[i].m);
                    if (ml[i].score < 0)
                    {
                        if (check && i + 1 == captureIterations)
                        {
                            movegen::genMoves<!movegen::QSearch, movegen::Quiets>(b, ml);
                        }
                        continue;
                    }
                }
            }
            if (!searchFlags::searching.test())
                throw Timeout();
            auto oldhash = hash;
            bcopy.makeMove(ml[i].m);
            StoreInfo recordMove(prevMoves, ml[i].m);
            hash ^= Tables::tt.incrementalUpdate(ml[i].m, b, bcopy);
            
            currEval = std::max<Eval>(currEval, -quiesceSearch(bcopy, -beta, -alpha, depth - 1));
            bcopy = b;
            hash = oldhash;
            alpha = std::max(currEval, alpha);
            if (alpha >= beta)
            {
                return currEval;
            }
            if (check && i + 1 == captureIterations)
            {
                movegen::genMoves<!movegen::QSearch, movegen::Quiets>(b, ml);
            }
        }
        return currEval;
    }

    Eval Engine::alphaBetaSearch(const board::QBB& b, PrincipalVariation& pv, Eval alpha, Eval beta, int depth, bool nullBranch)
    {
        if (depth <= 0)
        {
            return quiesceSearch(b, alpha, beta, depth);
        }
        auto nodeType = Tables::ALL;
        
        if (shouldStop())
        {
            searchFlags::searching.clear();
        }

        uciUpdate();

        if (b.get50() == 50)
        {
            return 0;
        }

        StoreInfo recordNode(prevPos, hash);
        if (threeFoldRep())
        {
            return 0;
        }
        ++nodes;
        
        if (Tables::tt[hash].key == hash && Tables::tt[hash].depth >= depth)
        {
            auto nodetype = Tables::tt[hash].nodeType;
            auto eval = Tables::tt[hash].eval;
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

        const bool inCheck = movegen::isInCheck(b);

        if (!nullBranch && !inCheck)
        {
            board::QBB bnull = b;
            auto oldhash = hash;
            hash ^= Tables::tt.nullUpdate(bnull);
            bnull.doNullMove();
            StoreInfo recordMove(prevMoves, 0);
            Eval nulleval = -alphaBetaSearch(bnull, pvChild, -beta, -beta + 1, depth - 3, true);
            hash = oldhash;
            if (nulleval >= beta)
            {
                return nulleval;
            }
        } // TODO new search routine to avoid passing PV to null move search

        if (inCheck)
        {
            ++depth;
        }

        pvChild.clear();

        board::Move topMove = 0;
        Eval currEval = negInf;
        movegen::MoveOrder moves(&killers, &historyHeuristic, b, hash, ply());
        board::Move nextMove = 0;
        board::QBB bcopy = b;
        std::size_t i = 0;
        Eval besteval = negInf;
        const bool PVNode = isPVNode(alpha, beta);
        //auto staticEval = std::make_pair(0, false);
        for (; moves.next(b, nextMove); ++i)
        {
            if (!searchFlags::searching.test())
            {
                throw Timeout();
            }
            /*
            if (futilityPruning(b, nextMove, depth, PVNode))
            {
                if (!staticEval.second) staticEval = std::make_pair(eval::evaluate(b), true);

                if (staticEval.first + 900 < alpha)
                {
                    moves.disableQuiets();
                    continue;
                }
            }
            */
            auto oldhash = hash;
            bcopy.makeMove(nextMove);

            StoreInfo recordMove(prevMoves, nextMove);
            hash ^= Tables::tt.incrementalUpdate(nextMove, b, bcopy);
            if (i == 0)
            {
                currEval = -alphaBetaSearch(bcopy, pvChild, -beta, -alpha, depth - 1, nullBranch);
            }
            else
            {
                auto LMRReduction = LMR(i, b, nextMove, bcopy, depth, PVNode);
                currEval = -alphaBetaSearch(bcopy, pvChild, -alpha - 1, -alpha, depth - 1 - LMRReduction, nullBranch);
                if (LMRReduction && currEval > alpha)
                {
                    currEval = -alphaBetaSearch(bcopy, pvChild, -alpha - 1, -alpha, depth - 1, nullBranch);
                }
                if (currEval > alpha && currEval < beta)
                {
                    currEval = -alphaBetaSearch(bcopy, pvChild, -beta, -alpha, depth - 1, nullBranch);
                }
            }
            besteval = std::max(besteval, currEval);
            bcopy = b;
            hash = oldhash;
            if (besteval >= beta)
            {
                nodeType = Tables::CUT;
                Tables::tt.tryStore(hash, depth, besteval, nextMove, nodeType, initialPos);
                if (!b.isCapture(nextMove))
                {
                    killers.storeKiller(nextMove, ply());
                    historyHeuristic.updateHistory(b, nextMove, depth);
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
            return movegen::isInCheck(b) ? negInf : 0;
        }

        if (nodeType == Tables::PV)
        {
            Tables::tt.store(hash, depth, besteval, topMove, nodeType, initialPos);
        }
        else
        {
            Tables::tt.tryStore(hash, depth, besteval, topMove, nodeType, initialPos);
        }
        return besteval;
    }
}