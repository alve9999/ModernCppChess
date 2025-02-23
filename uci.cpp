#include "uci.hpp"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <stdlib.h> 
#include "movegen.hpp"
#include "move.hpp"

bool white = false;
Board* brd = new Board(loadFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"));

void proccessCommand(std::string str){
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
            Move move;
            if(white){
	            move = algebraicToMove<true>(tokens.back(),*brd);
            }
            else{
                move = algebraicToMove<false>(tokens.back(),*brd);
            }
            brd = new Board(makeMoveCall(*brd,move,brd->state.IsWhite,brd->state.EP,brd->state.WLC,brd->state.WRC,brd->state.BLC,brd->state.BRC));
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
        Move m;
        std::string promotion = "";
        if(m.special == 8 || m.special == 12){
            promotion = "n";
        }
        if(m.special == 9 || m.special == 13){
            promotion = "b";
        }
        if(m.special == 10 || m.special == 14){
            promotion = "r";
        }
        if(m.special == 11 || m.special == 15){
            promotion = "q";
        }

        std::cout <<"bestmove "<< m.toAlgebraic() << promotion << std::endl;
    }
}

void uciRunGame(){
    while(1){
        if (std::cin.peek() != EOF) {
	        std::string str;
	        std::getline(std::cin, str);
	        proccessCommand(str);
        }
    }
}
