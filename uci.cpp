#include "uci.hpp"
#include "ai.hpp"
#include "hash.hpp"
#include "move.hpp"
#include "movegen.hpp"
#include <iostream>
#include <memory>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <vector>
bool white = false;
bool hasBeenActivated = false;

std::string convertToUCI(int index) {
    int row = index / 8;
    int col = index % 8;

    char file = 'a' + col;

    char rank = '1' + (row);

    return std::string(1, file) + std::string(1, rank);
}

std::string convertMoveToUCI(const Board &brd, int from, int to) {
    std::string uci = convertToUCI(from) + convertToUCI(to);

    uint64_t fromMask = 1ULL << from;

    bool isWhitePawn = (brd.WPawn & fromMask) != 0;
    bool isBlackPawn = (brd.BPawn & fromMask) != 0;

    if (isWhitePawn && to / 8 == 7) {
        uci += 'q';
    } else if (isBlackPawn && to / 8 == 0) {
        uci += 'q';
    }

    return uci;
}

void proccessCommand(std::string str, std::unique_ptr<Board> &brd,
                     std::unique_ptr<BoardState> &state) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, ' ')) {
        tokens.push_back(token);
    }
    if (tokens[0] == "quit") {
        exit(0);
    } else if (tokens[0] == "position") {
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
                            algebraicToMove<true>(tokens[i], *brd, *state, ep);
                    } else {
                        move =
                            algebraicToMove<false>(tokens[i], *brd, *state, ep);
                    }
                    brd.reset(new Board(move.boardCallback()));
                    state.reset(new BoardState(move.stateCallback()));
                    white = !white;
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
            printBoard(*brd);
            white = state->IsWhite;

            if (fenEnd < tokens.size()) {
                int ep = 0;
                for (size_t i = fenEnd + 1; i < tokens.size(); i++) {
                    MoveCallbacks move;
                    if (white) {
                        move =
                            algebraicToMove<true>(tokens[i], *brd, *state, ep);
                    } else {
                        move =
                            algebraicToMove<false>(tokens[i], *brd, *state, ep);
                    }
                    brd.reset(new Board(move.boardCallback()));
                    state.reset(new BoardState(move.stateCallback()));
                    white = !white;
                }
            }
            printBoard(*brd);
        }
    } else if (tokens[0] == "isready") {
        std::cout << "readyok" << std::endl;
    } else if (tokens[0] == "uci") {
        std::cout << "id name chess_engien\nid author Alve Lindell\nuciok\n";
    } else if (tokens[0] == "go") {
        int whiteTime = 0;
        int blackTime = 0;
        int whiteInc = 0;
        int blackInc = 0;
        for (size_t i = 1; i + 1 < tokens.size(); i++) {
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
        double think = ((double)time) * 0.00002f + ((double)inc) * 0.001f;
        printf("thinking time: %f\n", think);
        hasBeenActivated = true;
        // calculate next move
        bool whiteTurn = state->IsWhite;
        bool enPassant = state->EP;
        bool whiteLeft = state->WLC;
        bool whiteRight = state->WRC;
        bool blackLeft = state->BLC;
        bool blackRight = state->BRC;
        Callback ml =
            iterative_deepening(*brd, 0, whiteTurn, enPassant, whiteLeft,
                                whiteRight, blackLeft, blackRight, think);

        std::cout << "bestmove " << convertMoveToUCI(*brd, ml.from, ml.to)
                  << std::endl;

        MoveResult moveRes = ml.makeMove(*brd, ml.from, ml.to);
        brd.reset(new Board(moveRes.board));
        state.reset(new BoardState(moveRes.state));

        std::cout << ttc << " " << ttf << std::endl;
    }
}

void uciRunGame() {
    auto brd = std::make_unique<Board>(loadFenBoard(
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 "));
    auto state = std::make_unique<BoardState>(parseBoardState(
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 "));
    while (1) {
        if (std::cin.peek() != EOF) {
            std::string str;
            std::getline(std::cin, str);
            std::cout << str << std::endl;
            proccessCommand(str, brd, state);
        }
    }
}
