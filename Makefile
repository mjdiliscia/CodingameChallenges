all: kotg ss

kotg: keepOffTheGrass.cpp
	mkdir -p build
	clang++ keepOffTheGrass.cpp -o build/kotg

ss: seabedSecurity.cpp
	mkdir -p build
	clang++ seabedSecurity.cpp -o build/ss

