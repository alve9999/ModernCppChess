CXX ?= g++
EXE ?= engine

build:
	mkdir -p build
	cd build && cmake -DCMAKE_CXX_COMPILER=$(CXX) .. 
	cd build && cmake --build .

all: build
	cp build/ModernCppChess $(EXE)

clean:
	rm -rf build
