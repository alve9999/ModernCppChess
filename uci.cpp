#include "uci.hpp"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <stdlib.h> 
#include "movegen.hpp"
#include "move.hpp"
#include "ai.hpp"
#include "hash.hpp"
#include <memory>
bool white = false;

std::string convertToUCI(int index) {
    int row = index / 8;
    int col = index % 8;

    char file = 'a' + col;

    char rank = '1' + (row); 

    return std::string(1, file) + std::string(1, rank);
}

std::string convertMoveToUCI(int from, int to) {
    return convertToUCI(from) + convertToUCI(to);
}

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
	            move = algebraicToMove<false>(tokens.back(),*brd,*state,ep);
            }
            else{
                move = algebraicToMove<true>(tokens.back(),*brd,*state,ep);
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
        bool whiteTurn   = state->IsWhite;
        bool enPassant   = state->EP;
        bool whiteLeft   = state->WLC;
        bool whiteRight  = state->WRC;
        bool blackLeft   = state->BLC;
        bool blackRight  = state->BRC;
        Callback ml = iterative_deepening(*brd,0,whiteTurn,enPassant,whiteLeft,whiteRight,blackLeft,blackRight,3);

        MoveResult moveRes = ml.makeMove(*brd, ml.from, ml.to);
        brd.reset(new Board(moveRes.board));
        state.reset(new BoardState(moveRes.state));

        std::cout << ttc << " " << ttf << std::endl;

        std::cout << "bestmove " << convertMoveToUCI(ml.from,ml.to)<< std::endl;

    }
}

void uciRunGame(){
    auto brd = std::make_unique<Board>(loadFenBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 "));
    auto state = std::make_unique<BoardState>(parseBoardState("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 "));
    while(1){
        if (std::cin.peek() != EOF) {
	        std::string str;
	        std::getline(std::cin, str);
            std::cout << str << std::endl;
	        proccessCommand(str, brd, state);
        }
    }
}

