CXX?=g++
LINK?=$(CXX)
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

SOURCE:=$(wildcard src/*.cc)
OBJECT:=$(subst src/,,$(patsubst %.cc,%.o,$(SOURCE)))
BIN:=lda-train$(EXE)

all: $(BIN)

include Makefile.depend

$(BIN): $(OBJECT)
	$(LINK) -o $@ $^ $(LDFLAGS)

%.o: src/%.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

depend: $(SOURCE)
	$(CXX) $(CPPFLAGS) -E -MM $^ > Makefile.depend

clean:
	rm -f $(OBJECT) $(BIN)

.PHONY: all clean depend
