#include "board.hpp"
#include "eval.hpp"
#include "movegen.hpp"
#include "uci.hpp"
#include <chrono>
#include <iostream>
#include "SEE.hpp"
#include <memory>
#include "uci.hpp"
#include "ai.hpp"
#include "hash.hpp"
#include "move.hpp"
#include "eval.hpp"
#include "movegen.hpp"
int main(int argc, char** argv) {

    if (argc > 1 && std::string(argv[1]) == "bench") {
        runBench();
        return 0;
    }

    uciRunGame();
}
