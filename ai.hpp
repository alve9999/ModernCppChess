#pragma once
#include "board.hpp"
#include "check.hpp"
#include "constants.hpp"
#include "eval.hpp"
#include "hash.hpp"
#include "movegen.hpp"
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <omp.h>
#include <thread>
#include "SEE.hpp"
#include "minimax_info.hpp"
#include "parameter.hpp"
#include "nnue.h"

extern std::atomic<bool> shouldStop;
extern long node_count;

inline long node_count = 0;
inline std::atomic<bool> shouldStop(false);
#define CONTEMPT_FACTOR 0

// --- PV Table Definitions ---
inline const int MAX_SEARCH_DEPTH = 99;
inline int previousPvLineLength = 0;
struct MovePV {
    uint8_t from = 255;
    uint8_t to = 255;
};

inline MovePV pvTable[MAX_SEARCH_DEPTH + 1][MAX_SEARCH_DEPTH + 1];
inline int pvLength[MAX_SEARCH_DEPTH + 1];
inline MovePV previousPvLine[MAX_SEARCH_DEPTH + 1];

constexpr int MAX_KILLER_MOVES = 2;
constexpr int MAX_KILLER_PLY = 99; 

static uint16_t killerMoves[MAX_KILLER_PLY][MAX_KILLER_MOVES] = {0};

static int16_t captureHistory[2][64][64] = {0};
static uint16_t counterHistoryTable[2][64][64] = {0};
static uint16_t followUpTable[2][64][64] = {0};

inline uint16_t packMove(uint8_t from, uint8_t to) {
    return (static_cast<uint16_t>(from) << 8) | to;
}

inline void unpackMove(uint16_t packed, uint8_t& from, uint8_t& to) {
    from = (packed >> 8) & 0xFF;
    to = packed & 0xFF;
}

inline int getCounterHistoryBonus(bool isWhite, uint8_t prevFrom, uint8_t prevTo, uint8_t from, uint8_t to) {
    uint16_t counterMove = counterHistoryTable[isWhite][prevFrom][prevTo];
    uint16_t move = packMove(from, to);
    if (counterMove == move) {
        return COUNTER_HISTORY_BONUS;
    }
    return 0;
}

inline void updateCounterHistory(bool isWhite, uint8_t prevFrom, uint8_t prevTo, uint8_t from, uint8_t to) {
    counterHistoryTable[isWhite][prevFrom][prevTo] = packMove(from, to);
}


inline int getFollowUpBonus(bool isWhite, uint8_t prevPrevFrom, uint8_t prevPrevTo, uint8_t from, uint8_t to) {
    uint16_t followUpMove = followUpTable[isWhite][prevPrevFrom][prevPrevTo];
    uint16_t move = packMove(from, to);
    if (followUpMove == move) {
        return FOLLOW_UP_BONUS;
    }
    return 0;
}

inline void updateFollowUp(bool isWhite, uint8_t prevPrevFrom, uint8_t prevPrevTo, uint8_t from, uint8_t to) {
    followUpTable[isWhite][prevPrevFrom][prevPrevTo] = packMove(from, to);
}

inline void updateKillerMoves(int ply, uint8_t from, uint8_t to) {
    if (ply >= MAX_KILLER_PLY) return;
    
    uint16_t move = packMove(from, to);
    
    if (killerMoves[ply][0] == move) return;
    
    killerMoves[ply][1] = killerMoves[ply][0];
    killerMoves[ply][0] = move;
}

inline int getKillerMoveBonus(uint8_t from, uint8_t to, int ply) {
    uint16_t move = packMove(from, to);
    
    if (move == killerMoves[ply][0]) return KILLER_MOVE_BONUS;
    else if (move == killerMoves[ply][1]) return KILLER_MOVE_BONUS;
    
    return 0; 
}

inline void resetKillerMoves() {
    memset(killerMoves, 0, sizeof(killerMoves));
}

