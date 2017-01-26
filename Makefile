AR=ar rc
RANLIB=ranlib
CC=gcc
CXX=g++
CPPFLAGS=-Isrc -DNDEBUG
CFLAGS=-g -Wall -O3 -std=c89
CXXFLAGS=-g -Wall -O3 -std=c++98
LDFLAGS=
EXE=
SYS=$(shell $(CC) -dumpmachine)

ifeq ($(CLANG), 1)
	CC=clang
	CXX=clang++
	ifeq ($(CLANG_LIBCXX), 1)
		CXXFLAGS+=--stdlib=libc++
	endif
endif

ifeq ($(OS),Windows_NT)
	EXE=.exe
	LDFLAGS+=-lpsapi
	STATIC=1
else
	ifneq (, $(findstring mingw, $(SYS)))
		EXE=.exe
		LDFLAGS+=-lpsapi
		STATIC=1
	endif
endif

ifeq ($(STATIC), 1)
	LDFLAGS+=-static
endif

OBJECT=\
src/lda/alias.o \
src/lda/model.o \
src/lda/mt19937ar.o \
src/lda/mt19937-64.o \
src/lda/process-stat.o \
src/lda/rand.o \
src/lda/sampler.o

LIB=liblda-demo.a

BIN= lda-test$(EXE) lda-train$(EXE)

.PHONY: all clean
all: $(LIB) $(BIN)

$(LIB): $(OBJECT)
	$(AR) $@ $^
	$(RANLIB) $@

src/lda/mt19937-64.o: src/lda/mt19937-64.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

src/lda/mt19937ar.o: src/lda/mt19937ar.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

lda-test$(EXE): src/lda/lda-test.cc $(LIB)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ $^ $(LDFLAGS)

lda-train$(EXE): src/lda/lda-train.cc $(LIB)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(OBJECT) $(LIB) $(BIN)
