CXX=g++-4.9
CXXFLAGS=-O3 -std=c++14 -fopenmp -Wall
LDFLAGS=-fopenmp

CLANG_CXX=clang++
CLANG_CXXFLAGS=-O3 -std=c++1y -pthread
CLANG_LDFLAGS=-pthread

#CXX=$(CLANG_CXX)
#CXXFLAGS=$(CLANG_CXXFLAGS)
#LDFLAGS=$(CLANG_LDFLAGS)

all: wsim wsim_viewer

wsim: main.o wsim.o system.o common.o
	$(CXX) $(LDFLAGS) $+ -o $@

wsim_viewer: viewer.o wsim.o system.o common.o
	$(CXX) $(LDFLAGS) $(shell sdl2-config --libs) $+ -o $@

viewer.o: src/viewer.cpp
	$(CXX) $(CXXFLAGS) $(shell sdl2-config --cflags) -c $+

%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $+

clean:
	rm -f *.o wsim wsim_viewer