inline void sortMoves(Callback *array, int count) {
    std::sort(array, array + count, [](const Callback &a, const Callback &b) {
        return a.value > b.value;
    });
}

inline int clamp(int value, int min, int max) {
    if (value < min) {
        return min;
    }
    if (value > max) {
        return max;
    }
    return value;
}
inline void ageHistoryTable() {
    for (int i = 0; i < 2; i++) {
        for (int from = 0; from < 64; from++) {
            for (int to = 0; to < 64; to++) {
                historyTable[i][from][to] /= HISTORY_AGE_FACTOR;
                captureHistory[i][from][to] /= HISTORY_AGE_FACTOR;
            }
        }
    }
}
inline void clearHistoryTable() {
    for (int i = 0; i < 2; i++) {
        for (int from = 0; from < 64; from++) {
            for (int to = 0; to < 64; to++) {
                historyTable[i][from][to] = 0;
            }
        }
    }
}

#define MAX_HISTORY 10000
template <bool IsWhite>
inline void updateHistory(int from, int to, int depth) {
    int clampedBonus = clamp(depth, -MAX_HISTORY, MAX_HISTORY);
    historyTable[IsWhite][from][to] +=
        clampedBonus -
        historyTable[IsWhite][from][to] * abs(clampedBonus) / MAX_HISTORY;
}

template <bool IsWhite>
inline void updateCaptureHistory(int from, int to, int depth) {
    int clampedBonus = clamp(depth, -MAX_HISTORY, MAX_HISTORY);
    captureHistory[IsWhite][from][to] +=
        clampedBonus -
        captureHistory[IsWhite][from][to] * abs(clampedBonus) / MAX_HISTORY;
}


template <class BoardState status>
inline int quiescence(const Board &brd, minimax_info_t &info) noexcept {
    int ep = info.ep;
    int alpha = info.alpha;
    int beta = info.beta;
    int score = nnue_evaluate(info.accPair, status.IsWhite);
    uint64_t key = info.key;
    int qdepth = info.depth;
    int irreversibleCount = info.irreversibleCount;
    int ply = info.ply;
    bool isPVNode = info.isPVNode;
    bool isCapture = info.isCapture;

    node_count++;


    int standPat = -score;
    if (standPat >= beta) {
        return beta;
    }
    if (alpha < standPat) {
        alpha = standPat;
    }

    if (qdepth == 0) {
        return standPat;
    }



    Callback ml[217];
    int count = 0;
    genMoves<status, 1, 1>(brd, ep, ml, count);
    for(int i = 0; i < count; i++) {
        ml[i].value += captureHistory[status.IsWhite][ml[i].from][ml[i].to];
    }

    
    sortMoves(ml, count);
    for (int i = 0; i < count; i++) {
        if (ml[i].value < 0) {
            continue;
        }

        move_info_t moveInfo;
        moveInfo.from = ml[i].from;
        moveInfo.to = ml[i].to;
        moveInfo.alpha = -beta;
        moveInfo.beta = -alpha;
        moveInfo.score = -score;
        moveInfo.accPair = info.accPair;
        moveInfo.key = key;
        moveInfo.depth = qdepth - 1;
        moveInfo.irreversibleCount = irreversibleCount;
        moveInfo.ply = ply;
        moveInfo.isPVNode = false;
        int eval = -ml[i].move(brd, moveInfo);
        if (eval >= beta) {
            return beta;
        }
        if (eval > alpha) {
            alpha = eval;
        }
    }

    return alpha;
}

inline int calculateExtension(bool isCapture,bool isCheck, bool isPVNode,bool isOneReplay){
    if(isPVNode){
    	return 0;
    }
    if(isCapture){
    	return 0;
    }
    if(isCheck){
	    return 1;
    }
    if(isOneReplay){
        return 1;
    }
    return 0;
}

