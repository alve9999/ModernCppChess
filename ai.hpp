#pragma once
#include "board.hpp"
#include "constants.hpp"
#include "eval.hpp"
#include "hash.hpp"
#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <omp.h>
#include <thread>

extern std::atomic<bool> shouldStop;
std::atomic<bool> shouldStop(false);
#define STOP -7777


inline void sortMoves(Callback* array, int count) {
    std::sort(array, array + count, [](const Callback& a, const Callback& b) {
        return a.value > b.value;
    });
}

template <class BoardState status>
inline int quiescence(const Board &brd, int ep, int alpha, int beta, int score,
                      uint64_t key, int qdepth) noexcept {

    int standPat = -score;
    if (standPat >= beta){
        return beta;
    }
    if (alpha < standPat){
        alpha = standPat;
    }

    if (qdepth >= 5){
        return standPat;
    }

    Callback ml[217];
    int count = 0;
    genMoves<status, 1, 1>(brd, ep, ml, count);
    sortMoves(ml, count);
    for (int i = 0; i < count; i++) {

        int eval = -ml[i].move(brd, ml[i].from, ml[i].to, -beta, -alpha,
                                    -score, key, qdepth + 1);
        if (eval >= beta){
            return beta;
        }
        if (eval > alpha){
            alpha = eval;
        }
    }

    return alpha;
}

template <class BoardState status>
inline int minimax(const Board &brd, int ep, int alpha, int beta, int score,
                   uint64_t key,const int depth) noexcept {
    if (shouldStop.load()) {
        return STOP;
    }

    if (depth == 0) {
        //return -score;
        return quiescence<status>(brd, ep, alpha, beta, score, key,0);
    } else {
        int hashf = 1;
        if (depth != 1) {
            int val = TT.probe_hash(depth, alpha, beta, key);

            if (val != UNKNOWN) {
                return val;
            }
        }

        Callback ml[217];
        int count = 0;

        uint64_t kingBan =
            genMoves<status, 1, 0>(brd, ep, ml, count);
        sortMoves(ml, count);

        bool inCheck = status.IsWhite ? (brd.WKing & kingBan) != 0
                                      : (brd.BKing & kingBan) != 0;
        bool outOfMoves = (count == 0); 
        if (outOfMoves && inCheck) {
            return -99999;
        }
        if (outOfMoves) {
            return 0;
        }

        int maxEval = -99999;

        for (int i = 0; i < count; i++) {
            int eval = -ml[i].move(brd, ml[i].from, ml[i].to, -beta, -alpha,
                                   -score, key, depth - 1);
            maxEval = std::max(maxEval, eval);

            if (beta <= eval) {
                if(depth > 1) {
                    if (shouldStop.load()) {
                        return STOP;
                    }
                    TT.store(depth, eval, 2, key);
                }
                return maxEval;
            }
            if (alpha < eval) {
                hashf = 0;
                alpha = eval;
            }
        }
        if (shouldStop.load()) {
            return STOP;
        }
        TT.store(depth, maxEval, hashf, key);
        return maxEval;
    }
}

inline Callback findBestMove(const Board &brd, int ep, bool WH, bool EP,
                             bool WL, bool WR, bool BL, bool BR,
                             int preferredIndex, int &bestIndex, int depth) {
    auto start = std::chrono::high_resolution_clock::now();
    Callback ml[217];
    int count = 0; 
    moveGenCall<1, 0>(brd, ep, ml, count, WH, EP, WL, WR, BL,
                             BR);
    sortMoves(ml, count);
    int intitalScore = WH ? eval<true>(brd) : eval<false>(brd);
    int initialKey = create_hash(brd, WH);
    int bestScore = -99999;
    int bestMoveIndex = 0;
    std::atomic<int> alpha(-99999);
    int beta = 99999;

    if (preferredIndex >= 0 && preferredIndex < count) {
        int score = -ml[preferredIndex].move(
            brd, ml[preferredIndex].from, ml[preferredIndex].to, -beta,
            -alpha.load(), intitalScore, initialKey, depth - 1);
        if (shouldStop.load()) {
            count = 0;
            moveGenCall<0, 0>(brd, ep, ml, count, WH, EP, WL, WR,
                                 BL, BR);
            sortMoves(ml, count);
            printf("returning preferred move %d\n", preferredIndex);
            return ml[preferredIndex];
        }
        bestScore = score;
        bestMoveIndex = preferredIndex;
        alpha.store(score);
    }

    //#pragma omp parallel for
    for (int i = 0; i < count; i++) {
        if (i == preferredIndex) {
            continue;
        }
        int score = -ml[i].move(brd, ml[i].from, ml[i].to, -beta, -alpha.load(),
                                intitalScore, initialKey, depth - 1);
        if (shouldStop.load()) {
            continue;
        }
        if (score > bestScore) {
            bestScore = score;
            bestMoveIndex = i;

            int expected = alpha.load();
            while (score > expected &&
                   !alpha.compare_exchange_weak(expected, score)) {
            }
        }
    }
    bestIndex = bestMoveIndex;
    auto end = std::chrono::high_resolution_clock::now();
    // Calculate duration
    std::chrono::duration<double> duration = end - start;
    printf("evaluated %f\n", duration.count());
    printf("info depth %d score cp %d\n", depth, bestScore);
    count = 0;
    moveGenCall<0, 0>(brd, ep, ml, count, WH, EP, WL, WR, BL, BR);
    sortMoves(ml, count);
    return ml[bestMoveIndex];
}

Callback iterative_deepening(const Board &brd, int ep, bool WH, bool EP,
                             bool WL, bool WR, bool BL, bool BR,
                             double timeLimit) {
    using clock = std::chrono::high_resolution_clock;
    auto start = clock::now();
    shouldStop.store(false);

    Callback bestMove{};
    Callback globalBestMove{};

    int maxReachedDepth = 0;
    int bestMoveIndex = -1;
    std::thread timerThread([&] {
        while (true) {
            auto now = clock::now();
            if (std::chrono::duration<double>(now - start).count() >
                timeLimit) {
                shouldStop.store(true);
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });

    for (int depth = 1; depth <= 24; depth += 1) {

        if (shouldStop.load()) {
            break;
        }
        bestMove = findBestMove(brd, ep, WH, EP, WL, WR, BL, BR,bestMoveIndex, bestMoveIndex,depth);
        maxReachedDepth = depth;
    }

    timerThread.join();
    return bestMove;
}
