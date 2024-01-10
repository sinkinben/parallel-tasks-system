CPPFLAGS=g++ -m64 -std=c++17 -O3 -Wall
LINKFLAGS=-lm -lpthread

simple:
	$(CPPFLAGS) example-simple.cpp -o ./example-simple.exe $(LINKFLAGS)


sorting:
	$(CPPFLAGS) example-merge-sort.cpp -o ./example-merge-sort.exe $(LINKFLAGS)


test: simple sorting
	./example-simple.exe
	./example-merge-sort.exe

clean:
	rm *.exe