template <class BoardState status>
inline int minimax(const Board &brd, minimax_info_t &info) noexcept {
    int ep = info.ep;
    int alpha = info.alpha;
    int beta = info.beta;
    AccumulatorPair* accPair = info.accPair;
    info.score = nnue_evaluate(info.accPair, status.IsWhite);
    int score = info.score;
    uint64_t key = info.key;
    int depth = info.depth;
    int irreversibleCount = info.irreversibleCount;
    int ply = info.ply;
    bool isPVNode = info.isPVNode;
    bool isCapture = info.isCapture;
    int from = info.from;
    int to = info.to;
    bool nullMove = info.nullMove;
    node_count++;
    

    bool improving = true;

    if(info.prevMove != nullptr) {
        if(info.prevMove->prevMove != nullptr) {
            improving = score > info.prevMove->prevMove->score;
        }
    }

    uint64_t kingBan = 0;
    if (depth >= 1) {
        generateKingBan<status.IsWhite>(brd, kingBan);
    }
    bool inCheck = status.IsWhite ? (brd.WKing & kingBan) != 0
                                  : (brd.BKing & kingBan) != 0;

    // maybe (!isCapture)
    if ((!isPVNode) && (!inCheck) && (depth >= 1) && (depth <= 4)) {
        int staticEval = -score;
        int margin = RFP_MARGIN * depth /(improving ? 2 : 1);

        if (staticEval >= beta + margin) {
            return staticEval;
        }
    }

    if (ply <= MAX_SEARCH_DEPTH) {
        pvLength[ply] = 0;
    }

    if (shouldStop.load()) {
        return beta;
    }

    if (depth == 0) {
        // return -score;
        //
        minimax_info_t quiescenceInfo;
        quiescenceInfo.ep = ep;
        quiescenceInfo.alpha = alpha;
        quiescenceInfo.beta = beta;
        quiescenceInfo.score = score;
        quiescenceInfo.accPair = info.accPair;
        quiescenceInfo.key = key;
        quiescenceInfo.depth = 5;
        quiescenceInfo.irreversibleCount = irreversibleCount;
        quiescenceInfo.ply = 0;
        quiescenceInfo.isPVNode = isPVNode;
        quiescenceInfo.isCapture = 0;
        quiescenceInfo.prevMove = &info;
        quiescenceInfo.from = from;
        quiescenceInfo.to = to;

        return quiescence<status>(brd, quiescenceInfo);
    } else {
        prevHash.push_back(key);
        if (irreversibleCount >= 4) {
            if (std::count(prevHash.end() -
                               std::min<size_t>(irreversibleCount + 1,
                                                prevHash.size()),
                           prevHash.end(), key) > 2) {
                prevHash.pop_back();
                return (status.IsWhite == white) ? -CONTEMPT_FACTOR
                                                 : CONTEMPT_FACTOR;
            }
        }

        int hashf = 1;

        MovePV expectedPvMove;
        if (isPVNode && ply < previousPvLineLength) {
            expectedPvMove = previousPvLine[ply];
        } else {
            expectedPvMove.from = 255;
            expectedPvMove.to = 255;
        }

        uint8_t fromHash = 255;
        uint8_t toHash = 255;
        if (depth > 1) {
            res val = TT.probe_hash(depth, alpha, beta, key);
            fromHash = val.from;
            toHash = val.to;
            if (val.value != UNKNOWN) {

                prevHash.pop_back();
                return val.value;
            }
        }

        bool hasSufficientMaterial = true;
        if (status.IsWhite) {
            hasSufficientMaterial =
                (brd.WQueen | brd.WRook | brd.WBishop | brd.WKnight) != 0;
        } else {
            hasSufficientMaterial =
                (brd.BQueen | brd.BRook | brd.BBishop | brd.BKnight) != 0;
        }

        // bool inWindow = (alpha == (beta - 1));
        if (depth >= 4 && !inCheck && !isPVNode && !isCapture &&
            hasSufficientMaterial) {
            int R = 3 + depth / 4 + std::min(((-score) - beta) / 200, 3);
            R = std::clamp(R, 2, depth - 1);
            constexpr BoardState NextState =
                BoardState(!status.IsWhite, false, status.WLC, status.WRC,
                           status.BLC, status.BRC);

            minimax_info_t nullMoveInfo;
            nullMoveInfo.ep = ep;
            nullMoveInfo.alpha = -beta;
            nullMoveInfo.beta = -beta + 1;
            nullMoveInfo.score = -score;
            nullMoveInfo.accPair = info.accPair;
            nullMoveInfo.key = toggle_side_to_move(key);
            nullMoveInfo.depth = depth - R;
            nullMoveInfo.irreversibleCount = irreversibleCount + 1;
            nullMoveInfo.ply = ply + 1;
            nullMoveInfo.isPVNode = false;
            nullMoveInfo.isCapture = false;
            nullMoveInfo.nullMove = true;
            nullMoveInfo.prevMove = &info;
            nullMoveInfo.from = 255;
            nullMoveInfo.to = 255;
            int nullMoveScore = -minimax<NextState>(brd, nullMoveInfo);

            if (nullMoveScore >= beta) {
                prevHash.pop_back();
                return nullMoveScore;
            }
        }


        Callback ml[217];
        int count = 0;

        uint64_t kingBan = genMoves<status, 1, 0>(brd, ep, ml, count);

        for (int i = 0; i < count; i++) {
            if (ml[i].from == fromHash && ml[i].to == toHash) {
                ml[i].value += 10000000;
            }
            if (ml[i].from == expectedPvMove.from &&
                ml[i].to == expectedPvMove.to) {
                ml[i].value += 20000000;
            }
            if (!ml[i].capture && !ml[i].promotion) {
                ml[i].value += getKillerMoveBonus(ml[i].from, ml[i].to, ply);
            }
            else{
                ml[i].value += captureHistory[status.IsWhite][ml[i].from][ml[i].to];
            }
            if(!ml[i].capture && info.prevMove != nullptr && !info.prevMove->nullMove) {
                ml[i].value += getCounterHistoryBonus(status.IsWhite, info.prevMove->from, info.prevMove->to, ml[i].from, ml[i].to);
            }
            if(!ml[i].capture && info.prevMove != nullptr && info.prevMove->prevMove != nullptr && !info.prevMove->prevMove->nullMove) {
                ml[i].value += getFollowUpBonus(status.IsWhite, info.prevMove->prevMove->from, info.prevMove->prevMove->to, ml[i].from, ml[i].to);
            }
        }

        sortMoves(ml, count);

        bool outOfMoves = (count == 0);
        if (outOfMoves && inCheck) {
            prevHash.pop_back();
            return (-99999 + ply);
        }
        if (outOfMoves) {
            prevHash.pop_back();
            return 0;
        }

        bool futilityPruning = false;
        int futilityMargin = 0;
        
        if (depth <= 2 && !isPVNode && !inCheck && alpha > -90000 && alpha < 90000) {
            futilityMargin = depth == 1 ? (FP_BASE) : (FP_BASE + FP_ADD);
            
            if (-score + futilityMargin <= alpha) {
                futilityPruning = true;
            }
        }
        int maxIndex = -1;
        int bestEval = -99999;
        bool firstMove = true;

	    int extension = calculateExtension(isCapture,inCheck,isPVNode,count==1);

        for (int i = 0; i < count; i++) {

            if (futilityPruning && i > 0 && !ml[i].capture && !ml[i].promotion && !inCheck) {
                continue;
            }
            int eval;

            bool doFullSearch = true;
            int reduction = 0;

            double reductionFactor = 0.0;
            if(improving) {
                reductionFactor = 0.0;
            } else {
                reductionFactor = -0.0;
            }

            if (!isPVNode && !ml[i].capture && !inCheck && !ml[i].promotion &&
                depth >= 3 && i > 1) {
                int baseRed = 1.35 + int(std::log(depth) * std::log(i) / (2.75 + reductionFactor));
                reduction = std::clamp(baseRed, 1, depth - 1);
            } else if (!isPVNode && !inCheck && depth >= 3 && i > 1) {
                int baseRed = 0.2 + int(std::log(depth) * std::log(i) / (3.35 + reductionFactor));
                reduction = std::clamp(baseRed, 1, depth - 1);
            }

            if (reduction > 0) {
                doFullSearch = false;
                move_info_t moveInfo;
                moveInfo.from = ml[i].from;
                moveInfo.to = ml[i].to;
                moveInfo.alpha = -alpha - 1;
                moveInfo.beta = -alpha;
                moveInfo.score = -score;
                moveInfo.accPair = info.accPair;
                moveInfo.key = key;
                moveInfo.depth = depth - reduction;
                moveInfo.irreversibleCount = irreversibleCount;
                moveInfo.ply = ply + 1;
                moveInfo.isPVNode = false;
                moveInfo.prevMove = &info;
                eval = -ml[i].move(brd, moveInfo);
                if (eval > alpha) {
                    doFullSearch = true;
                }
            }

            if (doFullSearch) {
                move_info_t moveInfo;
                moveInfo.from = ml[i].from;
                moveInfo.to = ml[i].to;
                moveInfo.alpha = -beta;
                moveInfo.beta = -alpha;
                moveInfo.score = -score;
                moveInfo.accPair = info.accPair;
                moveInfo.key = key;
                moveInfo.depth = depth - 1 + extension;
                moveInfo.irreversibleCount = irreversibleCount;
                moveInfo.ply = ply + 1;
                moveInfo.isPVNode = isPVNode;
                moveInfo.prevMove = &info;

                if (firstMove) {
                    eval = -ml[i].move(brd, moveInfo);
                } else {
                    moveInfo.alpha = -alpha - 1;
                    moveInfo.isPVNode = false;
                    eval = -ml[i].move(brd, moveInfo);

                    if (eval > alpha && eval < beta) {
                        moveInfo.alpha = -beta;
                        moveInfo.isPVNode = isPVNode;
                        eval = -ml[i].move(brd, moveInfo);
                    }
                }
            }

            firstMove = false;

            if(eval > bestEval){
                bestEval = eval;
                maxIndex = i;
                if (eval > alpha) {
                    hashf = 0;
                    alpha = eval;

                    if (ply <= MAX_SEARCH_DEPTH) {
                        pvTable[ply][0].from = ml[i].from;
                        pvTable[ply][0].to = ml[i].to;
                        if (ply + 1 <= MAX_SEARCH_DEPTH) {
                            memcpy(&pvTable[ply][1], &pvTable[ply + 1][0],
                                pvLength[ply + 1] * sizeof(MovePV));
                            pvLength[ply] = pvLength[ply + 1] + 1;
                        } else {
                            pvLength[ply] = 1;
                        }
                    }
                }
            }
    
            // move is to good
            if (eval >= beta) {
                if (!ml[i].capture && !ml[i].promotion) {
                    updateHistory<status.IsWhite>(ml[i].from, ml[i].to, depth*depth);
                    updateKillerMoves(ply, ml[i].from, ml[i].to);
                    if(info.prevMove != nullptr && !info.prevMove->nullMove) {
                        updateCounterHistory(status.IsWhite, info.prevMove->from, info.prevMove->to, ml[i].from, ml[i].to);
                    }
                }
                else{
                    updateCaptureHistory<status.IsWhite>(ml[i].from, ml[i].to, depth*depth);
                }
                for (int j = 0; j < i; j++) {
                    if (!ml[j].capture && !ml[j].promotion) {
                        updateHistory<status.IsWhite>(ml[j].from, ml[j].to,
                                                       -depth);
                    }
                }
                if (depth > 1) {
                    TT.store(depth, beta, 2, key, ml[i].from, ml[i].to);
                }
                prevHash.pop_back();
                return bestEval;
            }
            

        }
        if (shouldStop.load()) {
            prevHash.pop_back();
            return beta;
        }
        if (depth > 1) {
            TT.store(depth, alpha, hashf, key, ml[maxIndex].from,
                     ml[maxIndex].to);
        }
        prevHash.pop_back();
        return bestEval;
    }
}

