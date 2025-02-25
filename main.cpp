#include "board.hpp"
#include "movegen.hpp"
#include "uci.hpp"
#include <chrono>
#include <iostream>
int main() {
    // MoveList ml = MoveList();
    constexpr const char* Fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 ";
    const Board brd =
        loadFenBoard(Fen);
    constexpr BoardState state = parseBoardState(Fen);
    // uciRunGame();
    auto start = std::chrono::high_resolution_clock::now();
    perft<state,7>(brd,0);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    auto time = duration.count();
    std::cout << "Time taken: " << time << " milliseconds" << std::endl;
    std::cout << "num nodes per second:" << c / ((double)time / 1000)
              << std::endl;
}
