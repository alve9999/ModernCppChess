#include "uci.hpp"
#include "ai.hpp"
#include "hash.hpp"
#include "move.hpp"
#include "eval.hpp"
#include "movegen.hpp"
#include <iostream>
#include <memory>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <vector>
bool white = false;
bool hasBeenActivated = false;
int historyTable[2][64][64] = {};
std::vector<uint64_t> prevHash = {};
int mg_phase = 0;
int eg_phase = 0;

void printVector(const std::vector<uint64_t>& vec, const std::string& delimiter = " ") {
    for (const auto& elem : vec) {
        std::cout << elem << delimiter;
    }
    std::cout << '\n';
}

void proccessCommand(std::string str, std::unique_ptr<Board> &brd,
                     std::unique_ptr<BoardState> &state,int &irreversibleCount, int &ep) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, ' ')) {
        tokens.push_back(token);
    }
    if (tokens[0] == "quit") {
        exit(0);
    } else if (tokens[0] == "position") {
        prevHash.clear();
        if (tokens[1] == "startpos") {
            brd.reset(new Board(loadFenBoard(
                "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1")));
            state.reset(new BoardState(parseBoardState(
                "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1")));

            white = true;

            // Handle moves after startpos
            int moveIndex = 2;
            if (tokens.size() > 2 && tokens[2] == "moves") {
                moveIndex = 3;
            }

            if (moveIndex < tokens.size()) {
                int ep = 0;
                for (size_t i = moveIndex; i < tokens.size(); i++) {
                    MoveCallbacks move;
                    if (white) {
                        move =
                            algebraicToMove<true>(tokens[i], *brd, *state);
                    } else {
                        move =
                            algebraicToMove<false>(tokens[i], *brd, *state);
                    }
                    brd.reset(new Board(move.boardCallback()));
                    state.reset(new BoardState(move.stateCallback()));
                    white = !white;
                    if(move.irreversible) {
                        irreversibleCount=0;
                    }
                    else {
                        irreversibleCount++;
                    }
                    ep = move.ep;
                    prevHash.push_back(create_hash(*brd, state->IsWhite));
                }
            }
        } else if (tokens[1] == "fen") {
            std::string fen = "";
            size_t fenEnd = tokens.size();

            for (size_t i = 2; i < tokens.size(); i++) {
                if (tokens[i] == "moves") {
                    fenEnd = i;
                    break;
                }
            }

            for (size_t i = 2; i < fenEnd; i++) {
                fen += tokens[i] + (i < fenEnd - 1 ? " " : "");
            }
            brd.reset(new Board(loadFenBoard(fen.c_str())));
            state.reset(new BoardState(parseBoardState(fen.c_str())));
            // printBoard(*brd);
            white = state->IsWhite;

            if (fenEnd < tokens.size()) {

                MoveCallbacks move;
                for (size_t i = fenEnd + 1; i < tokens.size(); i++) {
                    if (white) {
                        move =
                            algebraicToMove<true>(tokens[i], *brd, *state);
                    } else {
                        move =
                            algebraicToMove<false>(tokens[i], *brd, *state);
                    }
                    brd.reset(new Board(move.boardCallback()));
                    state.reset(new BoardState(move.stateCallback()));
                    white = !white;
                    if(move.irreversible) {
                        irreversibleCount=0;
                    }
                    else {
                        irreversibleCount++;
                    }
                    ep = move.ep;
                    prevHash.push_back(create_hash(*brd, state->IsWhite));
                }
            }
            // printBoard(*brd);
            // printBitboard((*brd).Occ);
        }
    } else if (tokens[0] == "isready") {
        std::cout << "readyok" << std::endl;
    } else if (tokens[0] == "uci") {
        std::cout << "id name chess_engien\nid author Alve Lindell\nuciok" << std::endl;
    } else if (tokens[0] == "go") {
        auto start = std::chrono::high_resolution_clock::now();
        int whiteTime = 100000000;
        int blackTime = 100000000;
        int whiteInc = 1000;
        int blackInc = 1000;
        int moveTime = 0;
        for (size_t i = 1; i + 1 < tokens.size(); i++) {
            if(tokens[i] == "movetime"){
                moveTime = std::stoi(tokens[i + 1]);
            }
            if (tokens[i] == "wtime")
                whiteTime = std::stoi(tokens[i + 1]);
            if (tokens[i] == "binc")
                blackInc = std::stoi(tokens[i + 1]);
            if (tokens[i] == "winc")
                whiteInc = std::stoi(tokens[i + 1]);
            if (tokens[i] == "btime")
                blackTime = std::stoi(tokens[i + 1]);
        }
        int time = white ? whiteTime : blackTime;
        int inc = white ? whiteInc : blackInc;
        double think = ((double)time) * 0.00001f + ((double)inc) * 0.0009f;
        if(moveTime > 0){
            think = ((double)moveTime/1000.0f)*0.9f;
        }
        // printf("thinking time: %f\n", think);
        hasBeenActivated = true;
        // calculate next move
        bool whiteTurn = state->IsWhite;
        bool enPassant = state->EP;
        bool whiteLeft = state->WLC;
        bool whiteRight = state->WRC;
        bool blackLeft = state->BLC;
        bool blackRight = state->BRC;

        Callback ml =
            iterative_deepening(*brd, ep, whiteTurn, enPassant, whiteLeft,
                                whiteRight, blackLeft, blackRight, think,irreversibleCount);

        std::cout << "bestmove " << convertMoveToUCI(*brd, ml.from, ml.to)
                  << std::endl;

        MoveResult moveRes = ml.makeMove(*brd, ml.from, ml.to);
        brd.reset(new Board(moveRes.board));
        state.reset(new BoardState(moveRes.state));
        irreversibleCount = 0;
        //std::cout << ttc << " " << ttf << std::endl;
        auto end = std::chrono::high_resolution_clock::now();
        // Calculate duration
        std::chrono::duration<double> duration = end - start;
        // printf("total time %f\n", duration.count());
    }
}

#include <fstream>

void uciRunGame() {

    auto brd = std::make_unique<Board>(loadFenBoard(
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 "));
    auto state = std::make_unique<BoardState>(parseBoardState(
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 "));

    int irreversibleCount = 0;
    int ep = -1;
    while (1) {
        if (std::cin.peek() != EOF) {
            std::string str;
            std::getline(std::cin, str);
            proccessCommand(str, brd, state, irreversibleCount, ep);
        }
    }
}
