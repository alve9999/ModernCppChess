#pragma once
#include "board.hpp"
#include "constants.hpp"
#include "eval.hpp"
#include "hash.hpp"
#include <iostream>
#include <chrono>
#include <omp.h>
#include <atomic>
#include <thread>
#include <atomic>

extern std::atomic<bool> shouldStop;
std::atomic<bool> shouldStop(false);

template <class BoardState status, int depth>
inline int minimax(const Board &brd, int ep, int alpha, int beta, int score, uint64_t key) noexcept {


    if constexpr (depth == 0) {
        //TT.store(depth, -score, 0, key);
        return -score;
    }
    else {
        int hashf = 1;
        if constexpr (depth !=1) {
            int val = TT.probe_hash(depth, alpha, beta, key);

            if(val != UNKNOWN) {
                return val;
            }
        }

        Callback ml[100];
        int count = GOOD_MOVE;
        int goodCount = GOOD_MOVE-1;
        genMoves<status, depth, 1>(brd, ep, ml, count, goodCount);
        
        if (count == 0) {
            return -99998;
        }

        int maxEval = -99999;
        if (shouldStop.load()) {
            return 0;
        }
        for (int i = goodCount + 1; i < count; i++) {
            int eval = -ml[i].move(brd, ml[i].from, ml[i].to, -beta, -alpha, -score, key);
            maxEval = std::max(maxEval, eval);
            if (beta <= eval) {
                if constexpr (depth > 1) {
                    TT.store(depth, eval, 2, key);
                }
                return maxEval;
            }
            if(alpha < eval) {
                hashf = 0;
                alpha = eval;
            }
        }
        TT.store(depth, maxEval, hashf, key);
        return maxEval;
    }
}


template <int depth>
inline Callback findBestMove(const Board &brd, int ep, bool WH, bool EP, bool WL, bool WR, bool BL, bool BR, int preferredIndex, int& bestIndex){
    auto start = std::chrono::high_resolution_clock::now();
    Callback ml[100];
    int count = GOOD_MOVE;
    int goodCount = GOOD_MOVE-1;
    moveGenCall<depth, 1>(brd, ep, ml, count, goodCount, WH, EP, WL, WR, BL, BR);
    int intitalScore = WH ? eval<true>(brd) : eval<false>(brd);
    int initialKey = create_hash(brd, WH);
    int bestScore = -99999;
    int bestMoveIndex = 0;
    std::atomic<int> alpha(-99999);
    int beta = 99999;

    if (preferredIndex >= goodCount + 1 && preferredIndex < count) {
        int score = -ml[preferredIndex].move(brd, ml[preferredIndex].from, ml[preferredIndex].to, -beta, -alpha.load(), intitalScore, initialKey);
        bestScore = score;
        bestMoveIndex = preferredIndex;
        alpha.store(score);
    }
    
    #pragma omp parallel for
    for (int i = goodCount + 1; i < count; i++) {
        if(i == preferredIndex) {
            continue;
        }
        int score = -ml[i].move(brd, ml[i].from, ml[i].to, -beta, -alpha.load(), intitalScore, initialKey);
        
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
    printf("score: %d\n", bestScore);
    count = GOOD_MOVE;
    goodCount = GOOD_MOVE-1;
    moveGenCall<1,0>(brd, ep, ml, count, goodCount, WH, EP, WL, WR, BL, BR);
    return ml[bestMoveIndex];
}

Callback iterative_deepening(const Board &brd, int ep, bool WH, bool EP, bool WL, bool WR, bool BL, bool BR, double timeLimit) {
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
            if (std::chrono::duration<double>(now - start).count() > timeLimit) {
                shouldStop.store(true);
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });

    for (int depth = 1; depth <= 14; depth+=2) {
        auto now = clock::now();
        std::chrono::duration<double> elapsed = now - start;
        if (elapsed.count() > timeLimit) {
            break;
        }


        switch (depth) {
            case 1: bestMove = findBestMove<1>(brd, ep, WH, EP, WL, WR, BL, BR, bestMoveIndex, bestMoveIndex); break;
            case 2: bestMove = findBestMove<2>(brd, ep, WH, EP, WL, WR, BL, BR, bestMoveIndex, bestMoveIndex); break;
            case 3: bestMove = findBestMove<3>(brd, ep, WH, EP, WL, WR, BL, BR, bestMoveIndex, bestMoveIndex); break;
            case 4: bestMove = findBestMove<4>(brd, ep, WH, EP, WL, WR, BL, BR, bestMoveIndex, bestMoveIndex); break;
            case 5: bestMove = findBestMove<5>(brd, ep, WH, EP, WL, WR, BL, BR, bestMoveIndex, bestMoveIndex); break;
            case 6: bestMove = findBestMove<6>(brd, ep, WH, EP, WL, WR, BL, BR, bestMoveIndex, bestMoveIndex); break;
            case 7: bestMove = findBestMove<7>(brd, ep, WH, EP, WL, WR, BL, BR, bestMoveIndex, bestMoveIndex); break;
            case 8: bestMove = findBestMove<8>(brd, ep, WH, EP, WL, WR, BL, BR, bestMoveIndex, bestMoveIndex); break;
            case 9: bestMove = findBestMove<9>(brd, ep, WH, EP, WL, WR, BL, BR, bestMoveIndex, bestMoveIndex); break;
            case 10: bestMove = findBestMove<10>(brd, ep, WH, EP, WL, WR, BL, BR, bestMoveIndex, bestMoveIndex); break;
            case 11: bestMove = findBestMove<11>(brd, ep, WH, EP, WL, WR, BL, BR, bestMoveIndex, bestMoveIndex); break;
            case 12: bestMove = findBestMove<12>(brd, ep, WH, EP, WL, WR, BL, BR, bestMoveIndex, bestMoveIndex); break;
            case 13: bestMove = findBestMove<13>(brd, ep, WH, EP, WL, WR, BL, BR, bestMoveIndex, bestMoveIndex); break;
            case 14: bestMove = findBestMove<14>(brd, ep, WH, EP, WL, WR, BL, BR, bestMoveIndex, bestMoveIndex); break;
            default: break;
        }
        printf("Depth %d %d completed\n", depth, bestMoveIndex); 
        maxReachedDepth = depth;

        if (shouldStop.load()) {
            break;
        }
        else{
            globalBestMove = bestMove;
        }
    }
    timerThread.join();
    printf("Max completed depth: %d\n", maxReachedDepth-2);
    return bestMove;
}
