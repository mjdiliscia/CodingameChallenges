CC=clang
CXX=clang++

sources := $(wildcard *.cpp)
objects := $(patsubst %.cpp,build/%.o,$(sources))

all: build/seabedSecurity

build:
	mkdir -p build

build/%.o: %.cpp build
	$(CXX) -c $< -o $@

build/seabedSecurity: $(objects)
	clang++ $^ -o build/seabedSecurity
	codingame-merge -o build/seabedSecurity

