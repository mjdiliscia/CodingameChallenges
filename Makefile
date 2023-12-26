all: build/kotg build/ss

build/kotg: keepOffTheGrass.cpp
	mkdir -p build
	clang++ keepOffTheGrass.cpp -o build/kotg

build/ss: seabedSecurity.cpp
	mkdir -p build
	clang++ seabedSecurity.cpp -o build/ss

