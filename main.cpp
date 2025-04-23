#include "board.hpp"
#include "eval.hpp"
#include "movegen.hpp"
#include "uci.hpp"
#include <chrono>
#include <iostream>

int main() {
    //const std::string str = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    //perft<parseBoardState("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1")>(loadFenBoard(str), 0,0,0,0,0,5,0);
    uciRunGame();
}
