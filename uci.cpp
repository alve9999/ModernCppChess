#include "uci.hpp"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <stdlib.h> 
#include "movegen.hpp"
#include "move.hpp"

bool white = false;
/*
void proccessCommand(std::string str, Board* brd, BoardState* state){
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
            MoveCallbacks move;
            if(white){
	            move = algebraicToMove<true>(tokens.back(),*brd,*state,ep);
            }
            else{
                move = algebraicToMove<false>(tokens.back(),*brd,*state,ep);
            }
            *brd = move.boardCallback();
            *state = move.stateCallback();
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

    }
}

void uciRunGame(){
    Board brd = loadFenBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 ");
    BoardState state = parseBoardState("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 ");
    while(1){
        if (std::cin.peek() != EOF) {
	        std::string str;
	        std::getline(std::cin, str);
	        proccessCommand(str, &brd, &state);
        }
    }
}*/

