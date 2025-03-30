#include "uci.hpp"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <stdlib.h> 
#include "movegen.hpp"
#include "move.hpp"
#include <memory>
bool white = false;

void proccessCommand(std::string str, std::unique_ptr<Board>& brd, std::unique_ptr<BoardState>& state){
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;

    while(std::getline(ss, token, ' ')) {
        tokens.push_back(token);
    }
    if (tokens[0] == "quit") {
        exit(1);
    }
    if(tokens[0] == "position"){
        if(tokens.back()!="startpos"){
            int ep = 0;
            MoveCallbacks move;
            if(white){
	            move = algebraicToMove<true>(tokens.back(),*brd,*state,ep);
            }
            else{
                move = algebraicToMove<false>(tokens.back(),*brd,*state,ep);
            }
            brd.reset(new Board(move.boardCallback()));
            state.reset(new BoardState(move.stateCallback()));

        }
        else{
            white = true;
        }
    }
    if (tokens[0] == "isready") {
        std::cout << "readyok\n";
    } 
    else if (tokens[0] == "uci") {
        std::cout << "id name chess_engien\nid author Alve Lindell\nuciok\n";
    } 
    else if (tokens[0] == "go") {
        //calculate next move
        Callback ml[100];
        int count = 0;
        bool whiteTurn   = state->IsWhite;
        bool enPassant   = state->EP;
        bool whiteLeft   = state->WLC;
        bool whiteRight  = state->WRC;
        bool blackLeft   = state->BLC;
        bool blackRight  = state->BRC;

        moveGenCall(*brd, 0, ml, count,whiteTurn, enPassant, whiteLeft, whiteRight, blackLeft, blackRight);
        srand((unsigned) time(NULL));
        int random = rand() % count;
        ml[random].move(*brd, ml[random].from, ml[random].to);

    }
}

void uciRunGame(){
    auto brd = std::make_unique<Board>(loadFenBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 "));
    auto state = std::make_unique<BoardState>(parseBoardState("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 "));
    while(1){
        if (std::cin.peek() != EOF) {
	        std::string str;
	        std::getline(std::cin, str);
	        proccessCommand(str, brd, state);
        }
    }
}

