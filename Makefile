CXX=g++
CPPFLAGS+=-DNDEBUG
CXXFLAGS+=-g -Wall -Wextra -Werror -Wendif-labels -Wmissing-include-dirs -Wmultichar -Wno-unused-parameter -Wpointer-arith -Wnon-virtual-dtor -O3 -std=c++11 -fopenmp
LDFLAGS+=-fopenmp
EXE=
SYS=$(shell $(CXX) -dumpmachine)

ifeq ($(OS),Windows_NT)
	EXE=.exe
	LDFLAGS+=-static
else
	ifneq (, $(findstring mingw, $(SYS)))
		EXE=.exe
		LDFLAGS+=-static
	endif
endif

OBJECT=\
src/corpus.o \
src/lda-train.o \
src/rand.o \
src/sampler.o
BIN= lda-train$(EXE)

all: $(BIN)

$(BIN): $(OBJECT)
	$(CXX) -o $@ $^ $(LDFLAGS)

*.o: *.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $^

clean:
	rm -f $(OBJECT) $(LIB) $(BIN)

.PHONY: all clean
