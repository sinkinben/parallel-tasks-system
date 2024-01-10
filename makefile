CPPFLAGS=g++ -m64 -std=c++17 -O3 -Wall
LINKFLAGS=-lm -lpthread

all:
	$(CPPFLAGS) example.cpp -o ./a.out $(LINKFLAGS)