inline Callback findBestMove(const Board &brd, int ep, bool WH, bool EP,
                             bool WL, bool WR, bool BL, bool BR, int depth,
                             int irreversibleCount, int &previousEval,
                             int &bestFrom, int &bestTo) {
    node_count = 0;
    auto start = std::chrono::high_resolution_clock::now();
    Callback ml[217];
    int count = 0;

    if (0 <= MAX_SEARCH_DEPTH) {
        pvLength[0] = 0;
    }

    AccumulatorPair* accPair = (AccumulatorPair*)malloc(sizeof(AccumulatorPair));
    nnue_init(accPair, brd);
    int score;
    score = nnue_evaluate(accPair, WH);
    uint64_t key = create_hash(brd, WH);
    int bestEval = -100000;
    int bestMoveIndex = -1;
    int alpha = -99999;
    int beta = 99999;

    bool firstMove = true;

    moveGenCall<1, 0>(brd, ep, ml, count, WH, EP, WL, WR, BL, BR);
    uint8_t fromHash = 255;
    uint8_t toHash = 255;
    if (depth != 1) {
        res val = TT.probe_hash(depth, alpha, beta, key);
        fromHash = val.from;
        toHash = val.to;
    }

    MovePV expectedRootPvMove =
        (previousPvLineLength > 0) ? previousPvLine[0] : MovePV{255, 255};
    for (int i = 0; i < count; i++) {
        if (ml[i].from == expectedRootPvMove.from &&
            ml[i].to == expectedRootPvMove.to) {
            ml[i].value += 2000000;
        }
        if (ml[i].from == fromHash && ml[i].to == toHash) {
            ml[i].value += 1000000;
        }
    }

    sortMoves(ml, count);

    if (depth > 5) {
        // aspiration search
        int window = WINDOW_INIT;
        int lo = previousEval - window, hi = previousEval + window;

        bool windowFailed = true;
        int windowIterations = 0;
        while (windowFailed && (windowIterations < 4)) {
            alpha = lo;
            beta = hi;
            firstMove = true;
            windowIterations++;
            windowFailed = false;
            bestEval = -99999;
            bestMoveIndex = -1;
            for (int i = 0; i < count; i++) {
                int eval;

                move_info_t moveInfo;
                moveInfo.from = ml[i].from;
                moveInfo.to = ml[i].to;
                moveInfo.alpha = -beta;
                moveInfo.beta = -alpha;
                moveInfo.score = score;
                moveInfo.accPair = accPair;
                moveInfo.key = key;
                moveInfo.depth = depth - 1;
                moveInfo.irreversibleCount = irreversibleCount;
                moveInfo.ply = 1;
                moveInfo.isPVNode = firstMove;
                moveInfo.prevMove = nullptr;

                eval = -ml[i].move(brd, moveInfo);
                firstMove = false;

                if (eval > bestEval) {
                    bestEval = eval;
                    bestMoveIndex = i;
                    alpha = eval;

                    pvTable[0][0].from = ml[i].from;
                    pvTable[0][0].to = ml[i].to;
                    if (pvLength[1] > 0) {
                        memcpy(&pvTable[0][1], &pvTable[1][0],
                               pvLength[1] * sizeof(MovePV));
                        pvLength[0] = pvLength[1] + 1;
                    } else {
                        pvLength[0] = 1;
                    }
                }
                if (bestEval >= beta || shouldStop.load()) {
                    break;
                }
            }

            if (bestEval <= lo || bestEval >= hi) {
                windowFailed = true;

                if (bestEval <= alpha) {
                    alpha = std::max(bestEval - window * 3, -99999);
                } else {
                    beta = std::min(bestEval + window * 3, 99999);
                }

                window *= WINDOW_MULT;
                lo = previousEval - window;
                hi = previousEval + window;
            }
        }
    }
    if (depth < 6) {
        for (int i = 0; i < count; i++) {
            int eval;
            move_info_t moveInfo;
            moveInfo.from = ml[i].from;
            moveInfo.to = ml[i].to;
            moveInfo.alpha = -beta;
            moveInfo.beta = -alpha;
            moveInfo.score = score;
            moveInfo.accPair = accPair;
            moveInfo.key = key;
            moveInfo.depth = depth - 1;
            moveInfo.irreversibleCount = irreversibleCount;
            moveInfo.ply = 1;
            moveInfo.isPVNode = firstMove;
            moveInfo.prevMove = nullptr;

            eval = -ml[i].move(brd, moveInfo);
            firstMove = false;

            if (shouldStop.load()) {
                break;
            }
            if (eval > bestEval) {
                bestEval = eval;
                bestMoveIndex = i;
                alpha = eval;

                pvTable[0][0].from = ml[i].from;
                pvTable[0][0].to = ml[i].to;
                if (pvLength[1] > 0) {
                    memcpy(&pvTable[0][1], &pvTable[1][0],
                           pvLength[1] * sizeof(MovePV));
                    pvLength[0] = pvLength[1] + 1;
                } else {
                    pvLength[0] = 1;
                }
            }
        }
    }
    if (bestMoveIndex != -1) {
        bestFrom = ml[bestMoveIndex].from;
        bestTo = ml[bestMoveIndex].to;
        previousEval = bestEval;
    }
    free(accPair);
    // print the stats
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;
    int nps = ((double)node_count) / duration.count();
    if (!shouldStop.load()) {
        TT.store(depth, bestEval, 0, key, bestFrom, bestTo);
        printf("info depth %d score cp %d nodes %ld nps %d time %d", depth,
               bestEval, node_count, nps, (int)(1000 * duration.count()));
        // for(int i = 0; i < pvLength[0]; i++) {
        //     printf("%s ", convertMoveToUCI(brd, pvTable[0][i].from,
        //     pvTable[0][i].to).c_str());
        // }
        printf("\n");
    }
    count = 0;

    // find the make version of the move
    moveGenCall<0, 0>(brd, ep, ml, count, WH, EP, WL, WR, BL, BR);
    for (int i = 0; i < count; i++) {
        if (ml[i].from == bestFrom && ml[i].to == bestTo) {
            return ml[i];
        }
    }
    std::cout << "Error: Best move not found in move list!"<< bestFrom <<" " << bestTo << std::endl;
    assert(false);
    return ml[0];
}

inline Callback iterative_deepening(const Board &brd, int ep, bool WH, bool EP,
                             bool WL, bool WR, bool BL, bool BR,
                             double timeLimit, int irreversibleCount) {
    TT.age++;
    resetKillerMoves();
    using clock = std::chrono::high_resolution_clock;
    auto start = clock::now();
    shouldStop.store(false);

    Callback bestMove{};
    int eval = 0;
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
    int bestFrom = 255;
    int bestTo = 255;

    for (int depth = 1; depth <= 99; depth += 1) {
        // clearHistoryTable();
        ageHistoryTable();
        if (shouldStop.load()) {
            break;
        }
        bestMove = findBestMove(brd, ep, WH, EP, WL, WR, BL, BR, depth,
                                irreversibleCount, eval, bestFrom, bestTo);
        if (0 <= MAX_SEARCH_DEPTH && pvLength[0] > 0) {
            previousPvLineLength = pvLength[0];
            memcpy(previousPvLine, &pvTable[0][0],
                   previousPvLineLength * sizeof(MovePV));
        } else {
            previousPvLineLength = 0;
        }
    }
    clearHistoryTable();
    timerThread.join();
    return bestMove;
}

